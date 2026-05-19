#include "unity.h"
#include "test_utils.h"
#include "constants.h"
#include "types.h"
#include "uuid.h"

// UUIDv4 RFC 4122 masks
static constexpr uint8_t UUID_VERSION_MASK  = 0xF0u;
static constexpr uint8_t UUID_VERSION_VALUE = 0x40u; // version 4
static constexpr uint8_t UUID_VARIANT_MASK  = 0xC0u;
static constexpr uint8_t UUID_VARIANT_VALUE = 0x80u; // variant 1 (RFC 4122)

TEST_CASE("generate_rng_uid_NullBuffer_ReturnsInvalidArg", "[uuid]")
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, generate_rng_uid(nullptr));
}

TEST_CASE("generate_rng_uid_ValidBuffer_ReturnsOk", "[uuid]")
{
    tg_uid_t uid = {};
    TEST_ASSERT_EQUAL(ESP_OK, generate_rng_uid(uid));
}

TEST_CASE("generate_rng_uid_ValidBuffer_UUIDv4VersionBitsCorrect", "[uuid]")
{
    tg_uid_t uid = {};
    TEST_ASSERT_EQUAL(ESP_OK, generate_rng_uid(uid));
    TEST_ASSERT_EQUAL_HEX8(UUID_VERSION_VALUE, uid[6] & UUID_VERSION_MASK);
}

TEST_CASE("generate_rng_uid_ValidBuffer_RFC4122VariantBitsCorrect", "[uuid]")
{
    tg_uid_t uid = {};
    TEST_ASSERT_EQUAL(ESP_OK, generate_rng_uid(uid));
    TEST_ASSERT_EQUAL_HEX8(UUID_VARIANT_VALUE, uid[8] & UUID_VARIANT_MASK);
}

TEST_CASE("generate_rng_uid_TwoCalls_ProduceDifferentValues", "[uuid]")
{
    tg_uid_t uid_a = {};
    tg_uid_t uid_b = {};

    TEST_ASSERT_EQUAL(ESP_OK, generate_rng_uid(uid_a));
    TEST_ASSERT_EQUAL(ESP_OK, generate_rng_uid(uid_b));

    // With UID_CAP * 8 bits of entropy the probability of a collision is negligible
    bool are_equal = true;
    for (size_t i = 0; i < UID_CAP; ++i) {
        if (uid_a[i] != uid_b[i]) {
            are_equal = false;
            break;
        }
    }
    TEST_ASSERT_FALSE_MESSAGE(are_equal, "Two consecutive UIDs must not be identical");
}

TEST_CASE("generate_rng_uid_ValidBuffer_OutputIsNotAllZeros", "[uuid]")
{
    tg_uid_t uid = {};
    TEST_ASSERT_EQUAL(ESP_OK, generate_rng_uid(uid));

    bool all_zero = true;
    for (size_t i = 0; i < UID_CAP; ++i) {
        if (uid[i] != 0u) {
            all_zero = false;
            break;
        }
    }
    TEST_ASSERT_FALSE_MESSAGE(all_zero, "UID output must not be all zeros");
}
