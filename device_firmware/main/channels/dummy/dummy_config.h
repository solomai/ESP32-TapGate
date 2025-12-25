#pragma once
#include "sdkconfig.h"
// Enable or Disable DUMMY channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_DUMMY

#include "../channels.h"

namespace channels {

class DUMMYConfig : public IChannelConfig
{
    public:
        DUMMYConfig() = default;
        ~DUMMYConfig() override = default;
        esp_err_t LoadFromNVS() override;

    // Add DUMMY-specific configuration parameters here

};

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_DUMMY