#pragma once

#include <stddef.h>

#include "types.h"
#include "constants.h"

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ECIES_KEY_SIZE                 PUBPEM_CAP
#define ECIES_SHARED_SECRET_SIZE       32
#define ECIES_SYMMETRIC_KEY_SIZE       32
#define ECIES_NONCE_SIZE               12
#define ECIES_TAG_SIZE                 16

/**
 * @brief Generate a new X25519 key pair.
 *
 * The resulting private/public keys are suitable for both long-term and
 * ephemeral use.
 */
esp_err_t ecies_generate_keypair(private_key_t private_key, public_key_t public_key);

/**
 * @brief Derive an X25519 public key from the provided private key.
 */
esp_err_t ecies_derive_public_key(const private_key_t private_key, public_key_t public_key);

/**
 * @brief Encrypt a payload using ECIES (X25519 + HKDF-SHA256 + AES-GCM).
 *
 * @param peer_public_key   Recipient's long-term public key.
 * @param plaintext         Pointer to plaintext buffer.
 * @param plaintext_len     Length of plaintext in bytes.
 * @param ciphertext        Buffer where ciphertext will be written.
 * @param ciphertext_buf_len Capacity of @p ciphertext.
 * @param ciphertext_len    Output length of produced ciphertext.
 * @param eph_public_key    Output ephemeral public key that must be sent
 *                          alongside the ciphertext.
 * @param nonce             Output AES-GCM nonce (12 bytes).
 * @param tag               Output AES-GCM authentication tag (16 bytes).
 */
esp_err_t ecies_encode(const public_key_t peer_public_key,
                       const uint8_t *plaintext,
                       size_t plaintext_len,
                       uint8_t *ciphertext,
                       size_t ciphertext_buf_len,
                       size_t *ciphertext_len,
                       public_key_t eph_public_key,
                       uint8_t nonce[ECIES_NONCE_SIZE],
                       uint8_t tag[ECIES_TAG_SIZE]);

/**
 * @brief Decrypt a payload using ECIES (X25519 + HKDF-SHA256 + AES-GCM).
 *
 * @param private_key       Local long-term private key.
 * @param peer_eph_public   Ephemeral public key received with the message.
 * @param ciphertext        Ciphertext buffer to decrypt.
 * @param ciphertext_len    Length of ciphertext.
 * @param nonce             AES-GCM nonce used during encryption.
 * @param tag               AES-GCM authentication tag.
 * @param plaintext         Output buffer where decrypted data is stored.
 * @param plaintext_buf_len Capacity of @p plaintext buffer.
 * @param plaintext_len     Output plaintext length.
 */
esp_err_t ecies_decode(const private_key_t private_key,
                       const public_key_t peer_eph_public,
                       const uint8_t *ciphertext,
                       size_t ciphertext_len,
                       const uint8_t nonce[ECIES_NONCE_SIZE],
                       const uint8_t tag[ECIES_TAG_SIZE],
                       uint8_t *plaintext,
                       size_t plaintext_buf_len,
                       size_t *plaintext_len);

#ifdef __cplusplus
}
#endif