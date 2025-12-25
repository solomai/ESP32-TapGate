#include "dummy_config.h"
// Enable or Disable DUMMY channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_DUMMY

#include "esp_log.h"

static const char* TAG = "Ch::DUMMYConfig";

namespace channels {
esp_err_t DUMMYConfig::LoadFromNVS()
{
    // TODO: Implement loading configuration from NVS
    ESP_LOGW(TAG, "LoadFromNVS called - not yet implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_DUMMY