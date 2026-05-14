#include "unity.h"
#include "esp_system.h"

TEST_CASE("reset reason is valid", "[diagnostics]")
{
    esp_reset_reason_t reason = esp_reset_reason();
    TEST_ASSERT(reason >= ESP_RST_UNKNOWN && reason <= ESP_RST_JTAG);
}

TEST_CASE("reset reason is not unknown on normal boot", "[diagnostics]")
{
    esp_reset_reason_t reason = esp_reset_reason();
    TEST_ASSERT_NOT_EQUAL(ESP_RST_UNKNOWN, reason);
}