#include "dummy.h"
// Enable or Disable DUMMY channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_DUMMY

#include "common/event_journal/event_journal.h"

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
    if (GetStatus() >= Status::Connecting) {
        return false;
    }
    // Reset last esp error code on new Start attempt
    ResetLastError();

    ESP_LOG_NOTIMPLEMENTED(TAG);
    return false;
}

void DUMMY::Stop()
{
    if (GetStatus() < Status::Connecting) {
        // If not connected or connecting, no need to stop
        return;
    }

    ESP_LOG_NOTIMPLEMENTED(TAG);
}

bool DUMMY::Send(std::span<const std::uint8_t> data)
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
    return false;
}

bool DUMMY::OnConfigValidate(const DUMMYConfig& config)
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
    // Validate DUMMY-specific configuration here
    return true;
}

void DUMMY::OnSetConfig(const DUMMYConfig& config)
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
    // Handle DUMMY-specific configuration here
}

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_DUMMY