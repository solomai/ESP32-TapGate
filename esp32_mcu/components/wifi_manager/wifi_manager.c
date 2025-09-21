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

static const char *TAG = "WIFI Manager";

// task handle for the main wifi_manager task
static TaskHandle_t task_wifi_manager = NULL;

// netif object for the STATION
static esp_netif_t* esp_netif_sta = NULL;

// netif object for the ACCESS POINT
static esp_netif_t* esp_netif_ap = NULL;

// Current STA config
wifi_config_t* wifi_manager_config_sta = NULL;

// The actual WiFi settings in use
struct wifi_settings_t wifi_settings = {
	.ap_ssid = DEFAULT_AP_SSID,
	.ap_pwd = DEFAULT_AP_PASSWORD,
	.ap_channel = DEFAULT_AP_CHANNEL,
	.ap_ssid_hidden = DEFAULT_AP_SSID_HIDDEN,
	.ap_bandwidth = DEFAULT_AP_BANDWIDTH,
	.sta_only = DEFAULT_STA_ONLY,
	.sta_power_save = DEFAULT_STA_POWER_SAVE,
	.sta_static_ip = 0,
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
                break;
            case WIFI_EVENT_AP_STOP:
                LOGI(TAG, "EVENT: WIFI_EVENT_AP_STOP");
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

esp_err_t wifi_manager_save_sta_config()
{
    if(wifi_manager_config_sta==NULL)
    {
        LOGE(TAG, "Error saving WiFi STA config: no settings exist");
        return ESP_ERR_WIFI_SSID;
    }

    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, NVS_READWRITE, &handle);
    if(err != ESP_OK){
        LOGE(TAG, "Error saving WiFi STA config to NVM \"%s\":\"%s\"", NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
        return err;
    }

    wifi_config_t tmp_conf;
	memset(&tmp_conf, 0x00, sizeof(tmp_conf));

    // Save ssid
    bool ssid_stored = false;
    size_t ssid_size = sizeof(tmp_conf.sta.ssid);
    err = nvs_get_blob(handle, STORE_NVM_SSID, tmp_conf.sta.ssid, &ssid_size);
    if( (err == ESP_OK  || err == ESP_ERR_NVS_NOT_FOUND)
        && strcmp( (char*)tmp_conf.sta.ssid, (char*)wifi_manager_config_sta->sta.ssid) != 0)
    {
        err = nvs_set_blob(handle, STORE_NVM_SSID, wifi_manager_config_sta->sta.ssid, ssid_size);
        if(err!=ESP_OK)
        {
            LOGE(TAG, "Failed saving %s to NVM \"%s\":\"%s\"", STORE_NVM_SSID, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
            nvs_close(handle);
            return err;
        }
        ssid_stored = true;
    }

    // Save password
    bool password_stored = false;
    size_t psw_size = sizeof(tmp_conf.sta.password);
    err = nvs_get_blob(handle, STORE_NVM_PSW, tmp_conf.sta.password, &psw_size);
    if( (err == ESP_OK  || err == ESP_ERR_NVS_NOT_FOUND)
        && strcmp( (char*)tmp_conf.sta.password, (char*)wifi_manager_config_sta->sta.password) != 0)
    {
        err = nvs_set_blob(handle, STORE_NVM_PSW, wifi_manager_config_sta->sta.password, psw_size);
        if(err!=ESP_OK)
        {
            if (ssid_stored)
                LOGI(TAG, "Saved %s:%s to NVM \"%s\":\"%s\"", STORE_NVM_SSID, (char*)wifi_manager_config_sta->sta.ssid, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
            LOGE(TAG, "Failed saving %s to NVM \"%s\":\"%s\"", STORE_NVM_PSW, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
            nvs_close(handle);
            return err;
        }
        password_stored = true;
    }

    // Save settings
    bool settings_stored = false;
    struct wifi_settings_t tmp_settings;
    memset(&tmp_settings, 0x00, sizeof(tmp_settings));
    size_t settings_size = sizeof(tmp_settings);
    err = nvs_get_blob(handle, STORE_NVM_SETTINGS, &tmp_settings, &settings_size);
    if( (err == ESP_OK  || err == ESP_ERR_NVS_NOT_FOUND) &&
    (
        strcmp( (char*)tmp_settings.ap_ssid, (char*)wifi_settings.ap_ssid) != 0 ||
        strcmp( (char*)tmp_settings.ap_pwd, (char*)wifi_settings.ap_pwd) != 0 ||
        tmp_settings.ap_ssid_hidden != wifi_settings.ap_ssid_hidden ||
        tmp_settings.ap_bandwidth != wifi_settings.ap_bandwidth ||
        tmp_settings.sta_only != wifi_settings.sta_only ||
        tmp_settings.sta_power_save != wifi_settings.sta_power_save ||
        tmp_settings.ap_channel != wifi_settings.ap_channel
    )
    ){
        err = nvs_set_blob(handle, STORE_NVM_SETTINGS, &wifi_settings, sizeof(wifi_settings));
        if(err!=ESP_OK)
        {
            if (ssid_stored)
                LOGI(TAG, "Saved %s:%s to NVM \"%s\":\"%s\"", STORE_NVM_SSID, (char*)wifi_manager_config_sta->sta.ssid, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
            if (password_stored)
                LOGI(TAG, "Saved %s:%s to NVM \"%s\":\"%s\"", STORE_NVM_PSW, (char*)wifi_manager_config_sta->sta.password, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
            LOGE(TAG, "Failed saving %s to NVM \"%s\":\"%s\"", STORE_NVM_SETTINGS, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
            nvs_close(handle);
            return err;
        }
        settings_stored = true;
    }

    #ifdef DIAGNOSTIC_VERSION
    ESP_LOGI(TAG,
        "\t\tssid:%s%s\n"
        "\t\tpassword:%s%s\n"
        "%s",
        wifi_manager_config_sta->sta.ssid,
        ssid_stored ? "" : " (update skipped because value did not change)",
        wifi_manager_config_sta->sta.password,
        password_stored ? "" : " (update skipped because value did not change)",
        settings_stored ? 
            // Normal block with settings
            "\t\tSoftAP_ssid:%s\n"
            "\t\tSoftAP_pwd:%s\n"
            "\t\tSoftAP_channel:%i\n"
            "\t\tSoftAP_hidden (1 = yes):%i\n"
            "\t\tSoftAP_bandwidth (1 = 20MHz, 2 = 40MHz):%i\n"
            "\t\tsta_only (0 = APSTA, 1 = STA when connected):%i\n"
            "\t\tsta_power_save (1 = yes):%i\n"
            "\t\tsta_static_ip (0 = dhcp client, 1 = static ip):%i"
        :
            // Message when settings not stored
            "\t\tSettings update skipped because values did not change",
        // Arguments for settings if they are stored
        wifi_settings.ap_ssid,
        wifi_settings.ap_pwd,
        wifi_settings.ap_channel,
        wifi_settings.ap_ssid_hidden,
        wifi_settings.ap_bandwidth,
        wifi_settings.sta_only,
        wifi_settings.sta_power_save,
        wifi_settings.sta_static_ip
    );
#endif

    if (settings_stored || password_stored || ssid_stored)
        nvs_commit(handle);

    nvs_close(handle);
    return ESP_OK;
}

bool wifi_manager_fetch_wifi_sta_config()
{
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, NVS_READONLY, &handle);
    if(err != ESP_OK){
        LOGW(TAG, "No saved data for Wi-Fi STA in NVM \"%s\":\"%s\"", NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
        return false;
    }

	// allocate buffer
	const size_t blob_buff_size = sizeof(wifi_settings);
	uint8_t blob_buff[blob_buff_size];
	memset(blob_buff, 0x00, blob_buff_size);

    if(wifi_manager_config_sta == NULL){
        wifi_manager_config_sta = (wifi_config_t*)malloc(sizeof(wifi_config_t));
    }
    memset(wifi_manager_config_sta, 0x00, sizeof(wifi_config_t));
    
    // load ssid
    size_t blob_ssid_size = sizeof(wifi_manager_config_sta->sta.ssid);
    err = nvs_get_blob(handle, STORE_NVM_SSID, blob_buff, &blob_ssid_size);
    if(err != ESP_OK){
        LOGE(TAG,"Failed read WiFi %s from NVM \"%s\":\"%s\"", STORE_NVM_SSID, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
		free(wifi_manager_config_sta);
		wifi_manager_config_sta = NULL;
        nvs_close(handle);
        return false;
    }
    memcpy(wifi_manager_config_sta->sta.ssid, blob_buff, blob_ssid_size);

    // Load password
    size_t blob_psw_size = sizeof(wifi_manager_config_sta->sta.password);
    err = nvs_get_blob(handle, STORE_NVM_PSW, blob_buff, &blob_psw_size);
    if(err != ESP_OK){
        LOGE(TAG,"Failed read WiFi %s from NVM \"%s\":\"%s\"", STORE_NVM_PSW, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
		free(wifi_manager_config_sta);
		wifi_manager_config_sta = NULL;
        nvs_close(handle);
        return false;
    }
    memcpy(wifi_manager_config_sta->sta.password, blob_buff, blob_psw_size);

    // Load settings
    size_t blob_setting_size = sizeof(wifi_settings);
    err = nvs_get_blob(handle, STORE_NVM_SETTINGS, blob_buff, &blob_setting_size);
    if(err != ESP_OK){
        LOGE(TAG,"Failed read WiFi %s from NVM \"%s\":\"%s\"", STORE_NVM_SETTINGS, NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
		free(wifi_manager_config_sta);
		wifi_manager_config_sta = NULL;
        nvs_close(handle);
        return false;
    }
    memcpy(&wifi_settings, blob_buff, blob_setting_size);

#ifdef DIAGNOSTIC_VERSION
    ESP_LOGI(TAG,
        "\t\tssid:%s password:%s\n"
        "\t\tSoftAP_ssid:%s\n"
        "\t\tSoftAP_pwd:%s\n"
        "\t\tSoftAP_channel:%i\n"
        "\t\tSoftAP_hidden (1 = yes):%i\n"
        "\t\tSoftAP_bandwidth (1 = 20MHz, 2 = 40MHz):%i\n"
        "\t\tsta_only (0 = APSTA, 1 = STA when connected):%i\n"
        "\t\tsta_power_save (1 = yes):%i\n"
        "\t\tsta_static_ip (0 = dhcp client, 1 = static ip):%i",
        wifi_manager_config_sta->sta.ssid,
        wifi_manager_config_sta->sta.password,
        wifi_settings.ap_ssid,
        wifi_settings.ap_pwd,
        wifi_settings.ap_channel,
        wifi_settings.ap_ssid_hidden,
        wifi_settings.ap_bandwidth,
        wifi_settings.sta_only,
        wifi_settings.sta_power_save,
        wifi_settings.sta_static_ip
    );
#endif

    nvs_close(handle);
    if (wifi_manager_config_sta && wifi_manager_config_sta->sta.ssid[0] != '\0')
        return true;
    return false;
}

void wifi_manager(void * pvParameters)
{
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

	// event handler for the connection
    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;
    ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,&instance_wifi_event),
                "Failed registered listener to WIFI_EVENT");
    ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL,&instance_ip_event),
                "Failed registered listener to IP_EVENT");

	// SoftAP - Wifi Access Point configuration setup
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
	ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA),
                "Set Mode APSTA failed");
	ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config),
                "Set Config failed");
	ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, wifi_settings.ap_bandwidth),
                "Set Bandwidth failed");
	ERROR_CHECK(esp_wifi_set_ps(wifi_settings.sta_power_save),
                "Set PS failed");

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

