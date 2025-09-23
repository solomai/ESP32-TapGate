#pragma once

#include "esp_err.h"
#include "http_service_types.h"

esp_err_t http_service_facade_get_ap_credentials(http_service_credentials_t *credentials);

esp_err_t http_service_facade_save_ap_credentials(const http_service_credentials_t *credentials);
