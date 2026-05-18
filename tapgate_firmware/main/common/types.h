#pragma once

#include <array>
#include <string_view>
#include <cstdint>

// UID type — named device_uid_t to avoid clash with POSIX uid_t (sys/types.h)
typedef uint8_t device_uid_t[UID_CAP];

// RSA keys types
typedef uint8_t public_key_t[PUBKEY_CAP];
typedef uint8_t private_key_t[PRVKEY_CAP];

// Stringifies a macro's expanded value. STR_IMPL does the actual stringification
// after the preprocessor has expanded x; STR forces that expansion first.
#define STR_IMPL(x) #x
#define STR(x) STR_IMPL(x)