    // Create wifi manager message queue
    wifi_manager_queue = xQueueCreate( 3, sizeof( queue_message) );

	// enqueue first event: load previous config
	wifi_manager_send_message(WM_ORDER_LOAD_AND_RESTORE_STA, NULL);
    
    // main wifi_manager loop
    queue_message msg;
	for(;;){
        BaseType_t xStatus = xQueueReceive( wifi_manager_queue, &msg, portMAX_DELAY );
        if(xStatus != pdPASS)
            continue;
        LOGI(TAG, "MESSAGE: %s", message_code_to_str(msg.code));
        switch(msg.code)
        {
            case WM_ORDER_LOAD_AND_RESTORE_STA:
                if(wifi_manager_fetch_wifi_sta_config()){
                    ESP_LOGI(TAG, "Saved wifi found on startup. Will attempt to connect.");
                    wifi_manager_send_message(WM_ORDER_CONNECT_STA, (void*)CONNECTION_REQUEST_RESTORE_CONNECTION);
                }
                else{
                    // no wifi saved: start soft AP! This is what should happen during a first run
                    ESP_LOGI(TAG, "No saved wifi found on startup. Starting access point.");
                    wifi_manager_send_message(WM_ORDER_START_AP, NULL);
                }
                // callback
				if(cb_ptr_arr[msg.code]) (*cb_ptr_arr[msg.code])(NULL);
                break;
            case WM_ORDER_START_AP:
                esp_err_t err = esp_wifi_set_mode(WIFI_MODE_APSTA);
                if (err == ESP_OK)
                {
                    // Start DNS server to have Captive Portal
                    dns_server_start();
                    // Start HTTP server with admin page
                    http_server_start();
                    ESP_LOGI(TAG, "WiFi mode set to APSTA successfully (SoftAP started)");
                }
                else
                    ESP_LOGE(TAG, "Failed to set WiFi mode to APSTA (error=0x%x)", err);

                /* callback */
                if(cb_ptr_arr[msg.code]) (*cb_ptr_arr[msg.code])(NULL);
                break;
            case WM_ORDER_STOP_AP:
                http_server_stop();
                dns_server_stop();
                /* callback */
                if(cb_ptr_arr[msg.code]) (*cb_ptr_arr[msg.code])(NULL);
                break;
            default:
				break;                
        }
    } // end of for loop
    vTaskDelete( NULL );
}

void wifi_manager_start(void)
{
    // disable default wifi logging
    disable_esp_log("wifi");
    LOGI(TAG, "Start manager");

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

    // free current STA config
	if(wifi_manager_config_sta){
		free(wifi_manager_config_sta);
		wifi_manager_config_sta = NULL;
	}    
}

esp_netif_t* wifi_manager_get_esp_netif_ap()
{
	return esp_netif_ap;
}

esp_netif_t* wifi_manager_get_esp_netif_sta()
{
	return esp_netif_sta;
}