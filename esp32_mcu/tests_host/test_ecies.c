/**
 * @file test_ecies.c
 * @brief Unit tests for ECIES encryption module
 * 
 * Tests encryption and decryption between two parties using X25519 ECDH
 * and AES-256-GCM.
 */

#include "unity.h"
#include "ecies_crypto/ecies.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void setUp(void) 
{
    /* Setup code runs before each test */
}

void tearDown(void) 
{
    /* Cleanup code runs after each test */
}

/**
 * Test 1: Encryption–Decryption Between Two Parties
 * 
 * This test verifies that a message encrypted by the client can be 
 * successfully decrypted by the host. The host generates a key pair,
 * the client encrypts a plaintext message using the host's public key,
 * and the host decrypts the ciphertext using its private key.
 * The decrypted plaintext must match the original message.
 */
void test_ecies_encrypt_decrypt_text_message(void)
{
    /* Host generates key pair */
    uint8_t host_private_key[ECIES_X25519_KEY_SIZE];
    uint8_t host_public_key[ECIES_X25519_KEY_SIZE];
    
    bool result = ecies_generate_keypair(host_private_key, host_public_key);
    TEST_ASSERT_TRUE(result);
    
    /* Client prepares plaintext message */
    const char *plaintext = "CLIENT PLAINT TEXT";
    size_t plaintext_len = strlen(plaintext);
    
    /* Client encrypts message with host's public key */
    uint8_t ciphertext[512];
    size_t ciphertext_len = 0;
    
    result = ecies_encrypt((const uint8_t *)plaintext, plaintext_len,
                          host_public_key, ciphertext, &ciphertext_len);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT(ciphertext_len > plaintext_len);
    TEST_ASSERT_EQUAL_INT(plaintext_len + ECIES_ENCRYPTION_OVERHEAD, ciphertext_len);
    
    /* Host decrypts ciphertext with its private key */
    uint8_t decrypted_plaintext[512];
    size_t decrypted_len = 0;
    
    result = ecies_decrypt(ciphertext, ciphertext_len, host_private_key,
                          decrypted_plaintext, &decrypted_len);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(plaintext_len, decrypted_len);
    
    /* Verify decrypted plaintext matches original */
    TEST_ASSERT(memcmp(plaintext, decrypted_plaintext, plaintext_len) == 0);
    
    /* Null-terminate and verify string content */
    decrypted_plaintext[decrypted_len] = '\0';
    TEST_ASSERT_EQUAL_STRING(plaintext, (const char *)decrypted_plaintext);
    
    printf("Test 1 passed: Text message encrypted and decrypted successfully\n");
    printf("  Original:  \"%s\"\n", plaintext);
    printf("  Decrypted: \"%s\"\n", (const char *)decrypted_plaintext);
}

/**
 * Test 2: Encryption of Binary Data
 * 
 * This test verifies that binary data can be encrypted and decrypted correctly.
 * The host generates a key pair, the client generates a 512-byte binary buffer,
 * encrypts it using the host's public key, and the host decrypts it using its
 * private key. The decrypted buffer must match the original binary data.
 */
void test_ecies_encrypt_decrypt_binary_data(void)
{
    /* Host generates key pair */
    uint8_t host_private_key[ECIES_X25519_KEY_SIZE];
    uint8_t host_public_key[ECIES_X25519_KEY_SIZE];
    
    bool result = ecies_generate_keypair(host_private_key, host_public_key);
    TEST_ASSERT_TRUE(result);
    
    /* Client prepares binary buffer (512 bytes) */
    const size_t buffer_size = 512;
    uint8_t *plainbuffer = malloc(buffer_size);
    TEST_ASSERT_NOT_NULL(plainbuffer);
    
    /* Fill buffer with test pattern */
    for (size_t i = 0; i < buffer_size; i++) {
        plainbuffer[i] = (uint8_t)(i & 0xFF);
    }
    
    /* Client encrypts binary data with host's public key */
    uint8_t *ciphertext = malloc(buffer_size + ECIES_ENCRYPTION_OVERHEAD);
    TEST_ASSERT_NOT_NULL(ciphertext);
    size_t ciphertext_len = 0;
    
    result = ecies_encrypt(plainbuffer, buffer_size, host_public_key,
                          ciphertext, &ciphertext_len);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(buffer_size + ECIES_ENCRYPTION_OVERHEAD, ciphertext_len);
    
    /* Host decrypts ciphertext with its private key */
    uint8_t *decrypted_plainbuffer = malloc(buffer_size);
    TEST_ASSERT_NOT_NULL(decrypted_plainbuffer);
    size_t decrypted_len = 0;
    
    result = ecies_decrypt(ciphertext, ciphertext_len, host_private_key,
                          decrypted_plainbuffer, &decrypted_len);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(buffer_size, decrypted_len);
    
    /* Verify decrypted binary data matches original */
    TEST_ASSERT(memcmp(plainbuffer, decrypted_plainbuffer, buffer_size) == 0);
    
    printf("Test 2 passed: Binary data (512 bytes) encrypted and decrypted successfully\n");
    
    /* Display first and last 16 bytes for verification */
    printf("  Original first 16 bytes:  ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", plainbuffer[i]);
    }
    printf("\n");
    
    printf("  Decrypted first 16 bytes: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", decrypted_plainbuffer[i]);
    }
    printf("\n");
    
    printf("  Original last 16 bytes:   ");
    for (int i = buffer_size - 16; i < (int)buffer_size; i++) {
        printf("%02X ", plainbuffer[i]);
    }
    printf("\n");
    
    printf("  Decrypted last 16 bytes:  ");
    for (int i = buffer_size - 16; i < (int)buffer_size; i++) {
        printf("%02X ", decrypted_plainbuffer[i]);
    }
    printf("\n");
    
    /* Clean up allocated memory */
    free(plainbuffer);
    free(ciphertext);
    free(decrypted_plainbuffer);
}

/**
 * Main test runner
 */
int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    
    RUN_TEST(test_ecies_encrypt_decrypt_text_message);
    RUN_TEST(test_ecies_encrypt_decrypt_binary_data);
    
    return UNITY_END();
}
