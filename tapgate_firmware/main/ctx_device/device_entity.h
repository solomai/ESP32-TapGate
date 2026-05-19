#pragma once

#include "types.h"
#include "constants.h"

// structure representing complete client information
typedef struct
{
    tg_uid_t         device_id;
    tg_name_t        name;
    tg_public_key_t  pub_key;
    tg_private_key_t private_key;
} device_entity_t;