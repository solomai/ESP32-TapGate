#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "constants.h"

// nonce type
typedef uint32_t nonce_t;

// UID type.
typedef uint8_t uid_t[UID_CAP];

typedef uint8_t name_t[NAME_MAX_SIZE];

// RSA keys types
typedef uint8_t public_key_t[PUBPEM_CAP];
typedef uint8_t private_key_t[PRVPEM_CAP];