/**
 * @file ecies.c
 * @brief ECIES implementation using ESP-IDF 5.5 mbedTLS
 */

 #include "ecies.h"
 #include <string.h>
 #include "esp_random.h"
 #include "esp_log.h"
 #include "mbedtls/ecp.h"
 #include "mbedtls/gcm.h"
 #include "mbedtls/hkdf.h"
 #include "mbedtls/md.h"
 #include "mbedtls/platform_util.h"
 
 static const char *TAG = "ECIES";
 
 /* HKDF info string for domain separation */
 static const uint8_t HKDF_INFO[] = "ECIES-AES256-GCM";
 static const size_t HKDF_INFO_LEN = sizeof(HKDF_INFO) - 1;
 
 /**
  * @brief Hardware RNG callback for mbedTLS
  */
 static int ecies_rng(void *ctx, unsigned char *output, size_t len)
 {
     (void)ctx;
     esp_fill_random(output, len);
     return 0;
 }
 
 /**
  * @brief Perform X25519 ECDH and derive shared secret
  * Using low-level ECP API for mbedTLS 3.x compatibility
  */
 static bool ecies_ecdh_x25519(const uint8_t our_privkey[ECIES_X25519_KEY_SIZE],
                              const uint8_t their_pubkey[ECIES_X25519_KEY_SIZE],
                              uint8_t shared_secret[ECIES_X25519_KEY_SIZE])
 {
     mbedtls_ecp_group grp;
     mbedtls_mpi d;      /* Our private key */
     mbedtls_ecp_point Q; /* Their public key */
     mbedtls_mpi z;      /* Shared secret */
     
     mbedtls_ecp_group_init(&grp);
     mbedtls_mpi_init(&d);
     mbedtls_ecp_point_init(&Q);
     mbedtls_mpi_init(&z);
     
     int ret = -1;
     
     /* Load Curve25519 */
     ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to load X25519 curve: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Load our private key */
     ret = mbedtls_mpi_read_binary(&d, our_privkey, ECIES_X25519_KEY_SIZE);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to load private key: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Load their public key (X coordinate only for Curve25519) */
     ret = mbedtls_mpi_read_binary(&Q.MBEDTLS_PRIVATE(X), their_pubkey, ECIES_X25519_KEY_SIZE);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to load public key: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Set Z coordinate to 1 for Montgomery curve */
     ret = mbedtls_mpi_lset(&Q.MBEDTLS_PRIVATE(Z), 1);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to set Z coordinate: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Compute shared secret: z = d * Q */
     ret = mbedtls_ecp_mul(&grp, &Q, &d, &Q, ecies_rng, NULL);
     if (ret != 0) {
         ESP_LOGE(TAG, "ECDH compute failed: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Export shared secret (X coordinate of result point) */
     ret = mbedtls_mpi_write_binary(&Q.MBEDTLS_PRIVATE(X), shared_secret, ECIES_X25519_KEY_SIZE);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to export shared secret: -0x%04X", -ret);
         goto cleanup;
     }
 
 cleanup:
     mbedtls_ecp_group_free(&grp);
     mbedtls_mpi_free(&d);
     mbedtls_ecp_point_free(&Q);
     mbedtls_mpi_free(&z);
     return (ret == 0);
 }
 
 /**
  * @brief Derive AES key from shared secret using HKDF-SHA256
  */
 static bool ecies_kdf(const uint8_t shared_secret[ECIES_X25519_KEY_SIZE],
                      uint8_t aes_key[ECIES_AES_KEY_SIZE])
 {
     const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
     if (md_info == NULL) {
         ESP_LOGE(TAG, "Failed to get SHA256 MD info");
         return false;
     }
     
     /* HKDF with no salt (NULL), shared secret as IKM, and info string */
     int ret = mbedtls_hkdf(md_info,
                           NULL, 0,                    /* salt */
                           shared_secret, ECIES_X25519_KEY_SIZE, /* IKM */
                           HKDF_INFO, HKDF_INFO_LEN,  /* info */
                           aes_key, ECIES_AES_KEY_SIZE); /* OKM */
     
     if (ret != 0) {
         ESP_LOGE(TAG, "HKDF failed: -0x%04X", -ret);
         return false;
     }
     
     return true;
 }
 
 bool ecies_generate_keypair(uint8_t private_key[ECIES_X25519_KEY_SIZE],
                             uint8_t public_key[ECIES_X25519_KEY_SIZE])
 {
     if (private_key == NULL || public_key == NULL) {
         return false;
     }
     
     mbedtls_ecp_group grp;
     mbedtls_mpi d;
     mbedtls_ecp_point Q;
     
     mbedtls_ecp_group_init(&grp);
     mbedtls_mpi_init(&d);
     mbedtls_ecp_point_init(&Q);
     
     int ret = -1;
     
     /* Load Curve25519 */
     ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to load curve: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Generate key pair: Q = d * G */
     ret = mbedtls_ecp_gen_keypair(&grp, &d, &Q, ecies_rng, NULL);
     if (ret != 0) {
         ESP_LOGE(TAG, "Key generation failed: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Export private key */
     ret = mbedtls_mpi_write_binary(&d, private_key, ECIES_X25519_KEY_SIZE);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to export private key: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Export public key (X coordinate only for X25519) */
     ret = mbedtls_mpi_write_binary(&Q.MBEDTLS_PRIVATE(X), public_key, ECIES_X25519_KEY_SIZE);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to export public key: -0x%04X", -ret);
         goto cleanup;
     }
 
 cleanup:
     mbedtls_ecp_group_free(&grp);
     mbedtls_mpi_free(&d);
     mbedtls_ecp_point_free(&Q);
     return (ret == 0);
 }
 
 bool ecies_compute_public_key(const uint8_t private_key[ECIES_X25519_KEY_SIZE],
                               uint8_t public_key[ECIES_X25519_KEY_SIZE])
 {
     if (private_key == NULL || public_key == NULL) {
         return false;
     }
     
     mbedtls_ecp_group grp;
     mbedtls_mpi d;
     mbedtls_ecp_point Q;
     
     mbedtls_ecp_group_init(&grp);
     mbedtls_mpi_init(&d);
     mbedtls_ecp_point_init(&Q);
     
     int ret = -1;
     
     /* Load Curve25519 */
     ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to load curve: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Load private key */
     ret = mbedtls_mpi_read_binary(&d, private_key, ECIES_X25519_KEY_SIZE);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to load private key: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Compute public key Q = d * G */
     ret = mbedtls_ecp_mul(&grp, &Q, &d, &grp.G, ecies_rng, NULL);
     if (ret != 0) {
         ESP_LOGE(TAG, "Public key computation failed: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Export public key (X coordinate) */
     ret = mbedtls_mpi_write_binary(&Q.MBEDTLS_PRIVATE(X), public_key, ECIES_X25519_KEY_SIZE);
     if (ret != 0) {
         ESP_LOGE(TAG, "Failed to export public key: -0x%04X", -ret);
         goto cleanup;
     }
 
 cleanup:
     mbedtls_ecp_group_free(&grp);
     mbedtls_mpi_free(&d);
     mbedtls_ecp_point_free(&Q);
     return (ret == 0);
 }
 
 bool ecies_encrypt(const uint8_t *plaintext,
                    size_t plaintext_len,
                    const uint8_t recipient_pubkey[ECIES_X25519_KEY_SIZE],
                    uint8_t *ciphertext,
                    size_t *ciphertext_len)
 {
     if (plaintext == NULL || recipient_pubkey == NULL || 
         ciphertext == NULL || ciphertext_len == NULL) {
         return false;
     }
     
     bool success = false;
     mbedtls_gcm_context gcm_ctx;
     mbedtls_gcm_init(&gcm_ctx);
     
     /* Allocate buffers for ephemeral key and derived key */
     uint8_t ephemeral_privkey[ECIES_X25519_KEY_SIZE];
     uint8_t ephemeral_pubkey[ECIES_X25519_KEY_SIZE];
     uint8_t shared_secret[ECIES_X25519_KEY_SIZE];
     uint8_t aes_key[ECIES_AES_KEY_SIZE];
     uint8_t nonce[ECIES_GCM_IV_SIZE];
     
     /* Build output: [ephemeral_pubkey || nonce || ciphertext || tag] */
     uint8_t *out_ephemeral = ciphertext;
     uint8_t *out_nonce = ciphertext + ECIES_X25519_KEY_SIZE;
     uint8_t *out_ciphertext = ciphertext + ECIES_X25519_KEY_SIZE + ECIES_GCM_IV_SIZE;
     uint8_t *out_tag = ciphertext + ECIES_X25519_KEY_SIZE + ECIES_GCM_IV_SIZE + plaintext_len;
     
     /* Step 1: Generate ephemeral key pair */
     if (!ecies_generate_keypair(ephemeral_privkey, ephemeral_pubkey)) {
         ESP_LOGE(TAG, "Failed to generate ephemeral keypair");
         goto cleanup;
     }
     
     memcpy(out_ephemeral, ephemeral_pubkey, ECIES_X25519_KEY_SIZE);
     
     /* Step 2: Perform ECDH to get shared secret */
     if (!ecies_ecdh_x25519(ephemeral_privkey, recipient_pubkey, shared_secret)) {
         ESP_LOGE(TAG, "ECDH failed");
         goto cleanup;
     }
     
     /* Step 3: Derive AES key using HKDF */
     if (!ecies_kdf(shared_secret, aes_key)) {
         ESP_LOGE(TAG, "KDF failed");
         goto cleanup;
     }
     
     /* Step 4: Generate random nonce */
     esp_fill_random(nonce, ECIES_GCM_IV_SIZE);
     memcpy(out_nonce, nonce, ECIES_GCM_IV_SIZE);
     
     /* Step 5: Setup GCM context */
     int ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, 
                                 aes_key, ECIES_AES_KEY_SIZE * 8);
     if (ret != 0) {
         ESP_LOGE(TAG, "GCM setkey failed: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Step 6: Encrypt and authenticate */
     ret = mbedtls_gcm_crypt_and_tag(&gcm_ctx,
                                    MBEDTLS_GCM_ENCRYPT,
                                    plaintext_len,
                                    nonce, ECIES_GCM_IV_SIZE,
                                    NULL, 0,  /* No additional authenticated data */
                                    plaintext,
                                    out_ciphertext,
                                    ECIES_GCM_TAG_SIZE,
                                    out_tag);
     
     if (ret != 0) {
         ESP_LOGE(TAG, "GCM encryption failed: -0x%04X", -ret);
         goto cleanup;
     }
     
     *ciphertext_len = plaintext_len + ECIES_ENCRYPTION_OVERHEAD;
     success = true;
 
 cleanup:
     /* Securely erase sensitive data */
     mbedtls_platform_zeroize(ephemeral_privkey, sizeof(ephemeral_privkey));
     mbedtls_platform_zeroize(shared_secret, sizeof(shared_secret));
     mbedtls_platform_zeroize(aes_key, sizeof(aes_key));
     mbedtls_gcm_free(&gcm_ctx);
     
     return success;
 }
 
 bool ecies_decrypt(const uint8_t *ciphertext,
                    size_t ciphertext_len,
                    const uint8_t recipient_privkey[ECIES_X25519_KEY_SIZE],
                    uint8_t *plaintext,
                    size_t *plaintext_len)
 {
     if (ciphertext == NULL || recipient_privkey == NULL || 
         plaintext == NULL || plaintext_len == NULL) {
         return false;
     }
     
     /* Validate minimum size */
     if (ciphertext_len < ECIES_ENCRYPTION_OVERHEAD) {
         ESP_LOGE(TAG, "Ciphertext too short");
         return false;
     }
     
     bool success = false;
     mbedtls_gcm_context gcm_ctx;
     mbedtls_gcm_init(&gcm_ctx);
     
     /* Allocate buffers for shared secret and derived key */
     uint8_t shared_secret[ECIES_X25519_KEY_SIZE];
     uint8_t aes_key[ECIES_AES_KEY_SIZE];
     
     /* Parse input: [ephemeral_pubkey || nonce || ciphertext || tag] */
     const uint8_t *in_ephemeral = ciphertext;
     const uint8_t *in_nonce = ciphertext + ECIES_X25519_KEY_SIZE;
     const uint8_t *in_ciphertext = ciphertext + ECIES_X25519_KEY_SIZE + ECIES_GCM_IV_SIZE;
     const uint8_t *in_tag = ciphertext + ciphertext_len - ECIES_GCM_TAG_SIZE;
     
     size_t encrypted_len = ciphertext_len - ECIES_ENCRYPTION_OVERHEAD;
     
     /* Step 1: Perform ECDH with ephemeral public key */
     if (!ecies_ecdh_x25519(recipient_privkey, in_ephemeral, shared_secret)) {
         ESP_LOGE(TAG, "ECDH failed");
         goto cleanup;
     }
     
     /* Step 2: Derive AES key using HKDF */
     if (!ecies_kdf(shared_secret, aes_key)) {
         ESP_LOGE(TAG, "KDF failed");
         goto cleanup;
     }
     
     /* Step 3: Setup GCM context */
     int ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES,
                                 aes_key, ECIES_AES_KEY_SIZE * 8);
     if (ret != 0) {
         ESP_LOGE(TAG, "GCM setkey failed: -0x%04X", -ret);
         goto cleanup;
     }
     
     /* Step 4: Decrypt and verify authentication tag */
     ret = mbedtls_gcm_auth_decrypt(&gcm_ctx,
                                   encrypted_len,
                                   in_nonce, ECIES_GCM_IV_SIZE,
                                   NULL, 0,  /* No additional authenticated data */
                                   in_tag, ECIES_GCM_TAG_SIZE,
                                   in_ciphertext,
                                   plaintext);
     
     if (ret != 0) {
         if (ret == MBEDTLS_ERR_GCM_AUTH_FAILED) {
             ESP_LOGE(TAG, "GCM authentication failed - data corrupted or wrong key");
         } else {
             ESP_LOGE(TAG, "GCM decryption failed: -0x%04X", -ret);
         }
         goto cleanup;
     }
     
     *plaintext_len = encrypted_len;
     success = true;
 
 cleanup:
     /* Securely erase sensitive data */
     mbedtls_platform_zeroize(shared_secret, sizeof(shared_secret));
     mbedtls_platform_zeroize(aes_key, sizeof(aes_key));
     mbedtls_gcm_free(&gcm_ctx);
     
     return success;
 }
 
 void ecies_secure_zero(void *buffer, size_t size)
 {
     if (buffer != NULL && size > 0) {
         mbedtls_platform_zeroize(buffer, size);
     }
 }
 