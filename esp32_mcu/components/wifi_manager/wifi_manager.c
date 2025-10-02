#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "wifi_manager.h"
#include "constants.h"
#include "logs.h"

// admin portal
#include "dns_server.h"
#include "http_server.h"

// ESP-IDF networking stack
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

// NVM
#include "nvs_flash.h"
#include "nvs.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/ip4_addr.h"

#include "bits.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "WIFI Manager";

static EventGroupHandle_t wifi_manager_event_group;

// task handle for the main wifi_manager task
static TaskHandle_t task_wifi_manager = NULL;

// netif object for the STATION
static esp_netif_t* esp_netif_sta = NULL;

// netif object for the ACCESS POINT
static esp_netif_t* esp_netif_ap = NULL;

// STA WiFi records
static wifi_ap_record_t *accessp_records = NULL;

// Global buffer to store JSON representation of access points
static char *accessp_json = NULL;

// The actual WiFi settings in use
struct wifi_settings_t wifi_settings = {
	.ap_ssid        = DEFAULT_AP_SSID,
	.ap_pwd         = DEFAULT_AP_PASSWORD,
	.ap_channel     = DEFAULT_AP_CHANNEL,
	.ap_ssid_hidden = DEFAULT_AP_SSID_HIDDEN,
	.ap_bandwidth   = DEFAULT_AP_BANDWIDTH,
    .sta_ssid       = DEFAULT_EMPTY,
    .sta_pwd        = DEFAULT_EMPTY
};

// objects used to manipulate the main queue of events
QueueHandle_t wifi_manager_queue;

BaseType_t wifi_manager_send_message(message_code_t code, void *param)
{
	queue_message msg;
	msg.code = code;
	msg.param = param;
	return xQueueSend( wifi_manager_queue, &msg, portMAX_DELAY);
}

// Array of callback function pointers
void (*cb_ptr_arr[WM_MESSAGE_CODE_COUNT])(void*) = {NULL};

esp_err_t wifi_manager_set_callback(message_code_t message_code, void (*func_ptr)(void*) )
{
	if(message_code < WM_MESSAGE_CODE_COUNT){
		cb_ptr_arr[message_code] = func_ptr;
        return ESP_OK;
    }
    return ESP_ERR_WIFI_NOT_INIT;
}

void wifi_manager_scan_async()
{
    wifi_manager_send_message(WM_ORDER_START_WIFI_SCAN, NULL);
}

void wifi_manager_disconnect_async()
{
    wifi_manager_send_message(WM_ORDER_DISCONNECT_STA, NULL);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_WIFI_READY:
                LOGI(TAG, "EVENT: WIFI_EVENT_WIFI_READY");
                break;
            case WIFI_EVENT_SCAN_DONE:
                LOGI(TAG, "EVENT: WIFI_EVENT_SCAN_DONE");
                xEventGroupClearBits(wifi_manager_event_group, WIFI_MANAGER_SCAN_BIT);
                wifi_event_sta_scan_done_t* event_sta_scan_done = (wifi_event_sta_scan_done_t*)malloc(sizeof(wifi_event_sta_scan_done_t));
                *event_sta_scan_done = *((wifi_event_sta_scan_done_t*)event_data);
                wifi_manager_send_message(WM_EVENT_SCAN_DONE, event_sta_scan_done);
                break;
            case WIFI_EVENT_STA_START:
                LOGI(TAG, "EVENT: WIFI_EVENT_STA_START");
                break;
            case WIFI_EVENT_STA_STOP:
                LOGI(TAG, "EVENT: WIFI_EVENT_STA_STOP");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                LOGI(TAG, "EVENT: WIFI_EVENT_STA_CONNECTED");            
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                LOGI(TAG, "EVENT: WIFI_EVENT_STA_DISCONNECTED");
                break;
            case WIFI_EVENT_STA_AUTHMODE_CHANGE:
                LOGI(TAG, "EVENT: WIFI_EVENT_STA_AUTHMODE_CHANGE");
                break;
            case WIFI_EVENT_AP_START:
                LOGI(TAG, "EVENT: WIFI_EVENT_AP_START");
                xEventGroupSetBits(wifi_manager_event_group, WIFI_MANAGER_AP_STARTED_BIT);
                break;
            case WIFI_EVENT_AP_STOP:
                LOGI(TAG, "EVENT: WIFI_EVENT_AP_STOP");
                xEventGroupClearBits(wifi_manager_event_group, WIFI_MANAGER_AP_STARTED_BIT);
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                LOGI(TAG, "EVENT: WIFI_EVENT_AP_STACONNECTED");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                LOGI(TAG, "EVENT: WIFI_EVENT_AP_STADISCONNECTED");
                break;
            case WIFI_EVENT_AP_PROBEREQRECVED:
                LOGI(TAG, "EVENT: WIFI_EVENT_AP_PROBEREQRECVED");
                break;
            default:
                break;
        }
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_base == IP_EVENT)
    {
		switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                LOGI(TAG, "EVENT: IP_EVENT_STA_GOT_IP");
                break;
            case IP_EVENT_GOT_IP6:
                LOGI(TAG, "EVENT: IP_EVENT_GOT_IP6");
                break;
            case IP_EVENT_STA_LOST_IP:
                LOGI(TAG, "EVENT: IP_EVENT_STA_LOST_IP");
                break;
            default:
                break;
        }
    }
}

