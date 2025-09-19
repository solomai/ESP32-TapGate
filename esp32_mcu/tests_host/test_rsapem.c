#include "unity.h"
#include "rsapem/rsapem.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_rsapem_rejects_null_keypair(void)
{
    TEST_ASSERT_EQUAL_INT(ESP_ERR_INVALID_ARG,
                          rsapem_generate_keypair(RSAPEM_DEFAULT_KEY_BITS, NULL));
}

void test_rsapem_rejects_invalid_key_size(void)
{
    rsapem_keypair_t keypair;
    TEST_ASSERT_EQUAL_INT(ESP_ERR_INVALID_ARG,
                          rsapem_generate_keypair(RSAPEM_MIN_KEY_BITS - 1, &keypair));
    TEST_ASSERT_EQUAL_INT(ESP_ERR_INVALID_ARG,
                          rsapem_generate_keypair(RSAPEM_MAX_KEY_BITS + 1, &keypair));
}

void test_rsapem_not_supported_on_host(void)
{
    rsapem_keypair_t keypair;
    TEST_ASSERT_EQUAL_INT(ESP_ERR_NOT_SUPPORTED,
                          rsapem_generate_keypair(RSAPEM_DEFAULT_KEY_BITS, &keypair));
    TEST_ASSERT_NULL(keypair.public_pem);
    TEST_ASSERT_NULL(keypair.private_pem);
    rsapem_free_keypair(&keypair);
}

void test_rsapem_free_handles_null(void)
{
    rsapem_free_keypair(NULL);

    rsapem_keypair_t keypair;
    keypair.public_pem = NULL;
    keypair.private_pem = NULL;
    keypair.public_pem_length = 123;
    keypair.private_pem_length = 456;
    rsapem_free_keypair(&keypair);
    TEST_ASSERT_NULL(keypair.public_pem);
    TEST_ASSERT_NULL(keypair.private_pem);
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)keypair.public_pem_length);
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)keypair.private_pem_length);
}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    RUN_TEST(test_rsapem_rejects_null_keypair);
    RUN_TEST(test_rsapem_rejects_invalid_key_size);
    RUN_TEST(test_rsapem_not_supported_on_host);
    RUN_TEST(test_rsapem_free_handles_null);
    return UNITY_END();
}

