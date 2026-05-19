#pragma once

#include <esp_err.h>
#include <span>
#include <string_view>

#include "constants.h"
#include "types.h"

// Buffer size for a UID hex string: 2 hex chars per byte + null terminator.
// Declare buffers as: char buf[UID_STR_LEN];
// Scales automatically with UID_CAP.
static constexpr std::size_t UID_STR_LEN = UID_CAP * 2u + 1u;

// ---------------------------------------------------------------------------
// Generation (hardware-dependent — not usable in host tests)
// ---------------------------------------------------------------------------

// Generates a hardware-seeded RFC 4122 UUIDv4 and writes it to out_uid.
// out_uid must point to a caller-owned buffer of at least UID_CAP bytes.
// Returns ESP_ERR_INVALID_ARG if out_uid is null.
//
// Precondition: Wi-Fi and Bluetooth must not be initialised at call time.
// When a radio stack is already active, esp_fill_random() already draws
// from the hardware TRNG and bootloader_random_enable() must not be called.
[[nodiscard]] esp_err_t generate_rng_uid(tg_uid_t out_uid);

// ---------------------------------------------------------------------------
// String conversion (pure — usable in host tests)
// ---------------------------------------------------------------------------

// Formats uid as a lowercase hex string (2 chars per byte, no separator) into out.
// out must hold at least UID_STR_LEN + 1 bytes.
// Returns out.data() on success, nullptr if uid is null or out is too small.
const char *uid_to_str(const tg_uid_t uid, std::span<char> out);

// Parses a lowercase or uppercase hex string of exactly UID_STR_LEN chars
// into out_uid. No separators expected.
// Returns ESP_ERR_INVALID_ARG if out_uid is null or str is malformed.
[[nodiscard]] esp_err_t str_to_uid(std::string_view str, tg_uid_t out_uid);
