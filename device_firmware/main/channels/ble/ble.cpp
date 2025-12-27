#include "ble.h"
// Enable or Disable BLE channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_BLE

#include "esp_log.h"

namespace channels {

static const char* TAG = "Ch::BLE";

BLE::BLE()
    : ChannelBase(ChannelType::BLEChannel)
{
    // Channel Objects will be constructed before main app_main starts.
    // Constructor will not perform any heavy operations and access to NVS and hardware
}

bool BLE::Start()
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
    // Start BLE channel implementation
    return false;
}

void BLE::Stop()
{
    if (GetStatus() < Status::Connecting) {
        // If not connected or connecting, no need to stop
        return;
    }

    ESP_LOG_NOTIMPLEMENTED(TAG);
    // Stop BLE channel implementation
}

bool BLE::Send(std::span<const std::uint8_t> data)
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
    // Send data over BLE channel implementation
    return false;
}

void BLE::OnSetConfig(const BLEConfig& config)
{
    ESP_LOG_NOTIMPLEMENTED(TAG);

    // Store the config if configuration is valid
    config_ = config;

    // Aply configuration changes
}

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_BLE