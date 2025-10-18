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

static const uint8_t host_private_key[] = {
    0x50, 0xEF, 0xF6, 0x34, 0xC2, 0xB2, 0x3F, 0x8A,
    0xF0, 0x4E, 0xDD, 0x5D, 0x58, 0x40, 0x2A, 0x48,
    0x6B, 0x67, 0xF5, 0xCF, 0x68, 0x56, 0x53, 0x00,
    0xED, 0x8F, 0x40, 0x80, 0x8F, 0x70, 0x27, 0x6E
};

static const uint8_t host_public_key[] = {
    0xD6, 0x6A, 0x0A, 0xFC, 0x1A, 0x75, 0xC7, 0x64,
    0xB1, 0x75, 0xC5, 0xEC, 0x04, 0x92, 0xA3, 0xF6,
    0x23, 0x74, 0x39, 0xDB, 0x21, 0xC1, 0xF2, 0xC6,
    0xCE, 0xA4, 0x34, 0xFC, 0x49, 0x3A, 0x56, 0x06
};

static const uint8_t client_public_key[] = {
    0xDD, 0xBD, 0x81, 0xCE, 0xEC, 0xCE, 0x0E, 0xC9,
    0x31, 0x40, 0x27, 0xEE, 0xDE, 0x90, 0x5B, 0x5B,
    0x30, 0x60, 0xD2, 0x7C, 0xA1, 0xFD, 0x5E, 0x1E,
    0x1B, 0x95, 0x9F, 0x9B, 0xD7, 0x7D, 0xFC, 0x4B
};

void setUp(void) 
{
    /* Setup code runs before each test */
}

void tearDown(void) 
{
    /* Cleanup code runs after each test */
}

// Test: Encrypt and decrypt a test natively generated message
void test_ecies_encrypt_decrypt_self_generated_message(void)
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
    uint8_t ciphertext[128];
    size_t ciphertext_len = 0;
    
    result = ecies_encrypt((const uint8_t *)plaintext, plaintext_len,
                          host_public_key, ciphertext, sizeof(ciphertext), &ciphertext_len);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT(ciphertext_len > plaintext_len);
    TEST_ASSERT_EQUAL_INT(plaintext_len + ECIES_ENCRYPTION_OVERHEAD, ciphertext_len);
    
    /* Host decrypts ciphertext with its private key */
    uint8_t decrypted_plaintext[128];
    size_t decrypted_len = 0;
    
    result = ecies_decrypt(ciphertext, ciphertext_len, host_private_key,
                          decrypted_plaintext, sizeof(decrypted_plaintext), &decrypted_len);
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

// Test: Decrypt message encoded on client (MAUI) with host_public_key
void test_ecies_encrypt_decrypt_client_generated_message(void)
{
    // Message as byte array (95 bytes)
    // Generated with MAUI ecies_encrypt using host_public_key
    // message: "Test message encoded on MAUI client"
    const uint8_t message[] = {
        0x98, 0x76, 0xF8, 0x7E, 0xC6, 0x84, 0x39, 0xCE, 
        0xC3, 0xE1, 0x51, 0xCC, 0x0A, 0xFA, 0xA6, 0x1B, 
        0x4D, 0xEE, 0x2D, 0x22, 0xFA, 0x85, 0x7B, 0xB2, 
        0xB6, 0x81, 0xBA, 0x87, 0x3A, 0x91, 0x12, 0x5F, 
        0x8F, 0xC9, 0x6B, 0xD5, 0x3C, 0xA3, 0x2C, 0x61, 
        0x98, 0x92, 0x6E, 0xED, 0xD7, 0x6E, 0x99, 0x89, 
        0x9C, 0x8D, 0x60, 0x95, 0x61, 0xC7, 0x6E, 0xA4, 
        0xA3, 0xFC, 0x9E, 0x38, 0xC7, 0x8A, 0xE4, 0x16, 
        0xAA, 0xD0, 0xD6, 0x82, 0x53, 0x80, 0x47, 0x09, 
        0xFB, 0x73, 0xF1, 0xED, 0xAB, 0xE0, 0x1C, 0x10, 
        0xFC, 0x44, 0xD8, 0x23, 0x4B, 0xD5, 0x67, 0x59, 
        0xA2, 0x2A, 0x5C, 0x2C, 0xB5, 0x5F, 0xF3
    };
    const size_t message_len = sizeof(message);

    const char *expected_plaintext = "Test message encoded on MAUI client";
    // Decrypt message in test
    uint8_t decrypted_plaintext[35];
    size_t decrypted_len = 0;
    
    bool result = ecies_decrypt(message, message_len, host_private_key,
                          decrypted_plaintext, sizeof(decrypted_plaintext), &decrypted_len);
    TEST_ASSERT_TRUE(result);
    
    /* Verify decrypted plaintext matches original */
    size_t expected_len = strlen(expected_plaintext);
    TEST_ASSERT_EQUAL_INT(expected_len, decrypted_len);
    TEST_ASSERT(memcmp(expected_plaintext, decrypted_plaintext, expected_len) == 0);
  
    printf("Test 2 passed: Host-generated message encrypted and decrypted successfully\n");
    printf("  Decrypted: \"%s\"\n", (const char *)decrypted_plaintext);
}

/**
 * Main test runner
 */
int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();

    RUN_TEST(test_ecies_encrypt_decrypt_self_generated_message);
    RUN_TEST(test_ecies_encrypt_decrypt_client_generated_message);

    return UNITY_END();
}
