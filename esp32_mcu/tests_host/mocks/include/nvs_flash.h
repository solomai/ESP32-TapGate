#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t nvs_flash_init(void);
esp_err_t nvs_flash_deinit(void);
esp_err_t nvs_flash_erase(void);

#ifdef __cplusplus
}
#endif
