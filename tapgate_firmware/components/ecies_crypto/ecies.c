/**
 * @file ecies.c
 * @brief ECIES implementation using PSA Crypto API (ESP-IDF 6.0 / mbedTLS 4.x)
 *
 * Replaces legacy mbedtls_ecp/gcm/hkdf calls with PSA Crypto API which is
 * the only supported interface in mbedTLS 4.x bundled with ESP-IDF 6.0.
 */

#include "ecies.h"

#include <string.h>

#include "esp_log.h"
#include "esp_random.h"

#include "psa/crypto.h"

static const char *TAG = "ECIES";

/* HKDF info string for domain separation */
static const uint8_t HKDF_INFO[]   = "ECIES-AES256-GCM";
static const size_t  HKDF_INFO_LEN = sizeof(HKDF_INFO) - 1;

/* ── Internal helpers ────────────────────────────────────────────────────── */

/**
 * @brief Log PSA error with status code
 */
static void log_psa_error(const char *operation, psa_status_t status)
{
    ESP_LOGE(TAG, "%s failed: PSA status %d", operation, (int)status);
}

/**
 * @brief Check if buffer contains all zeros (weak shared secret detection)
 */
static bool is_all_zero(const uint8_t *buf, size_t len)
{
    uint8_t acc = 0;
    for (size_t i = 0; i < len; i++) {
        acc |= buf[i];
    }
    return (acc == 0);
}

/**
 * @brief Perform X25519 ECDH using PSA Crypto API
 *
 * Keys are 32-byte little-endian per RFC 7748 / PSA convention for Montgomery keys.
 *
 * @param our_privkey   32-byte private scalar
 * @param their_pubkey  32-byte peer public key
 * @param shared_secret 32-byte output
 * @return true on success
 */
static bool ecies_ecdh_x25519(const uint8_t our_privkey[ECIES_X25519_KEY_SIZE],
                               const uint8_t their_pubkey[ECIES_X25519_KEY_SIZE],
                               uint8_t       shared_secret[ECIES_X25519_KEY_SIZE])
{
    psa_key_id_t         key_id = PSA_KEY_ID_NULL;
    psa_key_attributes_t attr   = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t         status;
    size_t               secret_len = 0;
    bool                 ok         = false;

    psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
    psa_set_key_bits(&attr, 255);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
    psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

    status = psa_import_key(&attr, our_privkey, ECIES_X25519_KEY_SIZE, &key_id);
    if (status != PSA_SUCCESS) {
        log_psa_error("ECDH import privkey", status);
        goto cleanup;
    }

    status = psa_raw_key_agreement(PSA_ALG_ECDH,
                                   key_id,
                                   their_pubkey, ECIES_X25519_KEY_SIZE,
                                   shared_secret, ECIES_X25519_KEY_SIZE,
                                   &secret_len);
    if (status != PSA_SUCCESS) {
        log_psa_error("ECDH compute", status);
        goto cleanup;
    }

    if (secret_len != ECIES_X25519_KEY_SIZE) {
        ESP_LOGE(TAG, "ECDH: unexpected secret length %zu", secret_len);
        goto cleanup;
    }

    /* SECURITY: reject all-zero shared secret (cofactor / weak key attack) */
    if (is_all_zero(shared_secret, ECIES_X25519_KEY_SIZE)) {
        ESP_LOGE(TAG, "ECDH produced all-zero shared secret - weak key rejected");
        goto cleanup;
    }

    ok = true;

cleanup:
    if (key_id != PSA_KEY_ID_NULL) {
        psa_destroy_key(key_id);
    }
    psa_reset_key_attributes(&attr);

    if (!ok) {
        ecies_secure_zero(shared_secret, ECIES_X25519_KEY_SIZE);
    }
    return ok;
}

/**
 * @brief Derive AES-256 key from shared secret using HKDF-SHA256 via PSA
 */
