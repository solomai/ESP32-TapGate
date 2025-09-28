#pragma once
#include "nvm\nvm_partition.h"

// NVM Partition to store wifi data and credential
#define NVM_WIFI_PARTITION NVM_PARTITION_DEFAULT

// NVM WiFi Manager namespace
#define NVM_WIFI_NAMESPACE "wifimanager"

// Stored AP SSID to connect
#define STORE_NVM_KEY_AP_SSID               "ap-ssid"

// Stored AP Password to connect
#define STORE_NVM_KEY_AP_PSW                "ap-psw"

// Stored AP channel
#define STORE_NVM_KEY_AP_CHANNEL            "ap-channel"

// Store AP SSID hidden flag
#define STORE_NVM_KEY_AP_SSID_HIDDEN        "ap-hidden"

// Store AP bandwidth
#define STORE_NVM_KEY_AP_BANDWIDTH          "ap-bandwidth"

// Stored STA SSID to connect
#define STORE_NVM_KEY_STA_SSID              "sta-ssid"

// Stored STA Password to connect
#define STORE_NVM_KEY_STA_PSW               "sta-psw"
