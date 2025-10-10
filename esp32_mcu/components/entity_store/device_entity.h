#pragma once

#include "types.h"
#include "constants.h"

// structure representing complete client information
typedef struct
{
    uid_t         device_id;
    name_t        name;
    public_key_t  pub_key;
    private_key_t private_key;
} device_entity_t;