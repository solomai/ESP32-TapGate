#include "unity.h"

#include "admin_portal_device.h"
#include "admin_portal_state.h"
#include "nvm/nvm.h"
#include "nvs.h"

static admin_portal_state_t state;

void setUp(void)
{
    nvs_mock_reset();
    admin_portal_state_init(&state, ADMIN_PORTAL_DEFAULT_IDLE_TIMEOUT_MIN * 60U * 1000U,
                            WPA2_MINIMUM_PASSWORD_LENGTH);
}

void tearDown(void)
{
}

void test_load_uses_defaults_when_storage_empty(void)
{
    esp_err_t err = admin_portal_device_load(&state);
    TEST_ASSERT_EQUAL_INT(ESP_OK, err);
    TEST_ASSERT_EQUAL_STRING(DEFAULT_AP_SSID, admin_portal_state_get_ssid(&state));
    TEST_ASSERT_FALSE(admin_portal_state_has_password(&state));
    TEST_ASSERT_EQUAL_UINT32(ADMIN_PORTAL_DEFAULT_IDLE_TIMEOUT_MIN * 60U * 1000U,
                             state.inactivity_timeout_ms);
}

void test_load_uses_persisted_values(void)
{
    TEST_ASSERT_EQUAL_INT(ESP_OK,
                          nvm_wifi_write_string(ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_SSID, "My Portal"));
    TEST_ASSERT_EQUAL_INT(ESP_OK,
                          nvm_wifi_write_string(ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_PSW, "superpass"));
    TEST_ASSERT_EQUAL_INT(ESP_OK,
                          nvm_wifi_write_u32(ADMIN_PORTAL_NVM_NAMESPACE_KEY_IDLE_TIMEOUT, 5));

    esp_err_t err = admin_portal_device_load(&state);
    TEST_ASSERT_EQUAL_INT(ESP_OK, err);
    TEST_ASSERT_EQUAL_STRING("My Portal", admin_portal_state_get_ssid(&state));
    TEST_ASSERT_TRUE(admin_portal_state_has_password(&state));
    TEST_ASSERT_EQUAL_UINT32(5U * 60U * 1000U, state.inactivity_timeout_ms);
}

void test_save_password_updates_state_and_storage(void)
{
    const char *password = "newpassword";
    esp_err_t err = admin_portal_device_save_password(&state, password);
    TEST_ASSERT_EQUAL_INT(ESP_OK, err);
    TEST_ASSERT_TRUE(admin_portal_state_verify_password(&state, password));

    char buffer[MAX_PASSWORD_SIZE] = {0};
    TEST_ASSERT_EQUAL_INT(ESP_OK,
                          nvm_wifi_read_string(ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_PSW, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING(password, buffer);
}

void test_save_ssid_updates_state_and_storage(void)
{
    const char *ssid = "Updated Portal";
    esp_err_t err = admin_portal_device_save_ssid(&state, ssid);
    TEST_ASSERT_EQUAL_INT(ESP_OK, err);
    TEST_ASSERT_EQUAL_STRING(ssid, admin_portal_state_get_ssid(&state));

    char buffer[MAX_SSID_SIZE] = {0};
    TEST_ASSERT_EQUAL_INT(ESP_OK,
                          nvm_wifi_read_string(ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_SSID, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING(ssid, buffer);
}

void test_save_timeout_persists_minutes_and_updates_state(void)
{
    esp_err_t err = admin_portal_device_save_timeout(&state, 20);
    TEST_ASSERT_EQUAL_INT(ESP_OK, err);
    TEST_ASSERT_EQUAL_UINT32(20U * 60U * 1000U, state.inactivity_timeout_ms);

    uint32_t stored = 0;
    TEST_ASSERT_EQUAL_INT(ESP_OK,
                          nvm_wifi_read_u32(ADMIN_PORTAL_NVM_NAMESPACE_KEY_IDLE_TIMEOUT, &stored));
    TEST_ASSERT_EQUAL_UINT32(20U, stored);
}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    RUN_TEST(test_load_uses_defaults_when_storage_empty);
    RUN_TEST(test_load_uses_persisted_values);
    RUN_TEST(test_save_password_updates_state_and_storage);
    RUN_TEST(test_save_ssid_updates_state_and_storage);
    RUN_TEST(test_save_timeout_persists_minutes_and_updates_state);
    return UNITY_END();
}
