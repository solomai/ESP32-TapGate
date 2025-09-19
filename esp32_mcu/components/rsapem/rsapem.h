#pragma once

#include <stddef.h>

#include "esp_err.h"

#ifndef ESP_ERR_NOT_SUPPORTED
#define ESP_ERR_NOT_SUPPORTED 0x106
#endif

#define RSAPEM_DEFAULT_KEY_BITS 2048
#define RSAPEM_MIN_KEY_BITS     1024
#define RSAPEM_MAX_KEY_BITS     4096

typedef struct
{
    char *public_pem;
    size_t public_pem_length; /* Excluding the terminating null byte. */
    char *private_pem;
    size_t private_pem_length; /* Excluding the terminating null byte. */
} rsapem_keypair_t;

esp_err_t rsapem_generate_keypair(int key_bits, rsapem_keypair_t *keypair);

void rsapem_free_keypair(rsapem_keypair_t *keypair);

