#pragma once

#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

static inline esp_err_t nvm_wifi_read_string(const char *key, char *buffer, size_t size)
{
    (void)key;
    if (buffer && size)
        buffer[0] = '\0';
    return ESP_OK;
}

static inline esp_err_t nvm_wifi_write_string(const char *key, const char *value)
{
    (void)key;
    (void)value;
    return ESP_OK;
}

static inline esp_err_t nvm_wifi_read_u32(const char *key, uint32_t *value)
{
    (void)key;
    if (value)
        *value = 0;
    return ESP_OK;
}

static inline esp_err_t nvm_wifi_write_u32(const char *key, uint32_t value)
{
    (void)key;
    (void)value;
    return ESP_OK;
}
