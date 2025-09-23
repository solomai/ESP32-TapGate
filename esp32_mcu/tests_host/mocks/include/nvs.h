#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *nvs_handle_t;

typedef enum {
    NVS_READONLY = 0,
    NVS_READWRITE = 1,
} nvs_open_mode_t;

esp_err_t nvs_open_from_partition(const char *partition_name,
                                  const char *name_space,
                                  nvs_open_mode_t open_mode,
                                  nvs_handle_t *out_handle);

esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *value);

esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *out_value, size_t *length);

esp_err_t nvs_get_u32(nvs_handle_t handle, const char *key, uint32_t *out_value);

esp_err_t nvs_set_u32(nvs_handle_t handle, const char *key, uint32_t value);

esp_err_t nvs_commit(nvs_handle_t handle);

void nvs_close(nvs_handle_t handle);

#ifdef __cplusplus
}
#endif

