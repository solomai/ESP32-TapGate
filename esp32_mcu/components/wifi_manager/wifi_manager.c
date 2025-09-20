#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "wifi_manager.h"
#include "constants.h"
#include "logs.h"

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

bool wifi_manager_fetch_wifi_sta_config()
{
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, NVS_READONLY, &handle);
    if(err != ESP_OK){
        LOGE(TAG, "Failed open NVM \"%s\":\"%s\"", NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
        return false;
    }
    return false;
}

void wifi_manager( void * pvParameters )
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
}
