#pragma once

#include "types.h"
#include "constants.h"

// structure representing complete client information
typedef struct
{
    uint8_t       allow_flags;
    nonce_t       client_nonce;
    uid_t         client_id;
    name_t        name;
    public_key_t  pub_pem;
} client_entity_t;