#include "unity.h"
#include "common/types.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_types_client_id_length(void)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(15, sizeof(client_uid_t));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_types_client_id_length);
    return UNITY_END();
}
