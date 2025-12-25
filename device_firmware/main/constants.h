#pragma once

// Specifies the device capacity for client enrollment
constexpr size_t CLIENTS_DB_MAX_RECORDS = 
#ifdef CONFIG_TAPGATE_MAX_CLIENTS_DB
    CONFIG_TAPGATE_MAX_CLIENTS_DB;
#else
    10;
#endif

// Display name capacity (bytes, includes '\0')
constexpr size_t NAME_MAX_SIZE = 32;

// Size of Id. Up to 15 characters are allowed due to NVM key size limitations.
constexpr size_t UID_CAP = 15;

// Capacity for public key X25519 (RFC 7748)
constexpr size_t PUBKEY_CAP = 32;

// Capacity for private key X25519 (RFC 7748)
constexpr size_t PRVKEY_CAP = 32;