// Store wifi_settings to NVM
esp_err_t wifi_manager_save_config()
{
    LOGI(TAG,"Store WiFi configuration to NVM");

#ifdef DIAGNOSTIC_VERSION
    ESP_LOGI(TAG, "Current WiFi configuration:\n"
        "\t\tAP SSID: \"%s\"\n"
        "\t\tAP pwd: \"%s\"\n"
        "\t\tAP channel: %i\n"
        "\t\tAP hidden: %s\n"
        "\t\tAP bandwidth (1 = 20MHz, 2 = 40MHz): %i\n"
        "\t\tSTA SSID: \"%s\"\n"
        "\t\tSTA pwd: \"%s\"",
        wifi_settings.ap_ssid,
        wifi_settings.ap_pwd,
        wifi_settings.ap_channel,
        (wifi_settings.ap_ssid_hidden ? "yes" : "no"),
        wifi_settings.ap_bandwidth,
        wifi_settings.sta_ssid,
        wifi_settings.sta_pwd
    );
#endif

    static const char *LOG_FMT_FAILED_SAVE = "Failed saving %s to NVM \"%s\":\"%s\" error: %s";
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, NVS_READWRITE, &handle);
    if(err==ESP_OK) {
        // Read buffer to check if the value has changed. Used to prevent memory degradation.
        struct wifi_settings_t tmp_conf;
        memset(&tmp_conf, 0x00, sizeof(tmp_conf));

        // Save AP SSID with check that value changed
        size_t size = sizeof(tmp_conf.ap_ssid);
        err = nvs_get_str(handle, STORE_NVM_KEY_AP_SSID, tmp_conf.ap_ssid, &size);
        if((err == ESP_OK && strcmp(tmp_conf.ap_ssid, wifi_settings.ap_ssid) != 0) || 
            err == ESP_ERR_NVS_NOT_FOUND)
        {
            err = nvs_set_str(handle, STORE_NVM_KEY_AP_SSID, wifi_settings.ap_ssid);
            if(err != ESP_OK)
                LOGE(TAG, LOG_FMT_FAILED_SAVE,
                    STORE_NVM_KEY_AP_SSID, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, esp_err_to_name(err));
        }

        // Save AP Password with check that value changed
        size = sizeof(wifi_settings.ap_pwd);
        err = nvs_get_str(handle, STORE_NVM_KEY_AP_PSW, tmp_conf.ap_pwd, &size);
        if((err == ESP_OK && strcmp(tmp_conf.ap_pwd, wifi_settings.ap_pwd) != 0) || 
            err == ESP_ERR_NVS_NOT_FOUND)
        {
            err = nvs_set_str(handle, STORE_NVM_KEY_AP_PSW, wifi_settings.ap_pwd);
            if (err!=ESP_OK)
                LOGE(TAG, LOG_FMT_FAILED_SAVE,
                    STORE_NVM_KEY_AP_PSW, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, esp_err_to_name(err));
        }

        // Save STA SSID with check that value changed
        size = sizeof(wifi_settings.sta_ssid);
        err = nvs_get_str(handle, STORE_NVM_KEY_STA_SSID, tmp_conf.sta_ssid, &size);
        if((err == ESP_OK && strcmp(tmp_conf.sta_ssid, wifi_settings.sta_ssid) != 0) || 
            err == ESP_ERR_NVS_NOT_FOUND)
        {
            err = nvs_set_str(handle, STORE_NVM_KEY_STA_SSID, wifi_settings.sta_ssid);
            if (err!=ESP_OK)
                LOGE(TAG, LOG_FMT_FAILED_SAVE,
                    STORE_NVM_KEY_STA_SSID, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, esp_err_to_name(err));
        }

        // Save STA Password with check that value changed
        size = sizeof(wifi_settings.sta_pwd);
        err = nvs_get_str(handle, STORE_NVM_KEY_STA_PSW, tmp_conf.sta_pwd, &size);
        if((err == ESP_OK && strcmp(tmp_conf.sta_pwd, wifi_settings.sta_pwd) != 0) || 
            err == ESP_ERR_NVS_NOT_FOUND)
        {
            err = nvs_set_str(handle, STORE_NVM_KEY_STA_PSW, wifi_settings.sta_pwd);
            if (err!=ESP_OK)
                LOGE(TAG, LOG_FMT_FAILED_SAVE,
                    STORE_NVM_KEY_STA_PSW, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, esp_err_to_name(err));
        }

        // Save AP channel
        err = nvs_get_u8(handle, STORE_NVM_KEY_AP_CHANNEL, &tmp_conf.ap_channel);
        if((err == ESP_OK && tmp_conf.ap_channel != wifi_settings.ap_channel) ||
            err == ESP_ERR_NVS_NOT_FOUND)
        {
            err = nvs_set_u8(handle, STORE_NVM_KEY_AP_CHANNEL, tmp_conf.ap_channel);
            if (err!=ESP_OK)
                LOGE(TAG, LOG_FMT_FAILED_SAVE,
                    STORE_NVM_KEY_AP_CHANNEL, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, esp_err_to_name(err));
        }

        // Save hidden flag
        err = nvs_get_u8(handle, STORE_NVM_KEY_AP_SSID_HIDDEN, &tmp_conf.ap_ssid_hidden);
        if((err == ESP_OK && tmp_conf.ap_ssid_hidden != wifi_settings.ap_ssid_hidden)|| 
            err == ESP_ERR_NVS_NOT_FOUND)
        {
            err = nvs_set_u8(handle, STORE_NVM_KEY_AP_SSID_HIDDEN, tmp_conf.ap_ssid_hidden);
            if (err!=ESP_OK)
                LOGE(TAG, LOG_FMT_FAILED_SAVE,
                    STORE_NVM_KEY_AP_SSID_HIDDEN, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, esp_err_to_name(err));
        }

        // Save AP bandwidth
        uint8_t value = DEFAULT_AP_BANDWIDTH;
        err = nvs_get_u8(handle, STORE_NVM_KEY_AP_BANDWIDTH, &value);
        if((err == ESP_OK && value != wifi_settings.ap_bandwidth) ||
            err == ESP_ERR_NVS_NOT_FOUND)
        {
            value = (uint8_t)wifi_settings.ap_bandwidth;
            err = nvs_set_u8(handle, STORE_NVM_KEY_AP_BANDWIDTH, value);
            if (err!=ESP_OK)
                LOGE(TAG, LOG_FMT_FAILED_SAVE,
                    STORE_NVM_KEY_AP_BANDWIDTH, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, esp_err_to_name(err));
        }
    }

    if (err == ESP_OK)
        err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

