#pragma once

#include "types.h"
#include "constants.h"

// structure representing complete client information
typedef struct
{
    uint8_t         allow_flags;
    tg_nonce_t      client_nonce;
    tg_uid_t        client_id;
    tg_name_t       name;
    tg_public_key_t pub_key;
} client_entity_t;

// device nonce value stored to nvs_nonce partition of NVM