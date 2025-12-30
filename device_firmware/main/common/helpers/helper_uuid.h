#pragma once

#include "types.h"

// Helper functions for UUID parsing

namespace helper_uuid {

constexpr uint8_t hexCharToByte(char c) noexcept {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    return 0;
}

constexpr uuid_t parseUUID(std::string_view s) noexcept {
    uuid_t result{};
    size_t byteIdx = 0;
    for (size_t i = 0; i < s.size() && byteIdx < 16; ++i) {
        if (s[i] == '-') [[unlikely]] {
            continue;
        }
        if (i + 1 < s.size()) {
            uint8_t high = hexCharToByte(s[i]);
            uint8_t low = hexCharToByte(s[++i]);
            result[byteIdx++] = static_cast<uint8_t>((high << 4) | low);
        }
    }
    return result;
}

} // namespace helper_uuid