// Load from NVM or Default for wifi_settings
esp_err_t wifi_manager_load_config()
{
    LOGI(TAG,"Load WiFi configuration from NVM");
    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, NVS_READONLY, &handle);
    if(err==ESP_OK) {

        // Load AP SSID
        size_t size = sizeof(wifi_settings.ap_ssid);
        err = nvs_get_str(handle, STORE_NVM_KEY_AP_SSID, wifi_settings.ap_ssid, &size);
        if(err!=ESP_OK){
            // Use default value
            strncpy(wifi_settings.ap_ssid, DEFAULT_AP_SSID, sizeof(wifi_settings.ap_ssid));
            wifi_settings.ap_ssid[sizeof(wifi_settings.ap_ssid) - 1] = '\0';
        }

        // Load AP Password
        size = sizeof(wifi_settings.ap_pwd);
        err = nvs_get_str(handle, STORE_NVM_KEY_AP_PSW, wifi_settings.ap_pwd, &size);
        if(err!=ESP_OK){
            // Use default value
            strncpy(wifi_settings.ap_pwd, DEFAULT_AP_PASSWORD, sizeof(wifi_settings.ap_pwd));
            wifi_settings.ap_pwd[sizeof(wifi_settings.ap_pwd) - 1] = '\0';
        }

        // Load STA SSID
        size = sizeof(wifi_settings.sta_ssid);
        err = nvs_get_str(handle, STORE_NVM_KEY_STA_SSID, wifi_settings.sta_ssid, &size);
        if(err!=ESP_OK){
            // Use default value
            strncpy(wifi_settings.sta_ssid, DEFAULT_EMPTY, sizeof(wifi_settings.sta_ssid));
            wifi_settings.sta_ssid[sizeof(wifi_settings.sta_ssid) - 1] = '\0';
        }

        // Load STA Password
        size = sizeof(wifi_settings.sta_pwd);
        err = nvs_get_str(handle, STORE_NVM_KEY_STA_PSW, wifi_settings.sta_pwd, &size);
        if(err!=ESP_OK){
            // Use default value
            strncpy(wifi_settings.sta_pwd, DEFAULT_EMPTY, sizeof(wifi_settings.sta_pwd));
            wifi_settings.sta_pwd[sizeof(wifi_settings.sta_pwd) - 1] = '\0';
        }

        // Load AP channel
        err = nvs_get_u8(handle, STORE_NVM_KEY_AP_CHANNEL, &wifi_settings.ap_channel);
        if(err!=ESP_OK){
            // Use default value
            wifi_settings.ap_channel = DEFAULT_AP_CHANNEL;
        }

        // Load AP hidden flag
        err = nvs_get_u8(handle, STORE_NVM_KEY_AP_SSID_HIDDEN, &wifi_settings.ap_ssid_hidden);
        if(err!=ESP_OK){
            // Use default value
            wifi_settings.ap_ssid_hidden = DEFAULT_AP_SSID_HIDDEN;
        }

        // Load AP bandwidth
        uint8_t value = DEFAULT_AP_BANDWIDTH;
        err = nvs_get_u8(handle, STORE_NVM_KEY_AP_BANDWIDTH, &value);
        if(err!=ESP_OK){
            // Use default value
            wifi_settings.ap_bandwidth = DEFAULT_AP_BANDWIDTH;
        }
        else{
            wifi_settings.ap_bandwidth = (wifi_bandwidth_t)value;
        }
    }

