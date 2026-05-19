#pragma once

#include <cstdint>

// Log tag for main app
#ifdef CONFIG_APP_DEBUG_MODE
    static constexpr char TAG_MAIN[] = "TAPGATE DEBUG";
#else
    static constexpr char TAG_MAIN[] = "TAPGATE";
#endif

// Specifies the device capacity for client enrollment
constexpr size_t CLIENTS_DB_MAX_RECORDS = 
#ifdef CONFIG_TAPGATE_MAX_CLIENTS_DB
    CONFIG_TAPGATE_MAX_CLIENTS_DB;
#else
    10;
#endif

// Display name capacity (bytes, includes '\0')
static constexpr std::size_t NAME_MAX_SIZE = 32;

// Size of Id. Up to 15 characters are allowed due to NVM key size limitations.
constexpr size_t UID_CAP = 16;

// Capacity for public key X25519 (RFC 7748)
constexpr size_t PUBKEY_CAP = 32;

// Capacity for private key X25519 (RFC 7748)
constexpr size_t PRVKEY_CAP = 32;