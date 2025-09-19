#pragma once
#include "client_entity.h"

// Status codes
typedef enum {
    CLIENTS_ENTITY_OK = 0,
    CLIENTS_ENTITY_ERR_NO_INIT,
    CLIENTS_ENTITY_ERR_STORAGE,
    CLIENTS_ENTITY_ERR_FULL,
    CLIENTS_ENTITY_ERR_NOT_FOUND,
    CLIENTS_ENTITY_ERR_INVALID_ARG,
    CLIENTS_ENTITY_ERR_EXISTS,
    CLIENTS_ENTITY_ERR_TOO_LONG,
} clients_status_t;

// ---- API ----

// Open / Close all releated partition
clients_status_t open_client_db(void);
void close_client_db(void);

// Iterator for traversing client IDs over NVM Iterators
// return null in the case no valid iterator
/*define type instead void*/void* begin_client_id();
/*define type instead void*/void* end_client_id();
/*define type instead void*/void* next_client_id();

// Quick checks
bool is_client_added(const uid_t client_id);

// Set/Get Client record
clients_status_t add_client_entity(const client_entity_t *client);
clients_status_t get_client_entity(const uid_t client_id,
                                         client_entity_t *client);

// Set/Get Client fields
clients_status_t get_client_pub_key(const uid_t client_id,
                                          public_key_t *public_key);
clients_status_t get_client_nonce(const uid_t client_id,
                                        nonce_t *nonce);
clients_status_t update_client_nonce(const uid_t client_id,
                                           nonce_t nonce);