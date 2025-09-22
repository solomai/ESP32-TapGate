#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Retrieve the stored Access Point credentials.
esp_err_t http_service_facade_get_ap_credentials(char *ssid, size_t ssid_len,
                                                 char *password, size_t password_len);

// Persist new Access Point credentials. Performs validation internally.
esp_err_t http_service_facade_store_ap_credentials(const char *ssid, const char *password);

// Returns true when a valid Access Point password is stored.
bool http_service_facade_has_valid_ap_password(void);

#ifdef __cplusplus
}
#endif

