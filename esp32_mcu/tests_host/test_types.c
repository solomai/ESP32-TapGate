#include "unity.h"
#include "common/types.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// TestCase:
// Up to 15 characters are allowed due to NVM key size limitations.
void test_types_uid_length(void)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(15, sizeof(uid_t));
}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    RUN_TEST(test_types_uid_length);
    return UNITY_END();
}
