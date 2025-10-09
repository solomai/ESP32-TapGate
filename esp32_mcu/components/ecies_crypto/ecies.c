#include "ecies.h"

#include <string.h>

#include "esp_err.h"
#include "esp_random.h"

#include "logs.h"

#include "mbedtls/bignum.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecp.h"
#include "mbedtls/gcm.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"
#include "mbedtls/platform_util.h"

#define TAG "ecies_crypto"

// HKDF context string to bind derived keys to this protocol.
static const uint8_t k_hkdf_info[] = "ESP32-TapGate-ECIES";

static int ecies_random(void *ctx, unsigned char *out, size_t len)
{
    (void)ctx;
    if (len == 0) {
        return 0;
    }

    esp_fill_random(out, len);
    return 0;
}

static void ecies_zeroize(void *buf, size_t len)
{
    if (buf != NULL && len > 0) {
        mbedtls_platform_zeroize(buf, len);
    }
}

static esp_err_t ecies_export_public_key(const mbedtls_ecp_point *point, public_key_t public_key)
{
    if (point == NULL || public_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int ret = mbedtls_mpi_write_binary_le(&point->X, public_key, PUBPEM_CAP);
    if (ret != 0) {
        LOGE(TAG, "Failed to export public key: 0x%04x", -ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t ecies_import_public_key(mbedtls_ecp_point *point, const public_key_t public_key)
{
    if (point == NULL || public_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int ret = mbedtls_mpi_read_binary_le(&point->X, public_key, PUBPEM_CAP);
    if (ret != 0) {
        LOGE(TAG, "Failed to parse public key: 0x%04x", -ret);
        return ESP_FAIL;
    }

    // For Montgomery curves, Y is unused; set Y=1 and Z=1 to represent affine point.
    ret = mbedtls_mpi_lset(&point->Y, 1);
    if (ret != 0) {
        LOGE(TAG, "Failed to initialize point coordinate: 0x%04x", -ret);
        return ESP_FAIL;
    }

    ret = mbedtls_mpi_lset(&point->Z, 1);
    if (ret != 0) {
        LOGE(TAG, "Failed to initialize projective coordinate: 0x%04x", -ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t ecies_compute_shared_secret(const private_key_t private_key,
                                             const public_key_t peer_public,
                                             uint8_t shared_secret[ECIES_SHARED_SECRET_SIZE])
{
    if (private_key == NULL || peer_public == NULL || shared_secret == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t status = ESP_FAIL;
    mbedtls_ecp_group grp;
    mbedtls_ecp_group_init(&grp);

    mbedtls_mpi d;
    mbedtls_mpi_init(&d);

    mbedtls_ecp_point Q;
    mbedtls_ecp_point_init(&Q);

    mbedtls_mpi z;
    mbedtls_mpi_init(&z);

    int ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
    if (ret != 0) {
        LOGE(TAG, "Failed to load curve: 0x%04x", -ret);
        goto cleanup;
    }

    ret = mbedtls_mpi_read_binary_le(&d, private_key, PRVPEM_CAP);
    if (ret != 0) {
        LOGE(TAG, "Failed to import private key: 0x%04x", -ret);
        goto cleanup;
    }

    ret = ecies_import_public_key(&Q, peer_public);
    if (ret != ESP_OK) {
        goto cleanup;
    }

    ret = mbedtls_ecdh_compute_shared(&grp, &z, &Q, &d, NULL, NULL);
    if (ret != 0) {
        LOGE(TAG, "Failed to compute shared secret: 0x%04x", -ret);
        goto cleanup;
    }

    ret = mbedtls_mpi_write_binary_le(&z, shared_secret, ECIES_SHARED_SECRET_SIZE);
    if (ret != 0) {
        LOGE(TAG, "Failed to export shared secret: 0x%04x", -ret);
        goto cleanup;
    }

    status = ESP_OK;

cleanup:
    mbedtls_mpi_free(&z);
    mbedtls_ecp_point_free(&Q);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&grp);
    return status;
}

static esp_err_t ecies_derive_symmetric_key(const uint8_t shared_secret[ECIES_SHARED_SECRET_SIZE],
                                            uint8_t symmetric_key[ECIES_SYMMETRIC_KEY_SIZE])
{
    if (shared_secret == NULL || symmetric_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md == NULL) {
        LOGE(TAG, "Failed to obtain SHA-256 MD info");
        return ESP_FAIL;
    }

    int ret = mbedtls_hkdf(md,
                           NULL,
                           0,
                           shared_secret,
                           ECIES_SHARED_SECRET_SIZE,
                           k_hkdf_info,
                           sizeof(k_hkdf_info) - 1,
                           symmetric_key,
                           ECIES_SYMMETRIC_KEY_SIZE);
    if (ret != 0) {
        LOGE(TAG, "HKDF derivation failed: 0x%04x", -ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t ecies_encrypt_aes_gcm(const uint8_t key[ECIES_SYMMETRIC_KEY_SIZE],
                                       const uint8_t *plaintext,
                                       size_t plaintext_len,
                                       uint8_t *ciphertext,
                                       uint8_t nonce[ECIES_NONCE_SIZE],
                                       uint8_t tag[ECIES_TAG_SIZE])
{
    if (key == NULL || plaintext == NULL || ciphertext == NULL || nonce == NULL || tag == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);

    esp_err_t status = ESP_FAIL;
    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, ECIES_SYMMETRIC_KEY_SIZE * 8);
    if (ret != 0) {
        LOGE(TAG, "Failed to set AES-GCM key: 0x%04x", -ret);
        goto cleanup;
    }

    esp_fill_random(nonce, ECIES_NONCE_SIZE);

    ret = mbedtls_gcm_crypt_and_tag(&gcm,
                                    MBEDTLS_GCM_ENCRYPT,
                                    plaintext_len,
                                    nonce,
                                    ECIES_NONCE_SIZE,
                                    NULL,
                                    0,
                                    plaintext,
                                    ciphertext,
                                    ECIES_TAG_SIZE,
                                    tag);
    if (ret != 0) {
        LOGE(TAG, "AES-GCM encryption failed: 0x%04x", -ret);
        goto cleanup;
    }

    status = ESP_OK;

cleanup:
    mbedtls_gcm_free(&gcm);
    return status;
}

static esp_err_t ecies_decrypt_aes_gcm(const uint8_t key[ECIES_SYMMETRIC_KEY_SIZE],
                                       const uint8_t *ciphertext,
                                       size_t ciphertext_len,
                                       const uint8_t nonce[ECIES_NONCE_SIZE],
                                       const uint8_t tag[ECIES_TAG_SIZE],
                                       uint8_t *plaintext)
{
    if (key == NULL || ciphertext == NULL || nonce == NULL || tag == NULL || plaintext == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);

    esp_err_t status = ESP_FAIL;
    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, ECIES_SYMMETRIC_KEY_SIZE * 8);
    if (ret != 0) {
        LOGE(TAG, "Failed to set AES-GCM key: 0x%04x", -ret);
        goto cleanup;
    }

    ret = mbedtls_gcm_auth_decrypt(&gcm,
                                   ciphertext_len,
                                   nonce,
                                   ECIES_NONCE_SIZE,
                                   NULL,
                                   0,
                                   tag,
                                   ECIES_TAG_SIZE,
                                   ciphertext,
                                   plaintext);
    if (ret != 0) {
        LOGE(TAG, "AES-GCM authentication failed: 0x%04x", -ret);
        goto cleanup;
    }

    status = ESP_OK;

cleanup:
    mbedtls_gcm_free(&gcm);
    return status;
}

esp_err_t ecies_generate_keypair(private_key_t private_key, public_key_t public_key)
{
    if (private_key == NULL || public_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t status = ESP_FAIL;
    mbedtls_ecp_keypair keypair;
    mbedtls_ecp_keypair_init(&keypair);

    int ret = mbedtls_ecp_group_load(&keypair.grp, MBEDTLS_ECP_DP_CURVE25519);
    if (ret != 0) {
        LOGE(TAG, "Failed to load curve for key generation: 0x%04x", -ret);
        goto cleanup;
    }

    ret = mbedtls_ecp_gen_keypair(&keypair.grp, &keypair.d, &keypair.Q, ecies_random, NULL);
    if (ret != 0) {
        LOGE(TAG, "Failed to generate key pair: 0x%04x", -ret);
        goto cleanup;
    }

    ret = mbedtls_mpi_write_binary_le(&keypair.d, private_key, PRVPEM_CAP);
    if (ret != 0) {
        LOGE(TAG, "Failed to export private key: 0x%04x", -ret);
        goto cleanup;
    }

    ret = ecies_export_public_key(&keypair.Q, public_key);
    if (ret != ESP_OK) {
        goto cleanup;
    }

    LOGI(TAG, "Generated X25519 key pair");
    status = ESP_OK;

cleanup:
    mbedtls_ecp_keypair_free(&keypair);
    if (status != ESP_OK) {
        ecies_zeroize(private_key, PRVPEM_CAP);
        ecies_zeroize(public_key, PUBPEM_CAP);
    }

    return status;
}

esp_err_t ecies_derive_public_key(const private_key_t private_key, public_key_t public_key)
{
    if (private_key == NULL || public_key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t status = ESP_FAIL;
    mbedtls_ecp_group grp;
    mbedtls_ecp_group_init(&grp);

    mbedtls_mpi d;
    mbedtls_mpi_init(&d);

    mbedtls_ecp_point Q;
    mbedtls_ecp_point_init(&Q);

    int ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
    if (ret != 0) {
        LOGE(TAG, "Failed to load curve for public key derivation: 0x%04x", -ret);
        goto cleanup;
    }

    ret = mbedtls_mpi_read_binary_le(&d, private_key, PRVPEM_CAP);
    if (ret != 0) {
        LOGE(TAG, "Failed to import private key: 0x%04x", -ret);
        goto cleanup;
    }

    ret = mbedtls_ecp_mul(&grp, &Q, &d, &grp.G, ecies_random, NULL);
    if (ret != 0) {
        LOGE(TAG, "Failed to compute public key: 0x%04x", -ret);
        goto cleanup;
    }

    ret = ecies_export_public_key(&Q, public_key);
    if (ret != ESP_OK) {
        goto cleanup;
    }

    LOGI(TAG, "Derived X25519 public key");
    status = ESP_OK;

cleanup:
    mbedtls_ecp_point_free(&Q);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&grp);

    return status;
}

esp_err_t ecies_encode(const public_key_t peer_public_key,
                       const uint8_t *plaintext,
                       size_t plaintext_len,
                       uint8_t *ciphertext,
                       size_t ciphertext_buf_len,
                       size_t *ciphertext_len,
                       public_key_t eph_public_key,
                       uint8_t nonce[ECIES_NONCE_SIZE],
                       uint8_t tag[ECIES_TAG_SIZE])
{
    if (peer_public_key == NULL || plaintext == NULL || ciphertext == NULL || ciphertext_len == NULL ||
        eph_public_key == NULL || nonce == NULL || tag == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ciphertext_buf_len < plaintext_len) {
        LOGE(TAG, "Ciphertext buffer too small: %zu < %zu", ciphertext_buf_len, plaintext_len);
        return ESP_ERR_INVALID_SIZE;
    }

    private_key_t eph_private_key = {0};
    esp_err_t status = ecies_generate_keypair(eph_private_key, eph_public_key);
    if (status != ESP_OK) {
        return status;
    }

    uint8_t shared_secret[ECIES_SHARED_SECRET_SIZE] = {0};
    status = ecies_compute_shared_secret(eph_private_key, peer_public_key, shared_secret);
    if (status != ESP_OK) {
        LOGE(TAG, "Failed to derive shared secret for encryption");
        goto cleanup;
    }

    uint8_t symmetric_key[ECIES_SYMMETRIC_KEY_SIZE] = {0};
    status = ecies_derive_symmetric_key(shared_secret, symmetric_key);
    if (status != ESP_OK) {
        LOGE(TAG, "Failed to derive symmetric key");
        goto cleanup;
    }

    status = ecies_encrypt_aes_gcm(symmetric_key, plaintext, plaintext_len, ciphertext, nonce, tag);
    if (status != ESP_OK) {
        LOGE(TAG, "AES-GCM encryption failed");
        goto cleanup;
    }

    *ciphertext_len = plaintext_len;
    LOGI(TAG, "Encrypted %zu bytes", plaintext_len);

cleanup:
    ecies_zeroize(eph_private_key, sizeof(eph_private_key));
    ecies_zeroize(shared_secret, sizeof(shared_secret));
    ecies_zeroize(symmetric_key, sizeof(symmetric_key));

    return status;
}

esp_err_t ecies_decode(const private_key_t private_key,
                       const public_key_t peer_eph_public,
                       const uint8_t *ciphertext,
                       size_t ciphertext_len,
                       const uint8_t nonce[ECIES_NONCE_SIZE],
                       const uint8_t tag[ECIES_TAG_SIZE],
                       uint8_t *plaintext,
                       size_t plaintext_buf_len,
                       size_t *plaintext_len)
{
    if (private_key == NULL || peer_eph_public == NULL || ciphertext == NULL || nonce == NULL || tag == NULL ||
        plaintext == NULL || plaintext_len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (plaintext_buf_len < ciphertext_len) {
        LOGE(TAG, "Plaintext buffer too small: %zu < %zu", plaintext_buf_len, ciphertext_len);
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t shared_secret[ECIES_SHARED_SECRET_SIZE] = {0};
    esp_err_t status = ecies_compute_shared_secret(private_key, peer_eph_public, shared_secret);
    if (status != ESP_OK) {
        LOGE(TAG, "Failed to compute shared secret for decryption");
        goto cleanup;
    }

    uint8_t symmetric_key[ECIES_SYMMETRIC_KEY_SIZE] = {0};
    status = ecies_derive_symmetric_key(shared_secret, symmetric_key);
    if (status != ESP_OK) {
        LOGE(TAG, "Failed to derive symmetric key");
        goto cleanup;
    }

    status = ecies_decrypt_aes_gcm(symmetric_key, ciphertext, ciphertext_len, nonce, tag, plaintext);
    if (status != ESP_OK) {
        LOGE(TAG, "AES-GCM decryption failed");
        goto cleanup;
    }

    *plaintext_len = ciphertext_len;
    LOGI(TAG, "Decrypted %zu bytes", ciphertext_len);

cleanup:
    ecies_zeroize(shared_secret, sizeof(shared_secret));
    ecies_zeroize(symmetric_key, sizeof(symmetric_key));

    return status;
}