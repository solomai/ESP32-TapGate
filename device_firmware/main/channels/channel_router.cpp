#include "channel_router.h"
#include "esp_log.h"

static const char* TAG = "ChannelRouter";

namespace channels
{

ChannelRouter& ChannelRouter::getInstance() noexcept
{
    static ChannelRouter instance;
    return instance;
}

void ChannelRouter::RestoreConfigs()
{
    ESP_LOGI(TAG, "Restoring channel configurations from NVS");
    for (size_t i = 0; i < all_channels.size(); ++i) {
        auto& channel = all_channels[i];
        const auto res = channel->RestoreConfig();
        if (res != ESP_OK) {
            ESP_LOGW(TAG, "Failed to restore config for channel %s, error: %s (0x%x). Default config will be used.",
                toString(channel->GetType()).data(), esp_err_to_name(res), res);
        }
        else {
            ESP_LOGI(TAG, "Restored config for channel %s",
                toString(channel->GetType()).data());
        }
    }
}

} // namespace channels