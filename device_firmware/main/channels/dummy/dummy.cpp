#include "dummy.h"
// Enable or Disable DUMMY channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_DUMMY

#include "esp_log.h"

namespace channels {

static const char* TAG = "Ch::DUMMY";

DUMMY::DUMMY()
    : ChannelBase(ChannelType::DUMMYChannel)
{
    // Channel Objects will be constructed before main app_main starts.
    // Constructor will not perform any heavy operations and access to NVS and hardware
}

bool DUMMY::Start()
{
    ESP_LOGI(TAG, "Start called");
    // Start BLE channel implementation
    return false;
}

void DUMMY::Stop()
{
    if (GetStatus() < Status::Connecting) {
        // If not connected or connecting, no need to stop
        return;
    }

    ESP_LOGI(TAG, "Stop called");
    // Stop BLE channel implementation
}

bool DUMMY::Send(std::span<const std::uint8_t> data)
{
    ESP_LOGI(TAG, "Send called with data size: %zu", data.size());
    // Send data over BLE channel implementation
    return false;
}

void DUMMY::OnSetConfig(const DUMMYConfig& config)
{
    ESP_LOGI(TAG, "OnSetConfig called");
    // Handle DUMMY-specific configuration here
    config_ = config;
}

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_DUMMY