static bool ecies_kdf(const uint8_t shared_secret[ECIES_X25519_KEY_SIZE],
                      uint8_t       aes_key[ECIES_AES_KEY_SIZE])
{
    psa_key_derivation_operation_t op     = PSA_KEY_DERIVATION_OPERATION_INIT;
    psa_key_id_t                   key_id = PSA_KEY_ID_NULL;
    psa_key_attributes_t           attr   = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t                   status;
    bool                           ok     = false;

    status = psa_key_derivation_setup(&op, PSA_ALG_HKDF(PSA_ALG_SHA_256));
    if (status != PSA_SUCCESS) {
        log_psa_error("HKDF setup", status);
        goto cleanup;
    }

    /* No salt: RFC 5869 Section 2.2 - use default all-zero salt */
    status = psa_key_derivation_input_bytes(&op,
                                            PSA_KEY_DERIVATION_INPUT_SALT,
                                            NULL, 0);
    if (status != PSA_SUCCESS) {
        log_psa_error("HKDF salt", status);
        goto cleanup;
    }

    /* Import shared secret as a PSA derivation key (IKM) */
    psa_set_key_type(&attr, PSA_KEY_TYPE_DERIVE);
    psa_set_key_bits(&attr, ECIES_X25519_KEY_SIZE * 8);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
    psa_set_key_algorithm(&attr, PSA_ALG_HKDF(PSA_ALG_SHA_256));

    status = psa_import_key(&attr, shared_secret, ECIES_X25519_KEY_SIZE, &key_id);
    if (status != PSA_SUCCESS) {
        log_psa_error("HKDF import IKM", status);
        goto cleanup;
    }

    status = psa_key_derivation_input_key(&op,
                                          PSA_KEY_DERIVATION_INPUT_SECRET,
                                          key_id);
    if (status != PSA_SUCCESS) {
        log_psa_error("HKDF input secret", status);
        goto cleanup;
    }

    status = psa_key_derivation_input_bytes(&op,
                                            PSA_KEY_DERIVATION_INPUT_INFO,
                                            HKDF_INFO, HKDF_INFO_LEN);
    if (status != PSA_SUCCESS) {
        log_psa_error("HKDF info", status);
        goto cleanup;
    }

    status = psa_key_derivation_output_bytes(&op, aes_key, ECIES_AES_KEY_SIZE);
    if (status != PSA_SUCCESS) {
        log_psa_error("HKDF output", status);
        goto cleanup;
    }

    ok = true;

cleanup:
    psa_key_derivation_abort(&op);
    if (key_id != PSA_KEY_ID_NULL) {
        psa_destroy_key(key_id);
    }
    psa_reset_key_attributes(&attr);
    return ok;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

bool ecies_generate_keypair(uint8_t private_key[ECIES_X25519_KEY_SIZE],
                             uint8_t public_key[ECIES_X25519_KEY_SIZE])
{
    if (private_key == NULL || public_key == NULL) {
        return false;
    }

    psa_key_id_t         key_id = PSA_KEY_ID_NULL;
    psa_key_attributes_t attr   = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t         status;
    bool                 ok     = false;

    psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
    psa_set_key_bits(&attr, 255);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

    status = psa_generate_key(&attr, &key_id);
    if (status != PSA_SUCCESS) {
        log_psa_error("Generate X25519 keypair", status);
        goto cleanup;
    }

    size_t out_len = 0;

    status = psa_export_key(key_id, private_key, ECIES_X25519_KEY_SIZE, &out_len);
    if (status != PSA_SUCCESS || out_len != ECIES_X25519_KEY_SIZE) {
        log_psa_error("Export private key", status);
        goto cleanup;
    }

    status = psa_export_public_key(key_id, public_key, ECIES_X25519_KEY_SIZE, &out_len);
    if (status != PSA_SUCCESS || out_len != ECIES_X25519_KEY_SIZE) {
        log_psa_error("Export public key", status);
        goto cleanup;
    }

    ok = true;

cleanup:
    if (key_id != PSA_KEY_ID_NULL) {
        psa_destroy_key(key_id);
    }
    psa_reset_key_attributes(&attr);

    if (!ok) {
        ecies_secure_zero(private_key, ECIES_X25519_KEY_SIZE);
    }
    return ok;
}

bool ecies_compute_public_key(const uint8_t private_key[ECIES_X25519_KEY_SIZE],
                               uint8_t       public_key[ECIES_X25519_KEY_SIZE])
{
    if (private_key == NULL || public_key == NULL) {
        return false;
    }

    psa_key_id_t         key_id = PSA_KEY_ID_NULL;
    psa_key_attributes_t attr   = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t         status;
    bool                 ok     = false;

    psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
    psa_set_key_bits(&attr, 255);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

    status = psa_import_key(&attr, private_key, ECIES_X25519_KEY_SIZE, &key_id);
    if (status != PSA_SUCCESS) {
        log_psa_error("Import private key", status);
        goto cleanup;
    }

    size_t out_len = 0;
    status = psa_export_public_key(key_id, public_key, ECIES_X25519_KEY_SIZE, &out_len);
    if (status != PSA_SUCCESS || out_len != ECIES_X25519_KEY_SIZE) {
        log_psa_error("Export public key", status);
        goto cleanup;
    }

    ok = true;

cleanup:
    if (key_id != PSA_KEY_ID_NULL) {
        psa_destroy_key(key_id);
    }
    psa_reset_key_attributes(&attr);
    return ok;
}

bool ecies_encrypt(const uint8_t *plaintext,
                   size_t         plaintext_len,
                   const uint8_t  recipient_pubkey[ECIES_X25519_KEY_SIZE],
                   uint8_t       *ciphertext,
                   size_t         ciphertext_capacity,
                   size_t        *ciphertext_len)
{
    if (plaintext == NULL || recipient_pubkey == NULL ||
        ciphertext == NULL || ciphertext_len == NULL) {
        return false;
    }

    if (plaintext_len == 0 || plaintext_len > ECIES_MAX_PLAINTEXT_SIZE) {
        ESP_LOGE(TAG, "Invalid plaintext length: %zu (max %d)",
                 plaintext_len, ECIES_MAX_PLAINTEXT_SIZE);
        return false;
    }

    if (plaintext_len > (SIZE_MAX - ECIES_ENCRYPTION_OVERHEAD)) {
        ESP_LOGE(TAG, "Plaintext length would overflow");
        return false;
    }

    const size_t required = plaintext_len + ECIES_ENCRYPTION_OVERHEAD;
    if (ciphertext_capacity < required) {
        ESP_LOGE(TAG, "Ciphertext buffer too small: need %zu, have %zu",
                 required, ciphertext_capacity);
        return false;
    }

    uint8_t ephemeral_priv[ECIES_X25519_KEY_SIZE];
    uint8_t ephemeral_pub [ECIES_X25519_KEY_SIZE];
    uint8_t shared_secret [ECIES_X25519_KEY_SIZE];
    uint8_t aes_key       [ECIES_AES_KEY_SIZE];
    uint8_t nonce         [ECIES_GCM_IV_SIZE];

    /*
     * Packet layout:
     *   [ephemeral_pub(32) | nonce(12) | ciphertext(N) | tag(16)]
     *
     * psa_aead_encrypt writes [ciphertext || tag] contiguously, so
     * out_ct must have capacity plaintext_len + ECIES_GCM_TAG_SIZE.
     * Because required = plaintext_len + 32 + 12 + 16, and
     * out_ct = ciphertext + 44, remaining space = capacity - 44
     * >= plaintext_len + 16. This is always satisfied after the
     * capacity check above.
     */
    uint8_t *out_pub   = ciphertext;
    uint8_t *out_nonce = ciphertext + ECIES_X25519_KEY_SIZE;
    uint8_t *out_ct    = ciphertext + ECIES_X25519_KEY_SIZE + ECIES_GCM_IV_SIZE;

    psa_key_id_t         aes_key_id = PSA_KEY_ID_NULL;
    psa_key_attributes_t aes_attr   = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t         status;
    bool                 ok         = false;

    /* Step 1: Ephemeral keypair */
    if (!ecies_generate_keypair(ephemeral_priv, ephemeral_pub)) {
        ESP_LOGE(TAG, "Failed to generate ephemeral keypair");
        goto cleanup;
    }
    memcpy(out_pub, ephemeral_pub, ECIES_X25519_KEY_SIZE);

    /* Step 2: ECDH */
    if (!ecies_ecdh_x25519(ephemeral_priv, recipient_pubkey, shared_secret)) {
        ESP_LOGE(TAG, "ECDH failed");
        goto cleanup;
    }

    /* Step 3: KDF */
    if (!ecies_kdf(shared_secret, aes_key)) {
        ESP_LOGE(TAG, "KDF failed");
        goto cleanup;
    }

    /* Step 4: Random nonce */
    esp_fill_random(nonce, ECIES_GCM_IV_SIZE);
    memcpy(out_nonce, nonce, ECIES_GCM_IV_SIZE);

    /* Step 5: AES-256-GCM encrypt (output: [ct || tag]) */
    psa_set_key_type(&aes_attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&aes_attr, ECIES_AES_KEY_SIZE * 8);
    psa_set_key_usage_flags(&aes_attr, PSA_KEY_USAGE_ENCRYPT);
    psa_set_key_algorithm(&aes_attr, PSA_ALG_GCM);

    status = psa_import_key(&aes_attr, aes_key, ECIES_AES_KEY_SIZE, &aes_key_id);
    if (status != PSA_SUCCESS) {
        log_psa_error("GCM import key", status);
        goto cleanup;
    }

    size_t out_ct_len = 0;
    status = psa_aead_encrypt(aes_key_id,
                              PSA_ALG_GCM,
                              nonce, ECIES_GCM_IV_SIZE,
                              NULL, 0,
                              plaintext, plaintext_len,
                              out_ct,
                              plaintext_len + ECIES_GCM_TAG_SIZE,
                              &out_ct_len);
    if (status != PSA_SUCCESS) {
        log_psa_error("GCM encrypt", status);
        goto cleanup;
    }

    if (out_ct_len != plaintext_len + ECIES_GCM_TAG_SIZE) {
        ESP_LOGE(TAG, "Unexpected GCM output length: %zu", out_ct_len);
        goto cleanup;
    }

    *ciphertext_len = required;
    ok = true;

cleanup:
    ecies_secure_zero(ephemeral_priv, sizeof(ephemeral_priv));
    ecies_secure_zero(shared_secret,  sizeof(shared_secret));
    ecies_secure_zero(aes_key,        sizeof(aes_key));
    ecies_secure_zero(nonce,          sizeof(nonce));

    if (aes_key_id != PSA_KEY_ID_NULL) {
        psa_destroy_key(aes_key_id);
    }
    psa_reset_key_attributes(&aes_attr);
    return ok;
}

bool ecies_decrypt(const uint8_t *ciphertext,
                   size_t         ciphertext_len,
                   const uint8_t  recipient_privkey[ECIES_X25519_KEY_SIZE],
                   uint8_t       *plaintext,
                   size_t         plaintext_capacity,
                   size_t        *plaintext_len)
{
    if (ciphertext == NULL || recipient_privkey == NULL ||
        plaintext == NULL || plaintext_len == NULL) {
        return false;
    }

    if (ciphertext_len < ECIES_ENCRYPTION_OVERHEAD) {
        ESP_LOGE(TAG, "Ciphertext too short: %zu (min %d)",
                 ciphertext_len, ECIES_ENCRYPTION_OVERHEAD);
        return false;
    }

    const size_t encrypted_len = ciphertext_len - ECIES_ENCRYPTION_OVERHEAD;

    if (encrypted_len > ECIES_MAX_PLAINTEXT_SIZE) {
        ESP_LOGE(TAG, "Payload too large: %zu (max %d)",
                 encrypted_len, ECIES_MAX_PLAINTEXT_SIZE);
        return false;
    }

    if (plaintext_capacity < encrypted_len) {
        ESP_LOGE(TAG, "Plaintext buffer too small: need %zu, have %zu",
                 encrypted_len, plaintext_capacity);
        return false;
    }

    /* Parse packet layout */
    const uint8_t *in_pub   = ciphertext;
    const uint8_t *in_nonce = ciphertext + ECIES_X25519_KEY_SIZE;
    const uint8_t *in_ct    = ciphertext + ECIES_X25519_KEY_SIZE + ECIES_GCM_IV_SIZE;
    /* in_ct points to [ct(encrypted_len) || tag(16)] */

    uint8_t shared_secret[ECIES_X25519_KEY_SIZE];
    uint8_t aes_key      [ECIES_AES_KEY_SIZE];

    psa_key_id_t         aes_key_id = PSA_KEY_ID_NULL;
    psa_key_attributes_t aes_attr   = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t         status;
    bool                 ok         = false;

    /* Step 1: ECDH */
    if (!ecies_ecdh_x25519(recipient_privkey, in_pub, shared_secret)) {
        ESP_LOGE(TAG, "ECDH failed");
        goto cleanup;
    }

    /* Step 2: KDF */
    if (!ecies_kdf(shared_secret, aes_key)) {
        ESP_LOGE(TAG, "KDF failed");
        goto cleanup;
    }

    /* Step 3: AES-256-GCM decrypt + verify tag */
    psa_set_key_type(&aes_attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&aes_attr, ECIES_AES_KEY_SIZE * 8);
    psa_set_key_usage_flags(&aes_attr, PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&aes_attr, PSA_ALG_GCM);

    status = psa_import_key(&aes_attr, aes_key, ECIES_AES_KEY_SIZE, &aes_key_id);
    if (status != PSA_SUCCESS) {
        log_psa_error("GCM import key", status);
        goto cleanup;
    }

    size_t pt_len = 0;
    status = psa_aead_decrypt(aes_key_id,
                              PSA_ALG_GCM,
                              in_nonce, ECIES_GCM_IV_SIZE,
                              NULL, 0,
                              in_ct, encrypted_len + ECIES_GCM_TAG_SIZE,
                              plaintext, plaintext_capacity,
                              &pt_len);

    if (status == PSA_ERROR_INVALID_SIGNATURE) {
        ESP_LOGE(TAG, "GCM authentication failed - data corrupted or wrong key");
        goto cleanup;
    }
    if (status != PSA_SUCCESS) {
        log_psa_error("GCM decrypt", status);
        goto cleanup;
    }

    *plaintext_len = pt_len;
    ok = true;

cleanup:
    ecies_secure_zero(shared_secret, sizeof(shared_secret));
    ecies_secure_zero(aes_key,       sizeof(aes_key));

    if (aes_key_id != PSA_KEY_ID_NULL) {
        psa_destroy_key(aes_key_id);
    }
    psa_reset_key_attributes(&aes_attr);

    if (!ok && plaintext != NULL) {
        ecies_secure_zero(plaintext, plaintext_capacity);
    }
    return ok;
}

void ecies_secure_zero(void *buffer, size_t size)
{
    if (buffer != NULL && size > 0) {
        memset(buffer, 0, size);
        /* Compiler barrier to prevent optimization */
        __asm__ __volatile__("" ::: "memory");
    }
}