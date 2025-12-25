#pragma once

#include "../channels.h"

namespace channels {

class BLEConfig : public IChannelConfig {
    public:
        BLEConfig() = default;
        ~BLEConfig() override = default;

    // Add BLE-specific configuration parameters here
};

} // namespace channels