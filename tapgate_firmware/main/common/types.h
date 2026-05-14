#pragma once

#include <array>
#include <string_view>
#include <cstdint>

// Types
using uuid_t = std::array<uint8_t, 16>;

// Constants
static constexpr std::size_t DEVICE_NAME_CAPACITY = 32;

// Other
#define STR_IMPL(x) #x
#define STR(x) STR_IMPL(x)