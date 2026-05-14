#include "unity.h"
#include "esp_log.h"

void app_main(void) 
{
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
}
