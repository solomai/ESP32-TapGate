
#include "unity.h"

extern "C" void setUp(void)
{}

extern "C" void tearDown(void)
{}

// Example host test case
void base_host_test(void)
{
    TEST_ASSERT_TRUE(true);
}

// Example failing test case to demonstrate test failure reporting
void base_host_test_false(void)
{
    TEST_ASSERT_TRUE(false);
}

int main(void)
{
    UNITY_BEGIN();
    UnityDefaultTestRun(base_host_test, "base_host_test", __FILE__);
    UnityDefaultTestRun(base_host_test_false, "base_host_test_false", __FILE__);
    return UNITY_END();
}
