#include "unity.h"
#include "clients/clients.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_clients_namespace_length(void)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(15, strlen(CLIENTS_DB_NAMESPACE));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clients_namespace_length);
    return UNITY_END();
}
