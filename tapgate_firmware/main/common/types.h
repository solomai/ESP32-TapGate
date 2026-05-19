#pragma once

#include <array>
#include <string_view>
#include <cstdint>

// nonce type
typedef uint32_t tg_nonce_t;

// UID type.
typedef uint8_t tg_uid_t[UID_CAP];

// Names
typedef uint8_t tg_name_t[NAME_MAX_SIZE];

// RSA keys types
typedef uint8_t tg_public_key_t[PUBKEY_CAP];
typedef uint8_t tg_private_key_t[PRVKEY_CAP];

// Stringifies a macro's expanded value. STR_IMPL does the actual stringification
// after the preprocessor has expanded x; STR forces that expansion first.
#define STR_IMPL(x) #x
#define STR(x) STR_IMPL(x)