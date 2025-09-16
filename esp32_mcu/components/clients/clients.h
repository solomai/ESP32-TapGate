#pragma once

// include components
#include "types.h"
#include "constants.h"

// ---- Config ----
// Maximum number of supported clients/users
#ifndef CLIENTS_DB_MAX_RECORDS
#define CLIENTS_DB_MAX_RECORDS 50
#endif

// Display name capacity (bytes, includes '\0')
#ifndef CLIENTS_NAME_MAX
#define CLIENTS_NAME_MAX       32
#endif

// Capacity for RSA public key in PEM (must include '\0')
#ifndef CLIENTS_PUBPEM_CAP
#define CLIENTS_PUBPEM_CAP     512
#endif

// NVM namespace. 15 characters are allowed due to NVM key size limitations.
#ifndef CLIENTS_DB_NAMESPACE
#define CLIENTS_DB_NAMESPACE   "clients"
#endif

// Status codes
typedef enum {
    CLIENTS_OK = 0,
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
typedef struct
{
    uint8_t      allow_flags;
    nonce_t      nonce;
    client_uid_t client_id;
    char         name[CLIENTS_NAME_MAX];
    char         pub_pem[CLIENTS_PUBPEM_CAP];
} client_t;


// ---- API ----
clients_status_t open_client_db(void);
void close_client_db(void);