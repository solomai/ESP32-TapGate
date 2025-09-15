#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

// nonce type
typedef uint32_t nonce_t;

// UID type. Up to 15 characters are allowed due to NVM key size limitations.
typedef uint8_t  client_uid_t[15];
