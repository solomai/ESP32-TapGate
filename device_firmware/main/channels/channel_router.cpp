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

} // namespace channels