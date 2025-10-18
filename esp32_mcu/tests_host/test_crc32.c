/**
 * @file test_crc32.c
 * @brief Unit tests for CRC32 module
 */

#include "unity.h"
#include "crc32/crc32.h"
#include <string.h>

void setUp(void)
{
    /* Set up test fixtures if needed */
}

void tearDown(void)
{
    /* Clean up after tests if needed */
}

/**
 * @brief Test CRC32 with the standard reference string "123456789"
 * 
 * This test verifies that the CRC32 implementation produces the correct
 * checksum for the well-known test string "123456789".
 * Expected result: 0xCBF43926
 */
void test_crc32_self(void)
{
    const char *test_string = "123456789";
    const uint32_t expected_crc = 0xCBF43926U;
    
    uint32_t calculated_crc = crc32_calculate((const uint8_t *)test_string, strlen(test_string));
    
    TEST_ASSERT_EQUAL_UINT32(expected_crc, calculated_crc);
}

/**
 * @brief Test CRC32 initialization
 */
void test_crc32_init(void)
{
    uint32_t crc = crc32_init(0xFFFFFFFFU);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFU, crc);
    
    crc = crc32_init(0x00000000U);
    TEST_ASSERT_EQUAL_UINT32(0x00000000U, crc);
}

/**
 * @brief Test CRC32 with empty data
 */
void test_crc32_empty(void)
{
    uint32_t crc = crc32_init(0xFFFFFFFFU);
    crc = crc32_update(crc, NULL, 0);
    crc = crc32_finalize(crc);
    
    /* CRC32 of empty string should be 0x00000000 */
    TEST_ASSERT_EQUAL_UINT32(0x00000000U, crc);
}

/**
 * @brief Test CRC32 with NULL pointer
 */
void test_crc32_null_pointer(void)
{
    uint32_t crc = crc32_init(0xFFFFFFFFU);
    crc = crc32_update(crc, NULL, 100);
    
    /* Should return unchanged CRC */
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFU, crc);
}

/**
 * @brief Test CRC32 with single byte
 */
void test_crc32_single_byte(void)
{
    const uint8_t data[] = {0x00};
    uint32_t crc = crc32_calculate(data, 1);
    
    /* CRC32 of single byte 0x00 is 0xD202EF8D */
    TEST_ASSERT_EQUAL_UINT32(0xD202EF8DU, crc);
}

/**
 * @brief Test CRC32 incremental calculation
 * 
 * Verify that calculating CRC in multiple steps produces the same
 * result as calculating it all at once.
 */
void test_crc32_incremental(void)
{
    const char *test_string = "123456789";
    const size_t len = strlen(test_string);
    
    /* Calculate in one go */
    uint32_t crc_complete = crc32_calculate((const uint8_t *)test_string, len);
    
    /* Calculate incrementally */
    uint32_t crc_incremental = crc32_init(0xFFFFFFFFU);
    crc_incremental = crc32_update(crc_incremental, (const uint8_t *)"123", 3);
    crc_incremental = crc32_update(crc_incremental, (const uint8_t *)"456", 3);
    crc_incremental = crc32_update(crc_incremental, (const uint8_t *)"789", 3);
    crc_incremental = crc32_finalize(crc_incremental);
    
    TEST_ASSERT_EQUAL_UINT32(crc_complete, crc_incremental);
}

/**
 * @brief Test CRC32 with various data patterns
 */
void test_crc32_patterns(void)
{
    /* All zeros */
    const uint8_t zeros[10] = {0};
    uint32_t crc_zeros = crc32_calculate(zeros, 10);
    TEST_ASSERT_TRUE(crc_zeros != 0);  /* Should not be zero */
    
    /* All 0xFF */
    const uint8_t ones[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint32_t crc_ones = crc32_calculate(ones, 10);
    TEST_ASSERT_TRUE(crc_ones != 0);  /* Should not be zero */
    
    /* Different patterns should produce different CRCs */
    TEST_ASSERT_TRUE(crc_zeros != crc_ones);
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_crc32_self);
    RUN_TEST(test_crc32_init);
    RUN_TEST(test_crc32_empty);
    RUN_TEST(test_crc32_null_pointer);
    RUN_TEST(test_crc32_single_byte);
    RUN_TEST(test_crc32_incremental);
    RUN_TEST(test_crc32_patterns);
    
    return UNITY_END();
}
