#include "nvm.h"
#include "nvm_partition.h"

#include "logs.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <stddef.h>

static const char *TAG_NVM = "NVM";

#ifndef NVM_WIFI_PARTITION
#define NVM_WIFI_PARTITION NVM_PARTITION_DEFAULT
#endif

#ifndef NVM_WIFI_NAMESPACE
#define NVM_WIFI_NAMESPACE "wifimanager"
#endif

static esp_err_t ensure_partition_ready(const char *partition_label)
{
    if (!partition_label)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = nvs_flash_init_partition(partition_label);

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        LOGW(TAG_NVM, "Erasing Partition \"%s\" due to previous error: %s", partition_label,
                 esp_err_to_name(err));

        err = nvs_flash_erase_partition(partition_label);
        if (err != ESP_OK)
        {
            LOGE(TAG_NVM, "Failed to erase Partition \"%s\": %s", partition_label,
                     esp_err_to_name(err));
            return err;
        }

        err = nvs_flash_init_partition(partition_label);
    }

    if (err != ESP_OK)
    {
        LOGE(TAG_NVM, "Failed to initialize Partition \"%s\": %s", partition_label,
                 esp_err_to_name(err));
        return err;
    }

    LOGI(TAG_NVM, "Partition \"%s\" is ready", partition_label);
    return ESP_OK;
}

esp_err_t nvm_init(void)
{
    const char *partitions[] = {
        NVM_PARTITION_DEFAULT,
        NVM_PARTITION_CLIENTS,
        NVM_PARTITION_NONCE,
    };

    LOGI(TAG_NVM, "Initializing %d partition(s)", sizeof(partitions) / sizeof(partitions[0]));
    for (size_t i = 0; i < sizeof(partitions) / sizeof(partitions[0]); ++i)
    {
        esp_err_t err = ensure_partition_ready(partitions[i]);
        if (err != ESP_OK) {
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t nvm_read_string(const char *partition,
                          const char *namespace_name,
                          const char *key,
                          char *buffer,
                          size_t size)
{
    if (!buffer || size == 0)
        return ESP_ERR_INVALID_ARG;

    buffer[0] = '\0';

    if (!partition || !namespace_name || !key)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(partition, namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK)
        return err;

    size_t length = size;
    err = nvs_get_str(handle, key, buffer, &length);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        buffer[0] = '\0';
        err = ESP_OK;
    }
    nvs_close(handle);
    return err;
}

esp_err_t nvm_write_string(const char *partition,
                           const char *namespace_name,
                           const char *key,
                           const char *value)
{
    if (!partition || !namespace_name || !key || !value)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(partition, namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_str(handle, key, value);
    if (err == ESP_OK)
        err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t nvm_read_u32(const char *partition,
                       const char *namespace_name,
                       const char *key,
                       uint32_t *value)
{
    if (!partition || !namespace_name || !key || !value)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(partition, namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_get_u32(handle, key, value);
    nvs_close(handle);
    return err;
}

esp_err_t nvm_write_u32(const char *partition,
                        const char *namespace_name,
                        const char *key,
                        uint32_t value)
{
    if (!partition || !namespace_name || !key)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(partition, namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_u32(handle, key, value);
    if (err == ESP_OK)
        err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t nvm_wifi_read_string(const char *key, char *buffer, size_t size)
{
    return nvm_read_string(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, key, buffer, size);
}

esp_err_t nvm_wifi_write_string(const char *key, const char *value)
{
    return nvm_write_string(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, key, value);
}

esp_err_t nvm_wifi_read_u32(const char *key, uint32_t *value)
{
    return nvm_read_u32(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, key, value);
}

esp_err_t nvm_wifi_write_u32(const char *key, uint32_t value)
{
    return nvm_write_u32(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, key, value);
}
