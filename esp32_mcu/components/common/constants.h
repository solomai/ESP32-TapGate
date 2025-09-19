#pragma once

// Enables diagnostic output during runtime for debug purposes
#ifndef DIAGNOSTIC_VERSION
#define DIAGNOSTIC_VERSION
#endif

// ---- Config ----
// Maximum number of supported clients/users
#ifndef CLIENTS_DB_MAX_RECORDS
#define CLIENTS_DB_MAX_RECORDS 50
#endif

// Display name capacity (bytes, includes '\0')
#ifndef NAME_MAX_SZIE
#define NAME_MAX_SIZE          32
#endif

// Size of Id. Up to 15 characters are allowed due to NVM key size limitations.
#ifndef UID_CAP
#define UID_CAP                15
#endif

// Capacity for public key X25519 (RFC 7748)
#ifndef PUBPEM_CAP
#define PUBPEM_CAP             32
#endif

// Capacity for private key X25519 (RFC 7748)
#ifndef PRVPEM_CAP
#define PRVPEM_CAP             32
#endif

// NVM partitions name
