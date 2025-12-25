#include "ble_config.h"
#include "esp_log.h"

static const char* TAG = "Ch::BLEConfig";

namespace channels {
esp_err_t BLEConfig::LoadFromNVS()
{
    // TODO: Implement loading configuration from NVS
    ESP_LOGW(TAG, "LoadFromNVS called - not yet implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

} // namespace channels