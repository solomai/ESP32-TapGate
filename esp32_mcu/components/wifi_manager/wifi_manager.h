// This code is based on wifi_manager.
// @see https://github.com/tonyp7/esp32-wifi-manager

#pragma once
#include "nvm\nvm_partition.h"
#include "esp_wifi_types.h"
#include "esp_netif.h"

// NVM Partition to store wifi data and credential
#define NVM_WIFI_PARTITION NVM_PARTITION_DEFAULT

// NVM WiFi Manager namespace
#define NVM_WIFI_NAMESPACE "wifimanager"

// Defines the task priority of the wifi_manager
#define WIFI_MANAGER_TASK_PRIORITY			3

// Defines access point's maximum number of clients. Default: 4
#define DEFAULT_AP_MAX_CONNECTIONS		 	4

// Defines access point's beacon interval. 100ms is the recommended default.
#define DEFAULT_AP_BEACON_INTERVAL 			100

// defines the minimum length of an access point password running on WPA2
#define WPA2_MINIMUM_PASSWORD_LENGTH		8

// Defines the maximum size of a SSID name. Default: 32 is IEEE standard.
#define MAX_SSID_SIZE						32

// Defines the maximum size of a WPA2 passkey. Default: 64 is IEEE standard.
#define MAX_PASSWORD_SIZE					64

// Defines the maximum number of access points that can be scanned.
#define MAX_AP_NUM 							15

// Defines the access point's default IP address. Default: "10.10.0.1"
#define DEFAULT_AP_IP						"10.10.0.1"

// Defines the access point's gateway. This should be the same as your IP. Default: "10.10.0.1"
#define DEFAULT_AP_GATEWAY					"10.10.0.1"

// Defines the access point's netmask. Default: "255.255.255.0"
#define DEFAULT_AP_NETMASK					"255.255.255.0"

// SSID (network name) the the esp32 will broadcast.
#define DEFAULT_AP_SSID 					"TapGate AP"

// Password used for the Access Point. Leave empty and set AUTH MODE to WIFI_AUTH_OPEN for no password.
#define DEFAULT_AP_PASSWORD                 ""

// Access Point WiFi Channel. Be careful you might not see the access point if you use a channel not allowed in your country.
#define DEFAULT_AP_CHANNEL                  1

// Defines visibility of the access point. 0: visible AP. 1: hidden
#define DEFAULT_AP_SSID_HIDDEN 				0

// Defines access point's bandwidth.
// Value: WIFI_BW_HT20 for 20 MHz  or  WIFI_BW_HT40 for 40 MHz
//  20 MHz minimize channel interference but is not suitable for applications with high data speeds
#define DEFAULT_AP_BANDWIDTH 				WIFI_BW_HT20

// Defines if esp32 shall run both AP + STA when connected to another AP.
//  Value: 0 will have the own AP always on (APSTA mode)
//  Value: 1 will turn off own AP when connected to another AP (STA only mode when connected)
//  Turning off own AP when connected to another AP minimize channel interference and increase throughput
#define DEFAULT_STA_ONLY 					1

// Defines if wifi power save shall be enabled.
// Value: WIFI_PS_NONE for full power (wifi modem always on)
// Value: WIFI_PS_MODEM for power save (wifi modem sleep periodically)
// Note: Power save is only effective when in STA only mode
#define DEFAULT_STA_POWER_SAVE 				WIFI_PS_NONE

#define STORE_NVM_SSID 						"ssid"
#define STORE_NVM_PSW                       "password"
#define STORE_NVM_SETTINGS                  "settings"

// The actual WiFi settings in use
struct wifi_settings_t{
	uint8_t ap_ssid[MAX_SSID_SIZE];
	uint8_t ap_pwd[MAX_PASSWORD_SIZE];
	uint8_t ap_channel;
	uint8_t ap_ssid_hidden;
	wifi_bandwidth_t ap_bandwidth;
	bool sta_only;
	wifi_ps_type_t sta_power_save;
	bool sta_static_ip;
	esp_netif_ip_info_t sta_static_ip_config;
};
extern struct wifi_settings_t wifi_settings;

// message_code_t
#define MESSAGE_CODES                \
    X(NONE)                          \
    X(WM_ORDER_LOAD_AND_RESTORE_STA) \
    X(WM_ORDER_START_AP)             \
	X(WM_ORDER_STOP_AP)              \
    X(WM_ORDER_START_HTTP_SERVER)    \
    X(WM_ORDER_STOP_HTTP_SERVER)     \
    X(WM_ORDER_START_DNS_SERVICE)    \
    X(WM_ORDER_STOP_DNS_SERVICE)     \
    X(WM_ORDER_START_WIFI_SCAN)      \
    X(WM_ORDER_CONNECT_STA)          \
    X(WM_ORDER_DISCONNECT_STA)       \
    X(WM_EVENT_STA_DISCONNECTED)     \
    X(WM_EVENT_SCAN_DONE)            \
    X(WM_EVENT_STA_GOT_IP)           \
    X(WM_MESSAGE_CODE_COUNT)

typedef enum {
#define X(name) name,
    MESSAGE_CODES
#undef X
} message_code_t;

static inline const char* message_code_to_str(message_code_t code) {
    switch(code) {
#define X(name) case name: return #name;
        MESSAGE_CODES
#undef X
        default: return "UNKNOWN";
    }
} // message_code_to_str

// connection_request_made_by_code_t
#define CONNECTION_REQUEST                    \
    X(CONNECTION_REQUEST_NONE)                \
    X(CONNECTION_REQUEST_USER)                \
	X(CONNECTION_REQUEST_AUTO_RECONNECT)      \
    X(CONNECTION_REQUEST_RESTORE_CONNECTION)  \
    X(CONNECTION_REQUEST_MAX)

typedef enum : int32_t {
#define X(name) name,
	CONNECTION_REQUEST
#undef X
} connection_request_made_by_code_t;

static inline const char* connection_request_to_str(connection_request_made_by_code_t code) {
    switch(code) {
#define X(name) case name: return #name;
	CONNECTION_REQUEST
#undef X
        default: return "UNKNOWN";
    }
} // connection_request_to_str


typedef struct{
	message_code_t code;
	void *param;
} queue_message;


// Start WiFi Manager
void wifi_manager_start(void);

// Stop WiFi Manager
void wifi_manager_stop(void);

// Register a callback to a custom function when specific event message_code happens.
esp_err_t wifi_manager_set_callback(message_code_t message_code, void (*func_ptr)(void*) );

// returns the current esp_netif object for the STAtion
esp_netif_t* wifi_manager_get_esp_netif_sta();

// returns the current esp_netif object for the Access Point
esp_netif_t* wifi_manager_get_esp_netif_ap();