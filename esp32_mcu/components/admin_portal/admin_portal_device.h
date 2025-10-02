#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

bool wifi_get_ap_ssid(char *ssid, size_t size);

void wifi_set_ap_ssid(const char *ssid);

bool wifi_get_ap_password(char *password, size_t size);

void wifi_set_ap_password(const char *password);

uint32_t session_get_idle_timeout();

void session_set_idle_timeout(uint32_t timeout_value);

void trigger_scan_wifi();

// Create JSON string from WiFi scan results
char* create_json_from_ap_records();