#ifdef DIAGNOSTIC_VERSION
    ESP_LOGI(TAG, "Current WiFi configuration %s\n"
        "\t\tAP SSID: \"%s\"\n"
        "\t\tAP pwd: \"%s\"\n"
        "\t\tAP channel: %i\n"
        "\t\tAP hidden: %s\n"
        "\t\tAP bandwidth (1 = 20MHz, 2 = 40MHz): %i\n"
        "\t\tSTA SSID: \"%s\"\n"
        "\t\tSTA pwd: \"%s\"",
        (err==ESP_OK ? "load from NVM" : "created by default"),
        wifi_settings.ap_ssid,
        wifi_settings.ap_pwd,
        wifi_settings.ap_channel,
        (wifi_settings.ap_ssid_hidden ? "yes" : "no"),
        wifi_settings.ap_bandwidth,
        wifi_settings.sta_ssid,
        wifi_settings.sta_pwd
    );
#endif

    nvs_close(handle);
    return err;
}

bool is_wifi_sta_configured()
{
    // check only the SSID in case the STA allows access without a password.
    if (wifi_settings.sta_ssid[0] != '\0'){
        return true;
    }
    return false;
}

esp_err_t configure_and_start_ap()
{
	// Access Point configuration setup
	wifi_config_t ap_config = {
		.ap = {
			.ssid_len = 0,
			.channel = wifi_settings.ap_channel,
			.ssid_hidden = wifi_settings.ap_ssid_hidden,
			.max_connection = DEFAULT_AP_MAX_CONNECTIONS,
			.beacon_interval = DEFAULT_AP_BEACON_INTERVAL,
		},
	};
	memcpy(ap_config.ap.ssid, wifi_settings.ap_ssid , sizeof(wifi_settings.ap_ssid));

	// if the password lenght is under 8 char which is the minium for WPA2, the access point starts as open
	if(strlen( (char*)wifi_settings.ap_pwd) < WPA2_MINIMUM_PASSWORD_LENGTH){
		ap_config.ap.authmode = WIFI_AUTH_OPEN;
		memset( ap_config.ap.password, 0x00, sizeof(ap_config.ap.password) );
	}
	else{
		ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
		memcpy(ap_config.ap.password, wifi_settings.ap_pwd, sizeof(wifi_settings.ap_pwd));
	}
    // AP mode should be set before change
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err!=ESP_OK)
        LOGE(TAG,"AP set set mode APSTA failed: %s", esp_err_to_name(err));
    // Set AP config
    if (err == ESP_OK) {
        err = esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);
        if (err!=ESP_OK)
            LOGE(TAG,"AP set config failed: %s", esp_err_to_name(err));
    }
    // Set bandwidth
    if (err == ESP_OK) {
        wifi_bandwidth_t current_bandwidth = 0;
        err = esp_wifi_get_bandwidth(WIFI_IF_AP, &current_bandwidth);
        if (err==ESP_OK && current_bandwidth != wifi_settings.ap_bandwidth)
            err = esp_wifi_set_bandwidth(WIFI_IF_AP, wifi_settings.ap_bandwidth);
        if (err!=ESP_OK)
            LOGE(TAG,"AP set bandwidth failed: %s", esp_err_to_name(err));
    }
    return err;
}

