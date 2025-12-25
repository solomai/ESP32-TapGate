#pragma once

#include <string_view>

namespace channels {

// Channel status enumeration
enum Status {
    Uninitialized = 0,
    Offline,
    Connecting,
    Online,
    Stopping,
    Error
};

// Channel Status toString function implementation
constexpr std::string_view toString(Status status) {
    static constexpr std::string_view StatusNames[] = {
        "Uninitialized",
        "Offline",
        "Connecting",
        "Online",
        "Stopping",
        "Error"
    };
    const size_t index = static_cast<size_t>(status);
    if (index >= (sizeof(StatusNames) / sizeof(StatusNames[0]))) {
        return "Unknown";
    }    
    return StatusNames[index];
}

} // namespace channels