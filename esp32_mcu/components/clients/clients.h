#pragma once
#include "types.h"
#include "constants.h"

// ---- Config ----
// Maximum number of supported clients/users
#define CLIENTS_DB_MAX_RECORDS 50

// Display name capacity (bytes, includes '\0')
#define CLIENTS_NAME_MAX       32

// Capacity for RSA public key in PEM ( BASE64 zero-base string )
#define CLIENTS_PUBPEM_CAP     512

// NVM namespace. 15 characters are allowed due to NVM key size limitations.
#define CLIENTS_DB_NAMESPACE   "clients"

// Status codes
typedef enum {
    CLIENTS_OK = OPP_OK,
    CLIENTS_ERR_NO_INIT,
    CLIENTS_ERR_STORAGE,
    CLIENTS_ERR_FULL,
    CLIENTS_ERR_NOT_FOUND,
    CLIENTS_ERR_INVALID_ARG,
    CLIENTS_ERR_EXISTS,
    CLIENTS_ERR_TOO_LONG,
} clients_status_t;

// ---- TYPES ----
// structure representing complete client information
#if defined(_MSC_VER)
#define CLIENTS_PACKED
#else
#define CLIENTS_PACKED __attribute__((packed))
#endif

typedef struct CLIENTS_PACKED {
    uint8_t      allow_flags;
    nonce_t      nonce;
    client_uid_t client_id;
    char         name[CLIENTS_NAME_MAX];
    char         pub_pem[CLIENTS_PUBPEM_CAP];
} client_t;

#undef CLIENTS_PACKED


// ---- API ----