void stop_ap()
{
    esp_wifi_set_mode(WIFI_MODE_STA);
}

void wifi_manager(void * pvParameters)
{
    // Load WiFi configuration from NVM
    esp_err_t err = wifi_manager_load_config();
    if (err==ESP_OK) {
        LOGI(TAG,"WiFi config loaded");
    }
    else {
        LOGW(TAG,"WiFi config not loaded. Used default.");
    }

	// event handler for the connection
    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;
    ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,&instance_wifi_event),
                "Failed registered listener to WIFI_EVENT");
    ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL,&instance_ip_event),
                "Failed registered listener to IP_EVENT");


	// DHCP AP configuration
	esp_netif_dhcps_stop(esp_netif_ap); // DHCP client/server must be stopped before setting new IP information.
	esp_netif_ip_info_t ap_ip_info;
	memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));
	inet_pton(AF_INET, DEFAULT_AP_IP, &ap_ip_info.ip);
	inet_pton(AF_INET, DEFAULT_AP_GATEWAY, &ap_ip_info.gw);
	inet_pton(AF_INET, DEFAULT_AP_NETMASK, &ap_ip_info.netmask);
	ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info),
                "NETIF set ip info failed");
	ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap),
                "NETIF start DHCPS failed");

    // Setup WiFi
	ERROR_CHECK(esp_wifi_set_ps(DEFAULT_STA_POWER_SAVE),
                "Set Power Save failed");

	// by default the mode is STA because wifi_manager will not start the access point unless it has to!
	ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA),
                "Set Mode STA failed");
	ERROR_CHECK(esp_wifi_start(),
                "Start failed");

	// wifi scanner config
	wifi_scan_config_t scan_config = {
		.ssid = 0,
		.bssid = 0,
		.channel = 0,
		.show_hidden = true
	};

	// enqueue first event: load previous config
	wifi_manager_send_message(WM_ORDER_LOAD_AND_RESTORE_STA, NULL);
    
    // main wifi_manager loop
    queue_message msg;
	for(;;){
        BaseType_t xStatus = xQueueReceive( wifi_manager_queue, &msg, portMAX_DELAY );
        if(xStatus != pdPASS)
            continue;
        LOGI(TAG, "COMMAND: %s", message_code_to_str(msg.code));
        switch(msg.code)
        {
            case WM_ORDER_LOAD_AND_RESTORE_STA:
                if(is_wifi_sta_configured()){
                    LOGI(TAG, "Saved WiFi found on startup. Will attempt to connect.");
                    wifi_manager_send_message(WM_ORDER_CONNECT_STA, (void*)CONNECTION_REQUEST_RESTORE_CONNECTION);
                }
                else{
                    // no wifi saved: start soft AP! This is what should happen during a first run
                    LOGI(TAG, "No saved WiFi found on startup. Starting Access Point.");
                    wifi_manager_send_message(WM_ORDER_START_AP, NULL);
                }
                // callback
				if(cb_ptr_arr[msg.code])
                    (*cb_ptr_arr[msg.code])(NULL);
                break;

            case WM_ORDER_START_AP:
                esp_err_t err = configure_and_start_ap();
                if (err == ESP_OK)
                {
                    // Start DNS server to have Captive Portal
                    dns_server_start();
                    // Start HTTP server with admin page
                    http_server_start();
                    LOGI(TAG, "AP started. DNS and HTTP server have been launched.");
                }
                else
                    LOGE(TAG, "Failed to start AP. error: %s", esp_err_to_name(err));

                /* callback */
                if(cb_ptr_arr[msg.code])
                    (*cb_ptr_arr[msg.code])((void*)err);
                break;

            case WM_ORDER_STOP_AP:
                //http_server_stop();
                //dns_server_stop();
                stop_ap();
                /* callback */
                if(cb_ptr_arr[msg.code])
                    (*cb_ptr_arr[msg.code])(NULL);
                break;

            case WM_ORDER_START_WIFI_SCAN:
				// avoid scan if already in progress
				EventBits_t uxBits = xEventGroupGetBits(wifi_manager_event_group);
				if(! (uxBits & WIFI_MANAGER_SCAN_BIT) )
                {
					xEventGroupSetBits(wifi_manager_event_group, WIFI_MANAGER_SCAN_BIT);
					ERROR_CHECK(esp_wifi_scan_start(&scan_config, false),
                                "Failed scan WiFi");
				}
				/* callback */
				if(cb_ptr_arr[msg.code])
                    (*cb_ptr_arr[msg.code])(NULL);
				break;

            case WM_EVENT_SCAN_DONE:
                wifi_event_sta_scan_done_t *evt_scan_done = (wifi_event_sta_scan_done_t*)msg.param;
                LOGI(TAG, "STA scan done: status: %s, count: %d, scan_id: %d",
                    (evt_scan_done->status==0?"success":"failure"),
                    evt_scan_done->number, evt_scan_done->scan_id );
                // only check for AP if the scan is succesful
				if(evt_scan_done->status == 0) {
                    uint16_t ap_num = MAX_AP_NUM;
                    ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, accessp_records),
                                "Scan WiFi failed");
                    create_json_from_ap_records(accessp_records, ap_num);
                }

				/* callback */
				if(cb_ptr_arr[msg.code])
                    (*cb_ptr_arr[msg.code])( msg.param );

                free(evt_scan_done);
                break;

            default:
				break;                
        }
    } // end of for loop
    vTaskDelete( NULL );
}

