#include "ble.h"
#include "esp_log.h"

namespace channels {

static const char* TAG = "Ch::BLE";

BLE::BLE()
    : ChannelBase(ChannelType::BLEChannel)
{
    ESP_LOGI(TAG, "Constructor called");
}
BLE::~BLE()
{
    ESP_LOGI(TAG, "Destructor called");
    // Destructor implementation
}

bool BLE::Start()
{
    ESP_LOGI(TAG, "Start called");
    // Start BLE channel implementation
    return false;
}

void BLE::Stop()
{
    ESP_LOGI(TAG, "Stop called");
    // Stop BLE channel implementation
}

bool BLE::Send(std::span<const std::uint8_t> data)
{
    ESP_LOGI(TAG, "Send called with data size: %zu", data.size());
    // Send data over BLE channel implementation
    return false;
}

void BLE::OnSetConfig(const BLEConfig& config)
{
    ESP_LOGI(TAG, "OnSetConfig called");
    // Handle BLE-specific configuration here
    config_ = config;
}

} // namespace channels