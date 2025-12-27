#pragma once

#include <string_view>

namespace channels {

// Channel status enumeration
// Don't change the order of these statuses as they are used for comparison
enum Status {
    Uninitialized = 0,
    Offline,
    Stopping,
    Connecting,
    Online
};

// Channel Status toString function implementation
constexpr std::string_view toString(Status status) {
    static constexpr std::string_view StatusNames[] = {
        "Uninitialized",
        "Offline",
        "Stopping",
        "Connecting",
        "Online"
    };
    const size_t index = static_cast<size_t>(status);
    if (index >= (sizeof(StatusNames) / sizeof(StatusNames[0]))) {
        return "Unknown";
    }    
    return StatusNames[index];
}

} // namespace channels