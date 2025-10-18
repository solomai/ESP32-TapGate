/**
 * @file ecies.c
 * @brief ECIES implementation using ESP-IDF 5.5 mbedTLS
 */

 #include "ecies.h"
 #include "mbedtls_compat.h"
 #include <string.h>
 #include "esp_random.h"
 #include "esp_log.h"
 #include "mbedtls/ecp.h"
 #include "mbedtls/gcm.h"
 #include "mbedtls/hkdf.h"
 #include "mbedtls/md.h"
 #include "mbedtls/error.h"
 #include "mbedtls/platform_util.h"
 
 static const char *TAG = "ECIES";
 
 /* HKDF info string for domain separation */
 static const uint8_t HKDF_INFO[] = "ECIES-AES256-GCM";
 static const size_t HKDF_INFO_LEN = sizeof(HKDF_INFO) - 1;
 
 /**
  * @brief Reverse bytes in buffer (LE <-> BE conversion)
  */
 static void reverse_bytes(uint8_t *dst, const uint8_t *src, size_t len)
 {
     for (size_t i = 0; i < len; i++) {
         dst[i] = src[len - 1 - i];
     }
 }
 
 /**
  * @brief Clamp X25519 private key per RFC 7748
  * Sets bits: clear bit 0, clear bit 255, clear bits 1-2, set bit 254
  */
 static void clamp_x25519_privkey(uint8_t key[ECIES_X25519_KEY_SIZE])
 {
     key[0] &= 248;      /* Clear bits 0, 1, 2 */
     key[31] &= 127;     /* Clear bit 255 */
     key[31] |= 64;      /* Set bit 254 */
 }
 
 /**
  * @brief Check if buffer contains all zeros (weak shared secret detection)
  */
 static bool is_all_zero(const uint8_t *buffer, size_t len)
 {
     uint8_t result = 0;
     for (size_t i = 0; i < len; i++) {
         result |= buffer[i];
     }
     return (result == 0);
 }
 
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
 * @brief Log mbedTLS error with human-readable description
 */
static void log_mbedtls_error(const char *operation, int error_code)
{
    char error_buf[128];
    mbedtls_strerror(error_code, error_buf, sizeof(error_buf));
    ESP_LOGE(TAG, "%s failed: -0x%04X ('%s')", operation, -error_code, error_buf);
}

/**
 * @brief Cleanup mbedTLS structures for ECDH
 */
static void cleanup_ecdh_structures(mbedtls_ecp_group *grp, mbedtls_mpi *d, 
                                   mbedtls_ecp_point *Q)
{
    mbedtls_ecp_group_free(grp);
    mbedtls_mpi_free(d);
    mbedtls_ecp_point_free(Q);
}

/**
 * @brief Perform X25519 ECDH and derive shared secret
 * Using low-level ECP API for mbedTLS 3.x compatibility
 * 
 * All keys on API boundary are little-endian (LE) per RFC 7748.
 * Internal mbedTLS operations use big-endian (BE).
 * 
 * @param our_privkey   32-byte LE private scalar
 * @param their_pubkey  32-byte LE public key (X coordinate)
 * @param shared_secret 32-byte LE output shared secret
 * @return true on success, false on error or weak shared secret (all-zero)
 */
