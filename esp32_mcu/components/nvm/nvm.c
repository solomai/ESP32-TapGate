#include "nvm.h"
#include "nvm_partition.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include <stddef.h>

static const char *TAG = "nvm";

static esp_err_t ensure_partition_ready(const char *partition_label)
{
    if (!partition_label)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = nvs_flash_init_partition(partition_label);

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "Erasing NVS partition %s due to previous error: %s", partition_label,
                 esp_err_to_name(err));

        err = nvs_flash_erase_partition(partition_label);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to erase NVS partition %s: %s", partition_label,
                     esp_err_to_name(err));
            return err;
        }

        err = nvs_flash_init_partition(partition_label);
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize NVS partition %s: %s", partition_label,
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGD(TAG, "NVS partition %s ready", partition_label);
    return ESP_OK;
}

esp_err_t nvm_init_partition(const char *partition_label)
{
    return ensure_partition_ready(partition_label);
}

esp_err_t nvm_init(void)
{
    const char *partitions[] = {
        NVM_PARTITION_DEFAULT,
        NVM_PARTITION_CLIENTS,
        NVM_PARTITION_NONCE,
    };

    for (size_t i = 0; i < sizeof(partitions) / sizeof(partitions[0]); ++i)
    {
        esp_err_t err = ensure_partition_ready(partitions[i]);
        if (err != ESP_OK)
            return err;
    }

    return ESP_OK;
}
