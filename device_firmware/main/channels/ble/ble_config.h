#pragma once

#include "../channels.h"

namespace channels {

class BLEConfig : public IChannelConfig
{
    public:
        BLEConfig() = default;
        ~BLEConfig() override = default;

        esp_err_t LoadFromNVS() override;

    // Add BLE-specific configuration parameters here
    private:
        uint16_t mtu = 256;
};

} // namespace channels