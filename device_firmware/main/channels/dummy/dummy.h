/**
 * dummy channel implementation.
 */

#pragma once
#include "sdkconfig.h"
// Enable or Disable DUMMY channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_DUMMY

#include <string_view>

#include "../channels.h"
#include "dummy_config.h"

namespace channels 
{
class DUMMY; // Forward declaration to have template specialization before class definition
// Map ChannelType::DUMMYChannel to DUMMY class. Used to automatically instantiate channels in ChannelRouter.
template<> struct MapType<ChannelType::DUMMYChannel> { 
    using type = DUMMY; 
    static constexpr std::string_view name = "DUMMY";
};
class DUMMY : public ChannelBase<DUMMYConfig>
{
    public:
        DUMMY();
        ~DUMMY() = default;
    
    public:
        // lifecycle methods
        bool Start() override;

        void Stop() override;

        // data sending method
        bool Send(std::span<const std::uint8_t> data) override;

    protected:
        // Triggered when configuration is set by SetConfig
        void OnSetConfig(const DUMMYConfig& config) override;
};

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_DUMMY