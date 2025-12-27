#include "nvm.h"
#include "nvm_partition.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

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
        ESP_LOGW(TAG, "Erasing Partition \"%s\" due to previous error: %s", partition_label,
                 esp_err_to_name(err));

        err = nvs_flash_erase_partition(partition_label);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to erase Partition \"%s\": %s", partition_label,
                     esp_err_to_name(err));
            return err;
        }

        err = nvs_flash_init_partition(partition_label);
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize Partition \"%s\": %s", partition_label,
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Partition \"%s\" is ready", partition_label);
    return ESP_OK;
}