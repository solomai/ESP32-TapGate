#pragma once
#include "device_entity.h"

// Status codes
typedef enum {
    DEVICE_ENTITY_OK = 0,
    DEVICE_ENTITY_ERR_NO_INIT,
    DEVICE_ENTITY_ERR_STORAGE,
    DEVICE_ENTITY_ERR_INVALID_ARG,
    DEVICE_ENTITY_ERR_EXISTS,
    DEVICE_ENTITY_ERR_TOO_LONG,
} device_status_t;

// Open / Close all releated partition
device_status_t open_device_db(void);
void close_device_db(void);

// Set/Get Client record
device_status_t get_device_entity(device_entity_t *client);
device_status_t update_device_entity(const device_entity_t *client);

// Quick Set/Get Client fields
device_status_t get_device_pub_key(public_key_t *public_key);
device_status_t get_device_private_key(private_key_t *private_key);
device_status_t get_device_nonce(nonce_t *nonce);
device_status_t update_device_nonce(nonce_t nonce);