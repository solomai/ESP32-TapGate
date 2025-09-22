#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "wifi_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ssid[MAX_SSID_SIZE + 1];
    char password[MAX_PASSWORD_SIZE + 1];
} http_service_credentials_t;

esp_err_t http_service_facade_load_ap_credentials(http_service_credentials_t *credentials, bool *has_valid_password);
esp_err_t http_service_facade_store_ap_credentials(const http_service_credentials_t *credentials);
bool http_service_facade_is_ssid_valid(const char *ssid);
bool http_service_facade_is_password_valid(const char *password);

#ifdef __cplusplus
}
#endif