void create_json_from_ap_records(wifi_ap_record_t *ap_records, uint16_t ap_count)
{
    if (accessp_json == NULL || ap_records == NULL) {
        LOGE(TAG, "accessp_json or ap_records is NULL");
        return;
    }

    // Calculate total buffer size
    size_t buffer_size = MAX_AP_NUM * MAXONEJSONRECORD + 4;
    LOGI(TAG, "Starting JSON generation: %d APs found, buffer size: %zu bytes", ap_count, buffer_size);

    // Sort AP records by RSSI (better signal first)
    // Create a copy array with indices for sorting
    typedef struct {
        uint16_t index;
        int8_t rssi;
    } ap_sort_entry_t;
    
    ap_sort_entry_t sort_array[MAX_AP_NUM];
    for (uint16_t i = 0; i < ap_count && i < MAX_AP_NUM; i++) {
        sort_array[i].index = i;
        sort_array[i].rssi = ap_records[i].rssi;
    }
    
    // Simple bubble sort by RSSI (better signal = higher RSSI value = less negative)
    for (uint16_t i = 0; i < ap_count - 1; i++) {
        for (uint16_t j = 0; j < ap_count - 1 - i; j++) {
            if (sort_array[j].rssi < sort_array[j + 1].rssi) {
                // Swap entries
                ap_sort_entry_t temp = sort_array[j];
                sort_array[j] = sort_array[j + 1];
                sort_array[j + 1] = temp;
            }
        }
    }

    // Start with opening bracket
    strcpy(accessp_json, "[\n");
    
    bool first_entry = true;
    size_t current_length = strlen(accessp_json);
    int processed_count = 0;
    int skipped_empty = 0;
    int skipped_duplicate = 0;
    
    // Process APs in sorted order
    for (uint16_t i = 0; i < ap_count; i++) {
        uint16_t ap_idx = sort_array[i].index;
        
        // Filter out empty SSIDs
        if (ap_records[ap_idx].ssid[0] == '\0') {
            skipped_empty++;
            continue;
        }
        
        // Check for duplicate SSIDs by comparing with previously processed entries
        bool is_duplicate = false;
        for (uint16_t j = 0; j < i; j++) {
            uint16_t prev_ap_idx = sort_array[j].index;
            if (ap_records[prev_ap_idx].ssid[0] != '\0' && 
                strcmp((char*)ap_records[ap_idx].ssid, (char*)ap_records[prev_ap_idx].ssid) == 0) {
                is_duplicate = true;
                break;
            }
        }
        
        if (is_duplicate) {
            skipped_duplicate++;
            continue;
        }
        
        // Create JSON entry first to check its size
        char json_entry[MAXONEJSONRECORD];
        int entry_len = snprintf(json_entry, sizeof(json_entry), 
                "%s{\"ssid\":\"%s\",\"chan\":%d,\"rssi\":%d,\"auth\":%d}",
                first_entry ? "" : ",\n",
                (char*)ap_records[ap_idx].ssid,
                ap_records[ap_idx].primary,
                ap_records[ap_idx].rssi,
                ap_records[ap_idx].authmode);
        
        // Check if adding this entry would exceed buffer size (leave space for closing "]")
        if (current_length + entry_len + 3 >= buffer_size) {
            LOGW(TAG, "JSON buffer would overflow, stopping at entry %d", i);
            break;
        }
        
        // Add the entry to the buffer
        strcat(accessp_json, json_entry);
        current_length += entry_len;
        first_entry = false;
        processed_count++;
        
        LOGI(TAG, "Added AP %d: '%s' ch:%d rssi:%d auth:%d", 
             processed_count, (char*)ap_records[ap_idx].ssid, 
             ap_records[ap_idx].primary, ap_records[ap_idx].rssi, ap_records[ap_idx].authmode);
    }
    
    // Close the JSON array
    strcat(accessp_json, "\n]");
}

