#include "unity.h"
#include "clients/clients.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    //RUN_TEST(test_name);
    return UNITY_END();
}
