#include "unity.h"

#include <string.h>

#include "admin_portal/http_service_logic.h"

static void set_credentials(http_service_credentials_t *credentials, const char *ssid, const char *password)
{
    memset(credentials, 0, sizeof(*credentials));
    if (ssid) {
        strncpy(credentials->ssid, ssid, sizeof(credentials->ssid) - 1);
    }
    if (password) {
        strncpy(credentials->password, password, sizeof(credentials->password) - 1);
    }
}

void setUp(void)
{
}

void tearDown(void)
{
}

static void expect_validation_error(const http_service_credentials_t *credentials)
{
    char message[64] = {0};
    TEST_ASSERT_TRUE(http_service_logic_validate_credentials(credentials, message, sizeof(message)) != ESP_OK);
    TEST_ASSERT_TRUE(message[0] != '\0');
}

void test_http_service_logic_select_initial_uri_requires_password(void)
{
    http_service_credentials_t credentials;
    set_credentials(&credentials, "TapGate", "");
    const char *uri = http_service_logic_select_initial_uri(&credentials);
    TEST_ASSERT_EQUAL_STRING(HTTP_SERVICE_URI_AP_PAGE, uri);
}

void test_http_service_logic_select_initial_uri_with_valid_password(void)
{
    http_service_credentials_t credentials;
    set_credentials(&credentials, "TapGate", "12345678");
    const char *uri = http_service_logic_select_initial_uri(&credentials);
    TEST_ASSERT_EQUAL_STRING(HTTP_SERVICE_URI_MAIN_PAGE, uri);
}

void test_http_service_logic_validate_credentials_success(void)
{
    http_service_credentials_t credentials;
    set_credentials(&credentials, "TapGate", "12345678");
    char message[64] = {0};
    TEST_ASSERT_EQUAL_INT(ESP_OK, http_service_logic_validate_credentials(&credentials, message, sizeof(message)));
    TEST_ASSERT_TRUE(message[0] == '\0');
}

void test_http_service_logic_validate_credentials_requires_ssid(void)
{
    http_service_credentials_t credentials;
    set_credentials(&credentials, "", "12345678");
    expect_validation_error(&credentials);
}

void test_http_service_logic_validate_credentials_rejects_short_password(void)
{
    http_service_credentials_t credentials;
    set_credentials(&credentials, "TapGate", "short");
    expect_validation_error(&credentials);
}

void test_http_service_logic_prepare_ap_state_sets_cancel_flag(void)
{
    http_service_credentials_t credentials;
    set_credentials(&credentials, "TapGate", "12345678");
    http_service_ap_state_t state;
    http_service_logic_prepare_ap_state(&credentials, &state);
    TEST_ASSERT_TRUE(state.cancel_enabled);
}

void test_http_service_logic_prepare_ap_state_disables_cancel_without_password(void)
{
    http_service_credentials_t credentials;
    set_credentials(&credentials, "TapGate", "");
    http_service_ap_state_t state;
    http_service_logic_prepare_ap_state(&credentials, &state);
    TEST_ASSERT_FALSE(state.cancel_enabled);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_http_service_logic_select_initial_uri_requires_password);
    RUN_TEST(test_http_service_logic_select_initial_uri_with_valid_password);
    RUN_TEST(test_http_service_logic_validate_credentials_success);
    RUN_TEST(test_http_service_logic_validate_credentials_requires_ssid);
    RUN_TEST(test_http_service_logic_validate_credentials_rejects_short_password);
    RUN_TEST(test_http_service_logic_prepare_ap_state_sets_cancel_flag);
    RUN_TEST(test_http_service_logic_prepare_ap_state_disables_cancel_without_password);
    return UNITY_END();
}