const char* wifi_manager_get_ap_json()
{
    return accessp_json;
}

void wifi_manager_start(void)
{
    // disable default wifi logging
    disable_esp_log("wifi");
    LOGI(TAG, "Start manager");

  	// initialize the tcp stack
      ERROR_CHECK(esp_netif_init(),
      "Error initialize the tcp stack");

    // event loop for the wifi driver
    ERROR_CHECK(esp_event_loop_create_default(),
        "Error create event loop for the wifi driver");

    esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_ap = esp_netif_create_default_wifi_ap();

    // default wifi config
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ERROR_CHECK(esp_wifi_init(&wifi_init_config),
        "Error setup defaut wifi config");
    ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM),
        "Cannot set wifi storage");

    // create event group
    wifi_manager_event_group = xEventGroupCreate();
    
    // Create wifi manager message queue
    wifi_manager_queue = xQueueCreate( 3, sizeof( queue_message) );

    // WiFi STA records
    accessp_records = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * MAX_AP_NUM);
    
    // JSON buffer for access points
    size_t json_buffer_size = MAX_AP_NUM * MAXONEJSONRECORD + 4; // 4 bytes for "[\n" and "]\0"
    accessp_json = (char*)malloc(json_buffer_size);
    if (accessp_json != NULL) {
        accessp_json[0] = '\0'; // Initialize as empty string
        LOGI(TAG, "Allocated JSON buffer: %zu bytes (%d APs * %d bytes/AP + 4)", 
             json_buffer_size, MAX_AP_NUM, MAXONEJSONRECORD);
    } else {
        LOGE(TAG, "Failed to allocate JSON buffer of %zu bytes", json_buffer_size);
    }
    
    xTaskCreate(&wifi_manager, "wifi_manager", 4096, NULL, WIFI_MANAGER_TASK_PRIORITY, &task_wifi_manager);
}

void wifi_manager_stop()
{
    LOGI(TAG, "Stop manager");
	vTaskDelete(task_wifi_manager);
	task_wifi_manager = NULL;

    // delete wifi manager message queue
    vQueueDelete(wifi_manager_queue);
	wifi_manager_queue = NULL;

    if (accessp_records != NULL) {
	    free(accessp_records);
	    accessp_records = NULL;
    }

    if (accessp_json != NULL) {
        free(accessp_json);
        accessp_json = NULL;
    }

    vEventGroupDelete(wifi_manager_event_group);
	wifi_manager_event_group = NULL;
}

esp_netif_t* wifi_manager_get_esp_netif_ap()
{
	return esp_netif_ap;
}

esp_netif_t* wifi_manager_get_esp_netif_sta()
{
	return esp_netif_sta;
}
