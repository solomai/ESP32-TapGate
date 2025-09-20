#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_manager.h"
#include "constants.h"
#include "esp_log.h"
#include "logs.h"

// ESP-IDF networking stack
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

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

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT)
    {
		switch(event_id)
        {
            case WIFI_EVENT_WIFI_READY:
                break;
            case WIFI_EVENT_SCAN_DONE:
                break;
            case WIFI_EVENT_STA_START:
                break;
            case WIFI_EVENT_STA_STOP:
                break;
            case WIFI_EVENT_STA_CONNECTED:
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                break;
            case WIFI_EVENT_STA_AUTHMODE_CHANGE:
                break;
            case WIFI_EVENT_AP_START:
                break;
            case WIFI_EVENT_AP_STOP:
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                break;
            case WIFI_EVENT_AP_PROBEREQRECVED:
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
                break;
            case IP_EVENT_GOT_IP6:
                break;
            case IP_EVENT_STA_LOST_IP:
                break;
            default:
                break;
        }
    }
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

	/* event handler for the connection */
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

    // main wifi_manager loop
	for(;;){

        // TODO: REMOVE DELAY
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
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
}
