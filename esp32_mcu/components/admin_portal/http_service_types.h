#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef ESP_PLATFORM
#include "wifi_manager.h"
#else
#ifndef MAX_SSID_SIZE
#define MAX_SSID_SIZE 32
#endif
#ifndef MAX_PASSWORD_SIZE
#define MAX_PASSWORD_SIZE 64
#endif
#ifndef WPA2_MINIMUM_PASSWORD_LENGTH
#define WPA2_MINIMUM_PASSWORD_LENGTH 8
#endif
#endif

typedef struct {
    char ssid[MAX_SSID_SIZE];
    char password[MAX_PASSWORD_SIZE];
} http_service_credentials_t;

typedef struct {
    http_service_credentials_t credentials;
    bool cancel_enabled;
} http_service_ap_state_t;
