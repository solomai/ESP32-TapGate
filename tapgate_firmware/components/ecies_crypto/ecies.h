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
 * Uses PSA Crypto API (ESP-IDF 6.0 / mbedTLS 4.x).
 * Thread-safe, stateless operations.
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
#define ECIES_GCM_IV_SIZE           12  /**< GCM nonce/IV size (96-bit recommended) */
#define ECIES_GCM_TAG_SIZE          16  /**< GCM authentication tag size (128-bit) */

/**
 * @brief Overhead added to plaintext during encryption
 *
 * Packet structure:
 *   [Ephemeral Public Key (32)] [IV/Nonce (12)] [Ciphertext (N)] [Auth Tag (16)]
 */
#define ECIES_ENCRYPTION_OVERHEAD   (ECIES_X25519_KEY_SIZE + ECIES_GCM_IV_SIZE + ECIES_GCM_TAG_SIZE)

/** @brief Maximum allowed plaintext size (protocol policy limit) */
#define ECIES_MAX_PLAINTEXT_SIZE    8192

/**
 * @brief Generate a new X25519 key pair
 *
 * Uses PSA hardware-backed RNG. Keys are 32-byte per RFC 7748 / PSA convention.
 * Thread-safe, stateless.
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
 * Output format: [ephemeral_pubkey(32) || nonce(12) || ciphertext(N) || tag(16)]
 *
 * @param[in]  plaintext            Input plaintext
 * @param[in]  plaintext_len        Length (must be > 0 and <= ECIES_MAX_PLAINTEXT_SIZE)
 * @param[in]  recipient_pubkey     Recipient's X25519 public key (32 bytes)
 * @param[out] ciphertext           Output buffer (must be >= plaintext_len + ECIES_ENCRYPTION_OVERHEAD)
 * @param[in]  ciphertext_capacity  Size of output buffer
 * @param[out] ciphertext_len       Actual output length
 * @return true on success, false on failure
 */
bool ecies_encrypt(const uint8_t *plaintext,
                   size_t         plaintext_len,
                   const uint8_t  recipient_pubkey[ECIES_X25519_KEY_SIZE],
                   uint8_t       *ciphertext,
                   size_t         ciphertext_capacity,
                   size_t        *ciphertext_len);

/**
 * @brief Decrypt data using ECIES
 *
 * Input format: [ephemeral_pubkey(32) || nonce(12) || ciphertext(N) || tag(16)]
 *
 * @param[in]  ciphertext          Input (must be >= ECIES_ENCRYPTION_OVERHEAD)
 * @param[in]  ciphertext_len      Input length
 * @param[in]  recipient_privkey   Recipient's X25519 private key (32 bytes)
 * @param[out] plaintext           Output plaintext buffer
 * @param[in]  plaintext_capacity  Size of output buffer
 * @param[out] plaintext_len       Actual decrypted length
 * @return true on success, false on authentication failure or error
 */
bool ecies_decrypt(const uint8_t *ciphertext,
                   size_t         ciphertext_len,
                   const uint8_t  recipient_privkey[ECIES_X25519_KEY_SIZE],
                   uint8_t       *plaintext,
                   size_t         plaintext_capacity,
                   size_t        *plaintext_len);

/**
 * @brief Compute X25519 public key from private key
 *
 * @param[in]  private_key  X25519 private key (32 bytes)
 * @param[out] public_key   X25519 public key (32 bytes)
 * @return true on success, false on failure
 */
bool ecies_compute_public_key(const uint8_t private_key[ECIES_X25519_KEY_SIZE],
                               uint8_t       public_key[ECIES_X25519_KEY_SIZE]);

/**
 * @brief Securely erase sensitive data from memory
 *
 * Uses a compiler barrier to prevent optimization of the memset.
 *
 * @param[in] buffer  Buffer to clear
 * @param[in] size    Size of buffer
 */
void ecies_secure_zero(void *buffer, size_t size);

#ifdef __cplusplus
}
#endif