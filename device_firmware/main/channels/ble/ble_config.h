#pragma once
#include "sdkconfig.h"
// Enable or Disable BLE channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_BLE

#include "../channels.h"
#include "../../common/helpers/helper_uuid.h"

namespace channels {

class BLEConfig : public IChannelConfig
{
    public:
        BLEConfig() = default;
        ~BLEConfig() override = default;

        esp_err_t LoadFromNVS() override;

    // Add BLE-specific configuration parameters here
    public:
        static constexpr auto serviceUUID = helper_uuid::parseUUID(CONFIG_TAPGATE_BLE_SERVICE_UUID);
        static constexpr auto rxCharUUID  = helper_uuid::parseUUID(CONFIG_TAPGATE_BLE_RX_UUID);
        static constexpr auto txCharUUID  = helper_uuid::parseUUID(CONFIG_TAPGATE_BLE_TX_UUID);

        // Advertising Parameters (in milliseconds)
        uint16_t advIntervalMin = 100;      ///< Min advertising interval (ms)
        uint16_t advIntervalMax = 150;      ///< Max advertising interval (ms)
        uint32_t advDuration = 0;           ///< Advertising duration (0 = indefinite)
};

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_BLE