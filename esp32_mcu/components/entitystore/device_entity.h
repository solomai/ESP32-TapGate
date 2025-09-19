#pragma once

#include "types.h"
#include "constants.h"

// structure representing complete client information
typedef struct
{
    nonce_t       client_nonce;
    uid_t         device_id;
    name_t        name;
    public_key_t  pub_pem;
    private_key_t private_pem;
} device_entity_t;