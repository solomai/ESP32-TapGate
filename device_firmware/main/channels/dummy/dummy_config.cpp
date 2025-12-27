#include "dummy_config.h"
// Enable or Disable DUMMY channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_DUMMY

#include "esp_log.h"

static const char* TAG = "Ch::DUMMYConfig";

namespace channels {
esp_err_t DUMMYConfig::LoadFromNVS()
{
    // TODO: Implement loading configuration from NVS
    ESP_LOG_NOTIMPLEMENTED(TAG);
    return ESP_ERR_DEV_NOT_IMPLEMENTED;
}

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_DUMMY