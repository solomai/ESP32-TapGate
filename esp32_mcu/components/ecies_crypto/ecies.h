#pragma once

/**
 * @file ecies.h
 * @brief ECIES (Elliptic Curve Integrated Encryption Scheme) module for ESP32
 * 
 * Implements ECIES-style encryption using:
 * - X25519 ECDH key exchange (RFC 7748)
 * - HKDF-SHA256 key derivation (RFC 5869)
 * - AES-256-GCM AEAD encryption (RFC 5116, NIST SP 800-38D)
 * 
 * Thread-safe, stateless operations using ESP-IDF 5.5 mbedTLS.
 */
 
 #include <stdint.h>
 #include <stddef.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* Key and algorithm sizes (bytes) */
 #define ECIES_X25519_KEY_SIZE       32  /**< X25519 private/public key size */
 #define ECIES_AES_KEY_SIZE          32  /**< AES-256 key size */
 #define ECIES_GCM_IV_SIZE           12  /**< GCM nonce/IV size (96 bits recommended) */
 #define ECIES_GCM_TAG_SIZE          16  /**< GCM authentication tag size (128 bits) */
 
 /**
  * @brief Overhead size added to plaintext during encryption
  * 
  * Encrypted packet structure:
  * [Ephemeral Public Key (32)] [IV/Nonce (12)] [Ciphertext (N)] [Auth Tag (16)]
  */
 #define ECIES_ENCRYPTION_OVERHEAD   (ECIES_X25519_KEY_SIZE + ECIES_GCM_IV_SIZE + ECIES_GCM_TAG_SIZE)
 
 /**
  * @brief Generate a new X25519 key pair
  * 
  * Uses hardware RNG (esp_random) for secure key generation.
  * Thread-safe, no internal state.
  * 
  * @param[out] private_key  Buffer for private key (32 bytes)
  * @param[out] public_key   Buffer for public key (32 bytes)
  * @return true on success, false on failure
  */
 bool ecies_generate_keypair(uint8_t private_key[ECIES_X25519_KEY_SIZE],
                             uint8_t public_key[ECIES_X25519_KEY_SIZE]);
 
 /**
  * @brief Encrypt data using ECIES
  * 
  * Performs:
  * 1. Generate ephemeral X25519 key pair
  * 2. Compute shared secret via ECDH with recipient's public key
  * 3. Derive AES-256 key using HKDF-SHA256
  * 4. Encrypt plaintext with AES-256-GCM
  * 5. Output: [ephemeral_pubkey || nonce || ciphertext || tag]
  * 
  * Thread-safe, stateless operation. Uses hardware RNG for ephemeral key and nonce.
  * 
  * @param[in]  plaintext          Input plaintext buffer
  * @param[in]  plaintext_len      Length of plaintext
  * @param[in]  recipient_pubkey   Recipient's X25519 public key (32 bytes)
  * @param[out] ciphertext         Output buffer (must be >= plaintext_len + ECIES_ENCRYPTION_OVERHEAD)
  * @param[out] ciphertext_len     Length of output ciphertext (including overhead)
  * @return true on success, false on failure
  */
 bool ecies_encrypt(const uint8_t *plaintext,
                    size_t plaintext_len,
                    const uint8_t recipient_pubkey[ECIES_X25519_KEY_SIZE],
                    uint8_t *ciphertext,
                    size_t *ciphertext_len);
 
 /**
  * @brief Decrypt data using ECIES
  * 
  * Performs:
  * 1. Extract ephemeral public key from ciphertext
  * 2. Compute shared secret using recipient's private key
  * 3. Derive AES-256 key using HKDF-SHA256
  * 4. Decrypt and verify using AES-256-GCM
  * 
  * Thread-safe, stateless operation.
  * 
  * @param[in]  ciphertext         Input ciphertext (format: [ephemeral_pubkey || nonce || encrypted || tag])
  * @param[in]  ciphertext_len     Length of ciphertext
  * @param[in]  recipient_privkey  Recipient's X25519 private key (32 bytes)
  * @param[out] plaintext          Output plaintext buffer (must be >= ciphertext_len - ECIES_ENCRYPTION_OVERHEAD)
  * @param[out] plaintext_len      Length of decrypted plaintext
  * @return true on success, false on authentication failure or error
  */
 bool ecies_decrypt(const uint8_t *ciphertext,
                    size_t ciphertext_len,
                    const uint8_t recipient_privkey[ECIES_X25519_KEY_SIZE],
                    uint8_t *plaintext,
                    size_t *plaintext_len);
 
 /**
  * @brief Compute X25519 public key from private key
  * 
  * Useful for deriving public key from stored private key.
  * Thread-safe, stateless.
  * 
  * @param[in]  private_key  X25519 private key (32 bytes)
  * @param[out] public_key   X25519 public key (32 bytes)
  * @return true on success, false on failure
  */
 bool ecies_compute_public_key(const uint8_t private_key[ECIES_X25519_KEY_SIZE],
                               uint8_t public_key[ECIES_X25519_KEY_SIZE]);
 
 /**
  * @brief Securely erase sensitive data from memory
  * 
  * Uses mbedtls_platform_zeroize to prevent compiler optimization.
  * 
  * @param[in] buffer  Buffer to clear
  * @param[in] size    Size of buffer
  */
 void ecies_secure_zero(void *buffer, size_t size);
 
 #ifdef __cplusplus
 }
 #endif
 