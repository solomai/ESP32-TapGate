#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_ERR_NVS_BASE             0x1100
#define ESP_ERR_NVS_NOT_INITIALIZED (ESP_ERR_NVS_BASE + 0x0)
#define ESP_ERR_NVS_PART_NOT_FOUND  (ESP_ERR_NVS_BASE + 0x3)
#define ESP_ERR_NVS_NEW_VERSION_FOUND (ESP_ERR_NVS_BASE + 0x10)
#define ESP_ERR_NVS_NO_FREE_PAGES   (ESP_ERR_NVS_BASE + 0x11)
#define ESP_ERR_NVS_INVALID_LENGTH  (ESP_ERR_NVS_BASE + 0x12)
#define ESP_ERR_NVS_NOT_FOUND       (ESP_ERR_NVS_BASE + 0x13)

esp_err_t nvs_flash_init_partition(const char *partition_name);
esp_err_t nvs_flash_erase_partition(const char *partition_name);

#ifdef __cplusplus
}
#endif

