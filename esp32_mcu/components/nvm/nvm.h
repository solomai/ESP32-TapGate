#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t nvm_init(void);
esp_err_t nvm_init_partition(const char *partition_label);

#ifdef __cplusplus
}
#endif

