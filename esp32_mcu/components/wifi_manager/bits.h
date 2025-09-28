#pragma once

// indicate that the ESP32 is currently connected.
const int WIFI_MANAGER_WIFI_CONNECTED_BIT = BIT0;

// indicate that the APSTA currently connected.
const int WIFI_MANAGER_AP_STA_CONNECTED_BIT = BIT1;

// Set automatically once the SoftAP is started
const int WIFI_MANAGER_AP_STARTED_BIT = BIT2;

// When set, means a client requested to connect to an access point
const int WIFI_MANAGER_REQUEST_STA_CONNECT_BIT = BIT3;

// This bit is set automatically as soon as a connection was lost
const int WIFI_MANAGER_STA_DISCONNECT_BIT = BIT4;

// When set, means the wifi manager attempts to restore a previously saved connection at startup.
const int WIFI_MANAGER_REQUEST_RESTORE_STA_BIT = BIT5;

// When set, means a client requested to disconnect from currently connected AP
const int WIFI_MANAGER_REQUEST_WIFI_DISCONNECT_BIT = BIT6;

// When set, means a scan is in progress
const int WIFI_MANAGER_SCAN_BIT = BIT7;

// When set, means user requested for a disconnect
const int WIFI_MANAGER_REQUEST_DISCONNECT_BIT = BIT8;