/**
 * BLE channel implementation.
 */

#pragma once
#include "sdkconfig.h"
// Enable or Disable BLE channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_BLE

#include <string_view>

#include "../channels.h"
#include "ble_config.h"

namespace channels 
{
class BLE; // Forward declaration to have template specialization before class definition
// Map ChannelType::BLEChannel to BLE class. Used to automatically instantiate channels in ChannelRouter.
template<> struct MapType<ChannelType::BLEChannel> { 
    using type = BLE; 
    static constexpr std::string_view name = "BLE";
};
class BLE : public ChannelBase<BLEConfig>
{
    public:
        BLE();
        ~BLE() = default;

    public:
        // lifecycle methods
        bool Start() override;

        void Stop() override;

        // data sending method
        bool Send(std::span<const std::uint8_t> data) override;

    protected:
        // Triggered when configuration is set by SetConfig
        void OnSetConfig(const BLEConfig& config) override;
};

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_BLE