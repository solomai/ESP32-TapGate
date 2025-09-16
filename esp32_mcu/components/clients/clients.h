#pragma once

#include <stdbool.h>
#include <stddef.h>

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


// ---- Flag positions ----
#define CLIENTS_ALLOW_FLAG_BLUETOOTH    0U
#define CLIENTS_ALLOW_FLAG_PUBLIC_MQTT  1U


// ---- API ----
clients_status_t open_client_db(void);
void close_client_db(void);
clients_status_t clients_add(const client_t *client);
clients_status_t clients_get(const client_uid_t client_id, const client_t **out_client);
clients_status_t clients_get_name(const client_uid_t client_id, char *buffer, size_t buffer_size);
clients_status_t clients_get_pub_pem(const client_uid_t client_id, char *buffer, size_t buffer_size);
clients_status_t clients_get_nonce(const client_uid_t client_id, nonce_t *nonce);
clients_status_t clients_set_nonce(const client_uid_t client_id, nonce_t nonce);
clients_status_t clients_get_allow_flags(const client_uid_t client_id, uint8_t *flags);
clients_status_t clients_set_allow_flags(const client_uid_t client_id, uint8_t flags);
clients_status_t clients_allow_bluetooth_get(const client_uid_t client_id, bool *allowed);
clients_status_t clients_allow_bluetooth_set(const client_uid_t client_id, bool allowed);
clients_status_t clients_allow_public_mqtt_get(const client_uid_t client_id, bool *allowed);
clients_status_t clients_allow_public_mqtt_set(const client_uid_t client_id, bool allowed);
clients_status_t clients_get_ids(client_uid_t *out_ids, size_t max_ids, size_t *out_count);
size_t clients_size(void);