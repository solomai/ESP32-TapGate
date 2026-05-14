#include "nvm.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <vector>
#include <cstring>

static const char* TAG = "NVM";

NVM& NVM::getInstance() noexcept
{
    static NVM instance;
    return instance;
}

esp_err_t NVM::Init() noexcept
{
    ESP_LOGI(TAG, "Initializing %d partition(s)", sizeof(NVM_PARTITION_LABELS) / sizeof(NVM_PARTITION_LABELS[0]));
    for (size_t idx = 0; idx < sizeof(NVM_PARTITION_LABELS) / sizeof(NVM_PARTITION_LABELS[0]); ++idx)
    {
        esp_err_t err = EnsurePartitionReady(NVM_PARTITION_LABELS[idx]);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t NVM::EnsurePartitionReady(const char *partition_label)
{
    if (!partition_label)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = nvs_flash_init_partition(partition_label);

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "Erasing Partition \"%s\" due to previous error: " ERR_FORMAT, partition_label,
                 esp_err_to_str(err), err);

        err = nvs_flash_erase_partition(partition_label);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to erase Partition \"%s\": " ERR_FORMAT, partition_label,
                     esp_err_to_str(err), err);
            return err;
        }

        err = nvs_flash_init_partition(partition_label);
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize Partition \"%s\": " ERR_FORMAT, partition_label,
                 esp_err_to_str(err), err);
        return err;
    }

    ESP_LOGI(TAG, "Partition \"%s\" is ready", partition_label);
    return ESP_OK;
}

esp_err_t NVM::ReadString(const char *partition,
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

esp_err_t NVM::WriteString(const char *partition,
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

    // To reduce memory cell degradation, data is rewritten only when changes have occurred
    size_t existing_len = 0;
    err = nvs_get_str(handle, key, nullptr, &existing_len);
    if (err == ESP_OK && existing_len > 0) {
        // existing_len includes the null terminator
        std::vector<char> buffer(existing_len);
        err = nvs_get_str(handle, key, buffer.data(), &existing_len);
        if (err == ESP_OK && std::strcmp(buffer.data(), value) == 0) {
            nvs_close(handle);
            ESP_LOGD(TAG, "%s the value \"%s\" already set for part: %s space: %s key %s", __FUNCTION__, value, partition, namespace_name, key);
            return ESP_OK; // No change needed
        }
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Key not present, proceed to set
    } else if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    err = nvs_set_str(handle, key, value);
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "%s Store: \"%s\" to part: %s space: %s key %s", __FUNCTION__, value, partition, namespace_name, key);
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t NVM::StringSize(const char *partition,
                          const char *namespace_name,
                          const char *key,
                          size_t &size)
{
    if (!partition || !namespace_name || !key)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(partition, namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }
    size = 0;
    err = nvs_get_str(handle, key, nullptr, &size);
    nvs_close(handle);
    return err;
}

esp_err_t NVM::ReadU32(const char *partition,
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

esp_err_t NVM::WriteU32(const char *partition,
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

    // To reduce memory cell degradation, data is rewritten only when changes have occurred
    uint32_t existing_value;
    err = nvs_get_u32(handle, key, &existing_value);
    if (err == ESP_OK && existing_value == value) {
        nvs_close(handle);
        ESP_LOGD(TAG, "%s the value \"%d\" already set for part: %s space: %s key %s", __FUNCTION__, value, partition, namespace_name, key);
        return ESP_OK; // No change needed
    }

    err = nvs_set_u32(handle, key, value);
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "%s Store: \"%d\" to part: %s space: %s key %s", __FUNCTION__, value, partition, namespace_name, key);
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

esp_err_t NVM::ReadU8(const char *partition,
                      const char *namespace_name,
                      const char *key,
                      uint8_t *value)
{
    if (!partition || !namespace_name || !key || !value)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(partition, namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_get_u8(handle, key, value);
    nvs_close(handle);
    return err;
}

esp_err_t NVM::WriteU8(const char *partition,
                       const char *namespace_name,
                       const char *key,
                       uint8_t value)
{
    if (!partition || !namespace_name || !key)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open_from_partition(partition, namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    // To reduce memory cell degradation, data is rewritten only when changes have occurred
    uint8_t existing_value;
    err = nvs_get_u8(handle, key, &existing_value);
    if (err == ESP_OK && existing_value == value) {
        nvs_close(handle);
        ESP_LOGD(TAG, "%s the value \"%d\" already set for part: %s space: %s key %s", __FUNCTION__, value, partition, namespace_name, key);
        return ESP_OK; // No change needed
    }

    err = nvs_set_u8(handle, key, value);
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "%s Store: \"%d\" to part: %s space: %s key %s", __FUNCTION__, value, partition, namespace_name, key);
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}
