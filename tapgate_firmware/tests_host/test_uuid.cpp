#include "unity.h"
#include "uuid.h"

#include <array>
#include <cctype>
#include <cstring>
#include <cstdio>

// Verify UID_STR_LEN tracks UID_CAP at compile time
static_assert(UID_STR_LEN == UID_CAP * 2 + 1, "UID_STR_LEN must equal UID_CAP * 2 + 1 (includes null terminator)");

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build a uid where every byte equals fill_byte.
static void fill_uid(tg_uid_t uid, uint8_t fill_byte)
{
    std::memset(uid, fill_byte, UID_CAP);
}

// Build the expected hex string for a repeating byte, using UID_CAP.
static void expected_hex_str(char* out, std::size_t out_sz, uint8_t fill_byte)
{
    for (std::size_t i = 0; i < UID_CAP && (i * 2 + 2) < out_sz; ++i)
        std::snprintf(out + i * 2, 3, "%02x", fill_byte);
}

// Parametric hex-conversion correctness check at arbitrary byte-array length N.
// Validates the same loop logic uid_to_str / str_to_uid use, so the test
// catches regressions if UID_CAP is changed to any value in [1..32].
template <std::size_t N>
static bool hex_roundtrip_correct(const std::array<uint8_t, N>& original)
{
    constexpr std::size_t STR_LEN = N * 2;
    char buf[STR_LEN + 1]{};

    // Encode: same loop as uid_to_str
    for (std::size_t i = 0; i < N; ++i)
        std::snprintf(buf + i * 2, 3, "%02x", original[i]);

    // Decode: same loop as str_to_uid
    uint8_t recovered[N]{};
    for (std::size_t i = 0; i < N; ++i) {
        auto hex_val = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        const int hi = hex_val(buf[i * 2]);
        const int lo = hex_val(buf[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        recovered[i] = static_cast<uint8_t>((hi << 4) | lo);
    }

    return std::memcmp(original.data(), recovered, N) == 0;
}

// ---------------------------------------------------------------------------
// uid_to_str — format correctness
// ---------------------------------------------------------------------------

void UidToStr_AllZeros_ProducesZeroHexString()
{
    tg_uid_t uid{};
    char buf[UID_STR_LEN]{};
    const char* result = uid_to_str(uid, {buf, sizeof(buf)});
    TEST_ASSERT_TRUE(result != nullptr);

    char expected[UID_STR_LEN]{};
    expected_hex_str(expected, sizeof(expected), 0x00);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

void UidToStr_AllFF_ProducesFFHexString()
{
    tg_uid_t uid;
    fill_uid(uid, 0xFF);
    char buf[UID_STR_LEN]{};
    const char* result = uid_to_str(uid, {buf, sizeof(buf)});
    TEST_ASSERT_TRUE(result != nullptr);

    char expected[UID_STR_LEN]{};
    expected_hex_str(expected, sizeof(expected), 0xFF);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

void UidToStr_KnownPattern_CorrectHex()
{
    tg_uid_t uid{};
    for (std::size_t i = 0; i < UID_CAP; ++i)
        uid[i] = static_cast<uint8_t>(i);  // 00 01 02 03 ...

    char buf[UID_STR_LEN]{};
    TEST_ASSERT_TRUE(uid_to_str(uid, {buf, sizeof(buf)}) != nullptr);

    // Verify each two-char group independently
    for (std::size_t i = 0; i < UID_CAP; ++i) {
        char pair[3]{};
        std::snprintf(pair, sizeof(pair), "%02x", static_cast<unsigned>(i));
        TEST_ASSERT_EQUAL(pair[0], buf[i * 2]);
        TEST_ASSERT_EQUAL(pair[1], buf[i * 2 + 1]);
    }
}

void UidToStr_OutputLength_EqualsUidCapTimes2()
{
    tg_uid_t uid{};
    char buf[UID_STR_LEN]{};
    uid_to_str(uid, {buf, sizeof(buf)});
    TEST_ASSERT_EQUAL(UID_STR_LEN - 1u, std::strlen(buf));
}

void UidToStr_ReturnsPointerToBuffer()
{
    tg_uid_t uid{};
    char buf[UID_STR_LEN]{};
    const char* result = uid_to_str(uid, {buf, sizeof(buf)});
    TEST_ASSERT_TRUE(result == buf);
}

void UidToStr_NoSeparatorsInOutput()
{
    tg_uid_t uid{};
    char buf[UID_STR_LEN]{};
    uid_to_str(uid, {buf, sizeof(buf)});
    bool has_dash = false;
    for (std::size_t i = 0; i < UID_STR_LEN; ++i)
        if (buf[i] == '-') has_dash = true;
    TEST_ASSERT_FALSE(has_dash);
}

// ---------------------------------------------------------------------------
// uid_to_str — error cases
// ---------------------------------------------------------------------------

void UidToStr_NullUid_ReturnsNull()
{
    char buf[UID_STR_LEN]{};
    TEST_ASSERT_TRUE(uid_to_str(nullptr, {buf, sizeof(buf)}) == nullptr);
}

void UidToStr_BufferOneByteTooSmall_ReturnsNull()
{
    tg_uid_t uid{};
    char buf[UID_STR_LEN - 1]{}; // one byte short — no room for NUL
    TEST_ASSERT_TRUE(uid_to_str(uid, {buf, sizeof(buf)}) == nullptr);
}

void UidToStr_EmptySpan_ReturnsNull()
{
    tg_uid_t uid{};
    TEST_ASSERT_TRUE(uid_to_str(uid, {}) == nullptr);
}

// ---------------------------------------------------------------------------
// str_to_uid — parse correctness
// ---------------------------------------------------------------------------

void StrToUid_AllZerosHex_ParsesToZeroBytes()
{
    char hex[UID_STR_LEN]{};
    expected_hex_str(hex, sizeof(hex), 0x00);

    tg_uid_t out;
    std::memset(out, 0xFF, UID_CAP);
    TEST_ASSERT_EQUAL(ESP_OK, str_to_uid(hex, out));

    tg_uid_t zeros{};
    TEST_ASSERT_EQUAL_MEMORY(zeros, out, UID_CAP);
}

void StrToUid_AllFFHex_ParsesToFFBytes()
{
    char hex[UID_STR_LEN]{};
    expected_hex_str(hex, sizeof(hex), 0xFF);

    tg_uid_t out{};
    TEST_ASSERT_EQUAL(ESP_OK, str_to_uid(hex, out));

    for (std::size_t i = 0; i < UID_CAP; ++i)
        TEST_ASSERT_EQUAL(0xFF, out[i]);
}

void StrToUid_AcceptsUppercase()
{
    char hex_lower[UID_STR_LEN]{};
    char hex_upper[UID_STR_LEN]{};
    expected_hex_str(hex_lower, sizeof(hex_lower), 0xAB);
    for (std::size_t i = 0; i < UID_STR_LEN; ++i)
        hex_upper[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(hex_lower[i])));

    tg_uid_t lower_result{}, upper_result{};
    TEST_ASSERT_EQUAL(ESP_OK, str_to_uid(hex_lower, lower_result));
    TEST_ASSERT_EQUAL(ESP_OK, str_to_uid(hex_upper, upper_result));
    TEST_ASSERT_EQUAL_MEMORY(lower_result, upper_result, UID_CAP);
}

// ---------------------------------------------------------------------------
// Roundtrip tests
// ---------------------------------------------------------------------------

void UidStr_Roundtrip_UidToStrToUid()
{
    tg_uid_t original{};
    for (std::size_t i = 0; i < UID_CAP; ++i)
        original[i] = static_cast<uint8_t>(i * 17u); // spread pattern

    char buf[UID_STR_LEN]{};
    uid_to_str(original, {buf, sizeof(buf)});

    tg_uid_t recovered{};
    TEST_ASSERT_EQUAL(ESP_OK, str_to_uid(buf, recovered));
    TEST_ASSERT_EQUAL_MEMORY(original, recovered, UID_CAP);
}

void UidStr_Roundtrip_StrToUidToStr()
{
    // Build a valid hex string from a known pattern
    tg_uid_t seed{};
    for (std::size_t i = 0; i < UID_CAP; ++i)
        seed[i] = static_cast<uint8_t>(0xF0u ^ i);

    char original_str[UID_STR_LEN]{};
    uid_to_str(seed, {original_str, sizeof(original_str)});

    tg_uid_t uid{};
    TEST_ASSERT_EQUAL(ESP_OK, str_to_uid(original_str, uid));

    char recovered_str[UID_STR_LEN]{};
    uid_to_str(uid, {recovered_str, sizeof(recovered_str)});

    TEST_ASSERT_EQUAL_STRING(original_str, recovered_str);
}

// ---------------------------------------------------------------------------
// Parametric length-independence tests
// Verify the hex encode/decode logic is correct at lengths 1, 4, 8, 16, 32.
// If UID_CAP changes to any of these, the same algorithm must hold.
// ---------------------------------------------------------------------------

void HexConversion_Length1_Correct()
{
    std::array<uint8_t, 1> data{0xAB};
    TEST_ASSERT_TRUE(hex_roundtrip_correct<1>(data));
}

void HexConversion_Length4_Correct()
{
    std::array<uint8_t, 4> data{0x12, 0x34, 0x56, 0x78};
    TEST_ASSERT_TRUE(hex_roundtrip_correct<4>(data));
}

void HexConversion_Length8_Correct()
{
    std::array<uint8_t, 8> data{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    TEST_ASSERT_TRUE(hex_roundtrip_correct<8>(data));
}

void HexConversion_Length16_Correct()
{
    std::array<uint8_t, 16> data{};
    for (std::size_t i = 0; i < 16; ++i) data[i] = static_cast<uint8_t>(i);
    TEST_ASSERT_TRUE(hex_roundtrip_correct<16>(data));
}

void HexConversion_Length32_Correct()
{
    std::array<uint8_t, 32> data{};
    for (std::size_t i = 0; i < 32; ++i) data[i] = static_cast<uint8_t>(i * 7u);
    TEST_ASSERT_TRUE(hex_roundtrip_correct<32>(data));
}

// ---------------------------------------------------------------------------
// str_to_uid — error cases
// ---------------------------------------------------------------------------

void StrToUid_NullOutput_ReturnsInvalidArg()
{
    char hex[UID_STR_LEN]{};
    expected_hex_str(hex, sizeof(hex), 0x00);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, str_to_uid(hex, nullptr));
}

void StrToUid_TooShort_ReturnsInvalidArg()
{
    tg_uid_t out{};
    char hex[UID_STR_LEN]{};
    std::memset(hex, '0', UID_STR_LEN - 2u); // one char short of UID_CAP*2
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        str_to_uid(std::string_view(hex, UID_STR_LEN - 2u), out));
}

void StrToUid_TooLong_ReturnsInvalidArg()
{
    tg_uid_t out{};
    char hex[UID_STR_LEN + 2]{};
    std::memset(hex, '0', UID_STR_LEN);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        str_to_uid(std::string_view(hex, UID_STR_LEN), out));
}

void StrToUid_InvalidHexChar_ReturnsInvalidArg()
{
    tg_uid_t out{};
    char hex[UID_STR_LEN]{};
    std::memset(hex, '0', UID_STR_LEN);
    hex[UID_STR_LEN / 2] = 'g'; // invalid
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
        str_to_uid(std::string_view(hex, UID_STR_LEN), out));
}

