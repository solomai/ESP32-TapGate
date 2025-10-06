#include "nvs.h"
#include "esp_err.h"
#include <string.h>
#include <stdlib.h>

// Simple mock implementation for NVS functions
// In a real test environment, you might want a more sophisticated mock

esp_err_t nvs_open_from_partition(const char *partition_name,
                                  const char *namespace_name,
                                  nvs_open_mode open_mode,
                                  nvs_handle_t *out_handle)
{
    (void)partition_name;
    (void)namespace_name;
    (void)open_mode;
    
    if (!out_handle) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *out_handle = 1; // Mock handle
    return ESP_OK;
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *out_value, size_t *length)
{
    (void)handle;
    (void)key;
    
    if (!length) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Mock: return not found for any key
    return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *value)
{
    (void)handle;
    (void)key;
    (void)value;
    
    // Mock: always succeed
    return ESP_OK;
}

esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out_value)
{
    (void)handle;
    (void)key;
    
    if (!out_value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Mock: return not found for any key
    return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value)
{
    (void)handle;
    (void)key;
    (void)value;
    
    // Mock: always succeed
    return ESP_OK;
}

esp_err_t nvs_get_u32(nvs_handle_t handle, const char *key, uint32_t *out_value)
{
    (void)handle;
    (void)key;
    
    if (!out_value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Mock: return not found for any key
    return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_set_u32(nvs_handle_t handle, const char *key, uint32_t value)
{
    (void)handle;
    (void)key;
    (void)value;
    
    // Mock: always succeed
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    
    // Mock: always succeed
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle)
{
    (void)handle;
    
    // Mock: do nothing
}