static bool ecies_ecdh_x25519(const uint8_t our_privkey[ECIES_X25519_KEY_SIZE],
                              const uint8_t their_pubkey[ECIES_X25519_KEY_SIZE],
                              uint8_t shared_secret[ECIES_X25519_KEY_SIZE])
{
    mbedtls_ecp_group grp;
    mbedtls_mpi d;       // Our private key
    mbedtls_ecp_point Q; // Their public key
    
    // Initialize all structures
    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_ecp_point_init(&Q);
    
    // Temporary buffers for LE<->BE conversion
    uint8_t privkey_be[ECIES_X25519_KEY_SIZE];
    uint8_t pubkey_be[ECIES_X25519_KEY_SIZE];
    uint8_t secret_be[ECIES_X25519_KEY_SIZE];
    
    int ret = 0;
    
    // Load Curve25519
    if (ret == 0) {
        ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
        if (ret != 0) {
            log_mbedtls_error("Load X25519 curve", ret);
        }
    }
    
    // Convert our private key from LE to BE for mbedTLS
    if (ret == 0) {
        reverse_bytes(privkey_be, our_privkey, ECIES_X25519_KEY_SIZE);
        ret = mbedtls_mpi_read_binary(&d, privkey_be, ECIES_X25519_KEY_SIZE);
        if (ret != 0) {
            log_mbedtls_error("Load private key", ret);
        }
    }
    
    // Convert their public key from LE to BE for mbedTLS
    if (ret == 0) {
        reverse_bytes(pubkey_be, their_pubkey, ECIES_X25519_KEY_SIZE);
        ret = mbedtls_mpi_read_binary(&Q.MBEDTLS_PRIVATE(X), pubkey_be, ECIES_X25519_KEY_SIZE);
        if (ret != 0) {
            log_mbedtls_error("Load public key", ret);
        }
    }
    
    // Set Z coordinate to 1 for Montgomery curve
    if (ret == 0) {
        ret = mbedtls_mpi_lset(&Q.MBEDTLS_PRIVATE(Z), 1);
        if (ret != 0) {
            log_mbedtls_error("Set Z coordinate", ret);
        }
    }
    
    // Compute shared secret: z = d * Q
    if (ret == 0) {
        ret = mbedtls_ecp_mul(&grp, &Q, &d, &Q, ecies_rng, NULL);
        if (ret != 0) {
            log_mbedtls_error("ECDH compute", ret);
        }
    }
    
    // Export shared secret (X coordinate of result point) - comes out in BE
    if (ret == 0) {
        ret = mbedtls_mpi_write_binary(&Q.MBEDTLS_PRIVATE(X), secret_be, ECIES_X25519_KEY_SIZE);
        if (ret != 0) {
            log_mbedtls_error("Export shared secret", ret);
        }
    }
    
    // Convert shared secret from BE to LE for API boundary
    if (ret == 0) {
        reverse_bytes(shared_secret, secret_be, ECIES_X25519_KEY_SIZE);
        
        // SECURITY: Check for all-zero shared secret (cofactor attack / weak key)
        if (is_all_zero(shared_secret, ECIES_X25519_KEY_SIZE)) {
            ESP_LOGE(TAG, "ECDH produced all-zero shared secret - AUTHENTICATION FAILURE");
            mbedtls_platform_zeroize(shared_secret, ECIES_X25519_KEY_SIZE);
            ret = -1;
        }
    }
    
    // Cleanup - zeroize temporary BE buffers containing secrets
    mbedtls_platform_zeroize(privkey_be, sizeof(privkey_be));
    mbedtls_platform_zeroize(secret_be, sizeof(secret_be));
    cleanup_ecdh_structures(&grp, &d, &Q);
    
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
         log_mbedtls_error("HKDF", ret);
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
     
     // Temporary buffers for BE output from mbedTLS
     uint8_t privkey_be[ECIES_X25519_KEY_SIZE];
     uint8_t pubkey_be[ECIES_X25519_KEY_SIZE];
     
     int ret = 0;
     
     /* Load Curve25519 */
     if (ret == 0) {
         ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
         if (ret != 0) {
            log_mbedtls_error("Load curve", ret);
         }
     }
     
     /* Generate key pair: Q = d * G (mbedTLS handles clamping internally) */
     if (ret == 0) {
         ret = mbedtls_ecp_gen_keypair(&grp, &d, &Q, ecies_rng, NULL);
         if (ret != 0) {
            log_mbedtls_error("Generate keypair", ret);
         }
     }
     
     /* Export private key to BE buffer */
     if (ret == 0) {
         ret = mbedtls_mpi_write_binary(&d, privkey_be, ECIES_X25519_KEY_SIZE);
         if (ret != 0) {
            log_mbedtls_error("Export private key", ret);
         }
     }
     
     /* Export public key (X coordinate only for X25519) to BE buffer */
     if (ret == 0) {
         ret = mbedtls_mpi_write_binary(&Q.MBEDTLS_PRIVATE(X), pubkey_be, ECIES_X25519_KEY_SIZE);
         if (ret != 0) {
            log_mbedtls_error("Export public key", ret);
         }
     }
     
     /* Convert keys from BE to LE for API boundary (RFC 7748) */
     if (ret == 0) {
         reverse_bytes(private_key, privkey_be, ECIES_X25519_KEY_SIZE);
         reverse_bytes(public_key, pubkey_be, ECIES_X25519_KEY_SIZE);
     }
     
     /* Cleanup */
     mbedtls_platform_zeroize(privkey_be, sizeof(privkey_be));
     mbedtls_ecp_group_free(&grp);
     mbedtls_mpi_free(&d);
     mbedtls_ecp_point_free(&Q);
     
     /* Securely erase private key from output buffer if operation failed */
     if (ret != 0) {
         mbedtls_platform_zeroize(private_key, ECIES_X25519_KEY_SIZE);
     }
     
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
     
     // Temporary buffers for LE->BE conversion (input) and BE->LE (output)
     uint8_t privkey_clamped[ECIES_X25519_KEY_SIZE];
     uint8_t privkey_be[ECIES_X25519_KEY_SIZE];
     uint8_t pubkey_be[ECIES_X25519_KEY_SIZE];
     
     int ret = 0;
     
     /* Load Curve25519 */
     if (ret == 0) {
         ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
         if (ret != 0) {
            log_mbedtls_error("Load curve", ret);
         }
     }
     
     /* Clamp imported private key per RFC 7748 (input is LE) */
     if (ret == 0) {
         memcpy(privkey_clamped, private_key, ECIES_X25519_KEY_SIZE);
         clamp_x25519_privkey(privkey_clamped);
         
         // Convert clamped LE private key to BE for mbedTLS
         reverse_bytes(privkey_be, privkey_clamped, ECIES_X25519_KEY_SIZE);
         ret = mbedtls_mpi_read_binary(&d, privkey_be, ECIES_X25519_KEY_SIZE);
         if (ret != 0) {
            log_mbedtls_error("Load private key", ret);
         }
     }
     
     /* Compute public key Q = d * G */
     if (ret == 0) {
         ret = mbedtls_ecp_mul(&grp, &Q, &d, &grp.G, ecies_rng, NULL);
         if (ret != 0) {
            log_mbedtls_error("Compute public key", ret);
         }
     }
     
     /* Export public key (X coordinate) to BE buffer */
     if (ret == 0) {
         ret = mbedtls_mpi_write_binary(&Q.MBEDTLS_PRIVATE(X), pubkey_be, ECIES_X25519_KEY_SIZE);
         if (ret != 0) {
            log_mbedtls_error("Export public key", ret);
         }
     }
     
     /* Convert public key from BE to LE for API boundary (RFC 7748) */
     if (ret == 0) {
         reverse_bytes(public_key, pubkey_be, ECIES_X25519_KEY_SIZE);
     }
     
     /* Cleanup - zeroize temporary buffers containing private key */
     mbedtls_platform_zeroize(privkey_clamped, sizeof(privkey_clamped));
     mbedtls_platform_zeroize(privkey_be, sizeof(privkey_be));
     mbedtls_ecp_group_free(&grp);
     mbedtls_mpi_free(&d);
     mbedtls_ecp_point_free(&Q);
     
     return (ret == 0);
 }
 
 bool ecies_encrypt(const uint8_t *plaintext,
                    size_t plaintext_len,
                    const uint8_t recipient_pubkey[ECIES_X25519_KEY_SIZE],
                    uint8_t *ciphertext,
                    size_t ciphertext_capacity,
                    size_t *ciphertext_len)
 {
     if (plaintext == NULL || recipient_pubkey == NULL || 
         ciphertext == NULL || ciphertext_len == NULL) {
         return false;
     }
     
     /* Validate plaintext length */
     if (plaintext_len == 0 || plaintext_len > ECIES_MAX_PLAINTEXT_SIZE) {
         ESP_LOGE(TAG, "Invalid plaintext length: %zu (max %d)", plaintext_len, ECIES_MAX_PLAINTEXT_SIZE);
         return false;
     }
     
     /* Check buffer capacity BEFORE any operations */
     const size_t required_len = plaintext_len + ECIES_ENCRYPTION_OVERHEAD;
     if (ciphertext_capacity < required_len) {
         ESP_LOGE(TAG, "Insufficient ciphertext buffer: need %zu, have %zu", required_len, ciphertext_capacity);
         return false;
     }
     
     /* Prevent overflow in size calculation */
     if (plaintext_len > (SIZE_MAX - ECIES_ENCRYPTION_OVERHEAD)) {
         ESP_LOGE(TAG, "Plaintext length would cause overflow");
         return false;
     }
     
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
     
     bool success = true;
     
     /* Step 1: Generate ephemeral key pair */
     if (success) {
         success = ecies_generate_keypair(ephemeral_privkey, ephemeral_pubkey);
         if (!success) {
            ESP_LOGE(TAG, "Failed to generate ephemeral keypair");
         }
     }
     
     /* Step 2: Copy ephemeral public key to output */
     if (success) {
         memcpy(out_ephemeral, ephemeral_pubkey, ECIES_X25519_KEY_SIZE);
    }
     
     /* Step 3: Perform ECDH to get shared secret */
     if (success) {
         success = ecies_ecdh_x25519(ephemeral_privkey, recipient_pubkey, shared_secret);
         if (!success) {
             ESP_LOGE(TAG, "ECDH failed or weak shared secret detected");
         }
     }
     
     /* Step 4: Derive AES key using HKDF */
     if (success) {
         success = ecies_kdf(shared_secret, aes_key);
         if (!success) {
            ESP_LOGE(TAG, "KDF failed");
         }
     }
     
     /* Step 5: Generate random nonce */
     if (success) {
         esp_fill_random(nonce, ECIES_GCM_IV_SIZE);
         memcpy(out_nonce, nonce, ECIES_GCM_IV_SIZE);      
     }
     
     /* Step 6: Setup GCM context */
     if (success) {
         int ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, 
                                     aes_key, ECIES_AES_KEY_SIZE * 8);
         if (ret != 0) {
             log_mbedtls_error("GCM setkey", ret);
             success = false;
         }
     }
     
     /* Step 7: Encrypt and authenticate */
     if (success) {
         int ret = mbedtls_gcm_crypt_and_tag(&gcm_ctx,
                                        MBEDTLS_GCM_ENCRYPT,
                                        plaintext_len,
                                        nonce, ECIES_GCM_IV_SIZE,
                                        NULL, 0,  /* No additional authenticated data */
                                        plaintext,
                                        out_ciphertext,
                                        ECIES_GCM_TAG_SIZE,
                                        out_tag);
         
         if (ret != 0) {
             log_mbedtls_error("GCM encryption", ret);
             success = false;
         }
     }
     
     /* Set output length on success */
     if (success) {
         *ciphertext_len = required_len;
     }
     
     /* Cleanup - securely erase ALL sensitive data */
     mbedtls_platform_zeroize(ephemeral_privkey, sizeof(ephemeral_privkey));
     mbedtls_platform_zeroize(shared_secret, sizeof(shared_secret));
     mbedtls_platform_zeroize(aes_key, sizeof(aes_key));
     mbedtls_platform_zeroize(nonce, sizeof(nonce));  // Zeroize local nonce copy
     mbedtls_gcm_free(&gcm_ctx);
     
     return success;
 }
 
 bool ecies_decrypt(const uint8_t *ciphertext,
                    size_t ciphertext_len,
                    const uint8_t recipient_privkey[ECIES_X25519_KEY_SIZE],
                    uint8_t *plaintext,
                    size_t plaintext_capacity,
                    size_t *plaintext_len)
 {
     if (ciphertext == NULL || recipient_privkey == NULL || 
         plaintext == NULL || plaintext_len == NULL) {
         return false;
     }
     
     /* Validate minimum size */
     if (ciphertext_len < ECIES_ENCRYPTION_OVERHEAD) {
         ESP_LOGE(TAG, "Ciphertext too short: %zu bytes (min %d)", ciphertext_len, ECIES_ENCRYPTION_OVERHEAD);
         return false;
     }
     
     /* Calculate encrypted payload size */
     const size_t encrypted_len = ciphertext_len - ECIES_ENCRYPTION_OVERHEAD;
     
     /* Validate against protocol maximum */
     if (encrypted_len > ECIES_MAX_PLAINTEXT_SIZE) {
         ESP_LOGE(TAG, "Encrypted payload too large: %zu bytes (max %d)", encrypted_len, ECIES_MAX_PLAINTEXT_SIZE);
         return false;
     }
     
     /* Check plaintext buffer capacity BEFORE any operations */
     if (plaintext_capacity < encrypted_len) {
         ESP_LOGE(TAG, "Insufficient plaintext buffer: need %zu, have %zu", encrypted_len, plaintext_capacity);
         return false;
     }
     
     mbedtls_gcm_context gcm_ctx;
     mbedtls_gcm_init(&gcm_ctx);
     
     /* Allocate buffers for shared secret and derived key */
     uint8_t shared_secret[ECIES_X25519_KEY_SIZE];
     uint8_t aes_key[ECIES_AES_KEY_SIZE];
     uint8_t nonce_copy[ECIES_GCM_IV_SIZE];  // Local copy for zeroize
     
     /* Parse input: [ephemeral_pubkey || nonce || ciphertext || tag] */
     const uint8_t *in_ephemeral = ciphertext;
     const uint8_t *in_nonce = ciphertext + ECIES_X25519_KEY_SIZE;
     const uint8_t *in_ciphertext = ciphertext + ECIES_X25519_KEY_SIZE + ECIES_GCM_IV_SIZE;
     const uint8_t *in_tag = ciphertext + ciphertext_len - ECIES_GCM_TAG_SIZE;
     
     /* Copy nonce to local buffer for zeroization */
     memcpy(nonce_copy, in_nonce, ECIES_GCM_IV_SIZE);
     
     bool success = true;
     
     /* Step 1: Perform ECDH with ephemeral public key */
     if (success) {
        success = ecies_ecdh_x25519(recipient_privkey, in_ephemeral, shared_secret);
        if (!success) {
            ESP_LOGE(TAG, "ECDH failed or weak shared secret detected");
        }
     }
     
     /* Step 2: Derive AES key using HKDF */
     if (success) {
        success = ecies_kdf(shared_secret, aes_key);
        if (!success){
            ESP_LOGE(TAG, "KDF failed");
        }
     }
     
     /* Step 3: Setup GCM context */
     if (success) {
         int ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES,
                                     aes_key, ECIES_AES_KEY_SIZE * 8);
         if (ret != 0) {
             log_mbedtls_error("GCM setkey", ret);
             success = false;
         }
     }
     
     /* Step 4: Decrypt and verify authentication tag */
     if (success) {
         int ret = mbedtls_gcm_auth_decrypt(&gcm_ctx,
                                       encrypted_len,
                                       nonce_copy, ECIES_GCM_IV_SIZE,
                                       NULL, 0,  /* No additional authenticated data */
                                       in_tag, ECIES_GCM_TAG_SIZE,
                                       in_ciphertext,
                                       plaintext);
         
         if (ret != 0) {
             if (ret == MBEDTLS_ERR_GCM_AUTH_FAILED) {
                 ESP_LOGE(TAG, "GCM authentication failed - data corrupted or wrong key");
             } else {
                 log_mbedtls_error("GCM decryption", ret);
             }
             success = false;
         }
     }
     
     /* Set output length on success */
     if (success) {
         *plaintext_len = encrypted_len;
     }
     
     /* Cleanup - securely erase ALL sensitive data */
     mbedtls_platform_zeroize(shared_secret, sizeof(shared_secret));
     mbedtls_platform_zeroize(aes_key, sizeof(aes_key));
     mbedtls_platform_zeroize(nonce_copy, sizeof(nonce_copy));  // Zeroize local nonce copy
     mbedtls_gcm_free(&gcm_ctx);
     
     /* On failure, also zeroize any partial plaintext that was written */
     if (!success && plaintext != NULL) {
         mbedtls_platform_zeroize(plaintext, plaintext_capacity);
     }
     
     return success;
 }
 
 void ecies_secure_zero(void *buffer, size_t size)
 {
     if (buffer != NULL && size > 0) {
         mbedtls_platform_zeroize(buffer, size);
     }
 }
 