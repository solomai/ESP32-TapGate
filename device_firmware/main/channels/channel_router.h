#pragma once

#include <array>
#include <utility>
#include <string_view>

// Channel interfaces
#include "channels.h"
// Channels
#include "ble/ble.h"

namespace channels {

    // Compile-time channel names table
    namespace detail {
        // Compile-time generation of channel names array
        template<size_t... Is>
        static constexpr std::array<std::string_view, sizeof...(Is)> make_name_table(std::index_sequence<Is...>) {
            return { MapType<static_cast<ChannelType>(Is)>::name... };
        }

        static constexpr auto ChannelNames = make_name_table(
            std::make_index_sequence<static_cast<size_t>(ChannelType::_Count)>{}
        );
    } // namespace detail

    // ChannelType toString function implementation
    constexpr std::string_view toString(ChannelType type) {
        size_t index = static_cast<size_t>(type);
        if (index >= detail::ChannelNames.size()) {
            return "Unknown";
        }
        return detail::ChannelNames[index];
    }

class ChannelRouter {
public:
    ChannelRouter() = default;
    virtual ~ChannelRouter() = default;

    static ChannelRouter& getInstance() noexcept;

    // Delete copy constructor and assignment operator
    ChannelRouter(const ChannelRouter&) = delete;
    ChannelRouter& operator=(const ChannelRouter&) = delete;

    // Delete move constructor and assignment operator
    ChannelRouter(ChannelRouter&&) = delete;
    ChannelRouter& operator=(ChannelRouter&&) = delete;

public:
    // Access channel by type
    static IChannel& get(ChannelType type) {
        return *all_channels[static_cast<size_t>(type)];
    }

    // Access channel by mapped type
    template<typename T>
    static T& get() {
        return get_instance<T>();
    }

    // Get Channel count
    static constexpr size_t getChannelCount() {
        return all_channels.size();
    }

private:
    // Instantiate static channels
    template<typename T>
    static T& get_instance() {
        static T instance;
        return instance;
    }

    // Dispatch table of all channels by Enum ChannelType
    template<size_t... Is>
    static constexpr std::array<IChannel*, sizeof...(Is)> make_dispatch_table(std::index_sequence<Is...>) {
        return { (&get_instance<typename MapType<static_cast<ChannelType>(Is)>::type>())... };
    }

    // All channels container
    static inline std::array<IChannel*, static_cast<size_t>(ChannelType::_Count)> all_channels = 
        make_dispatch_table(std::make_index_sequence<static_cast<size_t>(ChannelType::_Count)>{});    
}; // class ChannelRouter

} // namespace channels