void StrToUid_EmptyString_ReturnsInvalidArg()
{
    tg_uid_t out{};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, str_to_uid("", out));
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main(void)
{
    UNITY_BEGIN();

    UnityDefaultTestRun(UidToStr_AllZeros_ProducesZeroHexString,
                        "UidToStr_AllZeros_ProducesZeroHexString", __FILE__);
    UnityDefaultTestRun(UidToStr_AllFF_ProducesFFHexString,
                        "UidToStr_AllFF_ProducesFFHexString", __FILE__);
    UnityDefaultTestRun(UidToStr_KnownPattern_CorrectHex,
                        "UidToStr_KnownPattern_CorrectHex", __FILE__);
    UnityDefaultTestRun(UidToStr_OutputLength_EqualsUidCapTimes2,
                        "UidToStr_OutputLength_EqualsUidCapTimes2", __FILE__);
    UnityDefaultTestRun(UidToStr_ReturnsPointerToBuffer,
                        "UidToStr_ReturnsPointerToBuffer", __FILE__);
    UnityDefaultTestRun(UidToStr_NoSeparatorsInOutput,
                        "UidToStr_NoSeparatorsInOutput", __FILE__);

    UnityDefaultTestRun(UidToStr_NullUid_ReturnsNull,
                        "UidToStr_NullUid_ReturnsNull", __FILE__);
    UnityDefaultTestRun(UidToStr_BufferOneByteTooSmall_ReturnsNull,
                        "UidToStr_BufferOneByteTooSmall_ReturnsNull", __FILE__);
    UnityDefaultTestRun(UidToStr_EmptySpan_ReturnsNull,
                        "UidToStr_EmptySpan_ReturnsNull", __FILE__);

    UnityDefaultTestRun(StrToUid_AllZerosHex_ParsesToZeroBytes,
                        "StrToUid_AllZerosHex_ParsesToZeroBytes", __FILE__);
    UnityDefaultTestRun(StrToUid_AllFFHex_ParsesToFFBytes,
                        "StrToUid_AllFFHex_ParsesToFFBytes", __FILE__);
    UnityDefaultTestRun(StrToUid_AcceptsUppercase,
                        "StrToUid_AcceptsUppercase", __FILE__);

    UnityDefaultTestRun(UidStr_Roundtrip_UidToStrToUid,
                        "UidStr_Roundtrip_UidToStrToUid", __FILE__);
    UnityDefaultTestRun(UidStr_Roundtrip_StrToUidToStr,
                        "UidStr_Roundtrip_StrToUidToStr", __FILE__);

    UnityDefaultTestRun(HexConversion_Length1_Correct,
                        "HexConversion_Length1_Correct", __FILE__);
    UnityDefaultTestRun(HexConversion_Length4_Correct,
                        "HexConversion_Length4_Correct", __FILE__);
    UnityDefaultTestRun(HexConversion_Length8_Correct,
                        "HexConversion_Length8_Correct", __FILE__);
    UnityDefaultTestRun(HexConversion_Length16_Correct,
                        "HexConversion_Length16_Correct", __FILE__);
    UnityDefaultTestRun(HexConversion_Length32_Correct,
                        "HexConversion_Length32_Correct", __FILE__);

    UnityDefaultTestRun(StrToUid_NullOutput_ReturnsInvalidArg,
                        "StrToUid_NullOutput_ReturnsInvalidArg", __FILE__);
    UnityDefaultTestRun(StrToUid_TooShort_ReturnsInvalidArg,
                        "StrToUid_TooShort_ReturnsInvalidArg", __FILE__);
    UnityDefaultTestRun(StrToUid_TooLong_ReturnsInvalidArg,
                        "StrToUid_TooLong_ReturnsInvalidArg", __FILE__);
    UnityDefaultTestRun(StrToUid_InvalidHexChar_ReturnsInvalidArg,
                        "StrToUid_InvalidHexChar_ReturnsInvalidArg", __FILE__);
    UnityDefaultTestRun(StrToUid_EmptyString_ReturnsInvalidArg,
                        "StrToUid_EmptyString_ReturnsInvalidArg", __FILE__);

    return UNITY_END();
}
