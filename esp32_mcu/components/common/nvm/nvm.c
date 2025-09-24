#include "nvm.h"
#include "nvm_partition.h"

#include "logs.h"
#include "nvs_flash.h"
#include "nvs.h"

#include <stddef.h>

static const char *TAG_NVM = "NVM";

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

esp_err_t nvm_open_partition_handle(const char *partition_label,
                                    const char *name_space,
                                    nvs_open_mode mode,
                                    nvs_handle_t *handle)
{
    if (!partition_label || !name_space || !handle)
        return ESP_ERR_INVALID_ARG;

    return nvs_open_from_partition(partition_label, name_space, mode, handle);
}

esp_err_t nvm_read_string_from_partition(const char *partition_label,
                                         const char *name_space,
                                         const char *key,
                                         char *buffer,
                                         size_t size)
{
    if (!key || !buffer || size == 0)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvm_open_partition_handle(partition_label, name_space, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        buffer[0] = '\0';
        return err;
    }

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

esp_err_t nvm_write_string_to_partition(const char *partition_label,
                                        const char *name_space,
                                        const char *key,
                                        const char *value)
{
    if (!key || !value)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvm_open_partition_handle(partition_label, name_space, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_str(handle, key, value);
    if (err == ESP_OK)
        err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t nvm_read_u32_from_partition(const char *partition_label,
                                      const char *name_space,
                                      const char *key,
                                      uint32_t *value)
{
    if (!key || !value)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvm_open_partition_handle(partition_label, name_space, NVS_READONLY, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_get_u32(handle, key, value);
    nvs_close(handle);
    return err;
}

esp_err_t nvm_write_u32_to_partition(const char *partition_label,
                                     const char *name_space,
                                     const char *key,
                                     uint32_t value)
{
    if (!key)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvm_open_partition_handle(partition_label, name_space, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_u32(handle, key, value);
    if (err == ESP_OK)
        err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}
