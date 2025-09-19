#include "rsapem.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#ifdef ESP_PLATFORM
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#endif

#define RSAPEM_LOG_TAG "rsapem"
#define RSAPEM_RSA_PUBLIC_EXPONENT 65537

static size_t rsapem_bytes_for_bits(int key_bits)
{
    if (key_bits <= 0)
    {
        return 0;
    }
    return (size_t)((key_bits + 7) / 8);
}

static size_t rsapem_public_capacity(int key_bits)
{
    size_t modulus_bytes = rsapem_bytes_for_bits(key_bits);
    if (modulus_bytes == 0)
    {
        return 0;
    }

    size_t der_bytes = modulus_bytes + 64;
    size_t base64_bytes = ((der_bytes + 2) / 3) * 4;
    size_t newline_bytes = (base64_bytes / 64) + 1;
    return base64_bytes + newline_bytes + 64 + 1;
}

static size_t rsapem_private_capacity(int key_bits)
{
    size_t modulus_bytes = rsapem_bytes_for_bits(key_bits);
    if (modulus_bytes == 0)
    {
        return 0;
    }

    size_t der_bytes = ((modulus_bytes * 9) + 1) / 2 + 64;
    size_t base64_bytes = ((der_bytes + 2) / 3) * 4;
    size_t newline_bytes = (base64_bytes / 64) + 1;
    return base64_bytes + newline_bytes + 64 + 1;
}

static void rsapem_secure_zero(void *buffer, size_t length)
{
    if (!buffer || length == 0)
    {
        return;
    }

    volatile unsigned char *ptr = (volatile unsigned char *)buffer;
    while (length--)
    {
        *ptr++ = 0;
    }
}

static void rsapem_reset_keypair(rsapem_keypair_t *keypair)
{
    if (!keypair)
    {
        return;
    }

    keypair->public_pem = NULL;
    keypair->public_pem_length = 0;
    keypair->private_pem = NULL;
    keypair->private_pem_length = 0;
}

esp_err_t rsapem_generate_keypair(int key_bits, rsapem_keypair_t *keypair)
{
    if (!keypair)
    {
        return ESP_ERR_INVALID_ARG;
    }

    rsapem_reset_keypair(keypair);

    if (key_bits < RSAPEM_MIN_KEY_BITS || key_bits > RSAPEM_MAX_KEY_BITS)
    {
        return ESP_ERR_INVALID_ARG;
    }

    size_t public_capacity = rsapem_public_capacity(key_bits);
    size_t private_capacity = rsapem_private_capacity(key_bits);
    if (public_capacity == 0 || private_capacity == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char *public_buffer = (char *)calloc(public_capacity, sizeof(char));
    char *private_buffer = (char *)calloc(private_capacity, sizeof(char));
    if (!public_buffer || !private_buffer)
    {
        free(public_buffer);
        rsapem_secure_zero(private_buffer, private_capacity);
        free(private_buffer);
        return ESP_ERR_NO_MEM;
    }

#ifdef ESP_PLATFORM
    esp_err_t status = ESP_OK;
    bool success = true;
    int ret = 0;

    mbedtls_pk_context pk;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    const mbedtls_pk_info_t *pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA);
    if (!pk_info)
    {
        ESP_LOGE(RSAPEM_LOG_TAG, "Failed to resolve RSA pk info");
        status = ESP_FAIL;
        success = false;
    }
    else if ((ret = mbedtls_pk_setup(&pk, pk_info)) != 0)
    {
        ESP_LOGE(RSAPEM_LOG_TAG, "mbedtls_pk_setup failed (%d)", ret);
        status = ESP_FAIL;
        success = false;
    }

    const unsigned char personalization[] = "rsapem";
    if (success)
    {
        ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    personalization, sizeof(personalization));
        if (ret != 0)
        {
            ESP_LOGE(RSAPEM_LOG_TAG, "mbedtls_ctr_drbg_seed failed (%d)", ret);
            status = ESP_FAIL;
            success = false;
        }
    }

    if (success)
    {
        ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(pk), mbedtls_ctr_drbg_random, &ctr_drbg,
                                  key_bits, RSAPEM_RSA_PUBLIC_EXPONENT);
        if (ret != 0)
        {
            ESP_LOGE(RSAPEM_LOG_TAG, "mbedtls_rsa_gen_key failed (%d)", ret);
            status = ESP_FAIL;
            success = false;
        }
    }

    if (success)
    {
        ret = mbedtls_pk_write_pubkey_pem(&pk, (unsigned char *)public_buffer, public_capacity);
        if (ret != 0)
        {
            ESP_LOGE(RSAPEM_LOG_TAG, "Failed to export public key (%d)", ret);
            status = ESP_FAIL;
            success = false;
        }
    }

    if (success)
    {
        ret = mbedtls_pk_write_key_pem(&pk, (unsigned char *)private_buffer, private_capacity);
        if (ret != 0)
        {
            ESP_LOGE(RSAPEM_LOG_TAG, "Failed to export private key (%d)", ret);
            status = ESP_FAIL;
            success = false;
        }
    }

    if (success)
    {
        keypair->public_pem = public_buffer;
        keypair->public_pem_length = strlen(public_buffer);
        keypair->private_pem = private_buffer;
        keypair->private_pem_length = strlen(private_buffer);
    }
    else
    {
        rsapem_secure_zero(private_buffer, private_capacity);
        free(public_buffer);
        free(private_buffer);
    }

    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    if (success)
    {
        return ESP_OK;
    }

    return status;
#else
    (void)key_bits;
    rsapem_secure_zero(private_buffer, private_capacity);
    free(public_buffer);
    free(private_buffer);
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

void rsapem_free_keypair(rsapem_keypair_t *keypair)
{
    if (!keypair)
    {
        return;
    }

    if (keypair->public_pem)
    {
        free(keypair->public_pem);
    }

    if (keypair->private_pem)
    {
        rsapem_secure_zero(keypair->private_pem, keypair->private_pem_length + 1);
        free(keypair->private_pem);
    }

    rsapem_reset_keypair(keypair);
}

