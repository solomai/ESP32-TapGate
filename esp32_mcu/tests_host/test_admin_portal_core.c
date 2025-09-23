#include "unity.h"

#include "admin_portal_core.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>

typedef struct {
    char ssid[ADMIN_PORTAL_SSID_MAX];
    char password[ADMIN_PORTAL_PASSWORD_MAX];
    uint32_t timeout;
} mock_nvs_storage_t;

static mock_nvs_storage_t g_mock_nvs;

static void mock_nvs_reset(void)
{
    memset(&g_mock_nvs, 0, sizeof(g_mock_nvs));
}

esp_err_t nvs_open_from_partition(const char *partition_name,
                                  const char *name_space,
                                  nvs_open_mode_t open_mode,
                                  nvs_handle_t *out_handle)
{
    (void)partition_name;
    (void)name_space;
    (void)open_mode;
    if (!out_handle)
        return ESP_ERR_INVALID_ARG;
    *out_handle = (nvs_handle_t)&g_mock_nvs;
    return ESP_OK;
}

static const char *value_for_key(const char *key)
{
    if (strcmp(key, ADMIN_PORTAL_KEY_AP_SSID) == 0)
        return g_mock_nvs.ssid[0] ? g_mock_nvs.ssid : NULL;
    if (strcmp(key, ADMIN_PORTAL_KEY_AP_PASSWORD) == 0)
        return g_mock_nvs.password[0] ? g_mock_nvs.password : NULL;
    return NULL;
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *out_value, size_t *length)
{
    (void)handle;
    if (!length)
        return ESP_ERR_INVALID_ARG;

    const char *value = value_for_key(key);
    if (!value)
        return ESP_ERR_NVS_NOT_FOUND;

    size_t required = strlen(value) + 1;
    if (!out_value)
    {
        *length = required;
        return ESP_OK;
    }
    if (*length < required)
    {
        *length = required;
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    memcpy(out_value, value, required);
    *length = required;
    return ESP_OK;
}

esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *value)
{
    (void)handle;
    if (!key || !value)
        return ESP_ERR_INVALID_ARG;

    if (strcmp(key, ADMIN_PORTAL_KEY_AP_SSID) == 0)
    {
        strncpy(g_mock_nvs.ssid, value, sizeof(g_mock_nvs.ssid) - 1);
        g_mock_nvs.ssid[sizeof(g_mock_nvs.ssid) - 1] = '\0';
        return ESP_OK;
    }
    if (strcmp(key, ADMIN_PORTAL_KEY_AP_PASSWORD) == 0)
    {
        strncpy(g_mock_nvs.password, value, sizeof(g_mock_nvs.password) - 1);
        g_mock_nvs.password[sizeof(g_mock_nvs.password) - 1] = '\0';
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t nvs_get_u32(nvs_handle_t handle, const char *key, uint32_t *out_value)
{
    (void)handle;
    if (!key || !out_value)
        return ESP_ERR_INVALID_ARG;

    if (strcmp(key, ADMIN_PORTAL_KEY_IDLE_TIMEOUT) == 0)
    {
        if (g_mock_nvs.timeout == 0)
            return ESP_ERR_NVS_NOT_FOUND;
        *out_value = g_mock_nvs.timeout;
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t nvs_set_u32(nvs_handle_t handle, const char *key, uint32_t value)
{
    (void)handle;
    if (!key)
        return ESP_ERR_INVALID_ARG;
    if (strcmp(key, ADMIN_PORTAL_KEY_IDLE_TIMEOUT) == 0)
    {
        g_mock_nvs.timeout = value;
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle)
{
    (void)handle;
}

void setUp(void)
{
    mock_nvs_reset();
}

void tearDown(void)
{
}

static void init_state(admin_portal_state_t *state)
{
    TEST_ASSERT_NOT_NULL(state);
    TEST_ASSERT_EQUAL_INT(ESP_OK, admin_portal_state_init(state));
}

void test_enroll_required_when_password_missing(void)
{
    admin_portal_state_t state;
    init_state(&state);
    TEST_ASSERT_FALSE(state.password_defined);
    TEST_ASSERT_EQUAL_INT(ADMIN_PAGE_ENROLL, admin_portal_select_page(&state, ADMIN_PAGE_MAIN));

    char session_id[ADMIN_PORTAL_SESSION_ID_MAX];
    admin_session_result_t result = admin_portal_resolve_session(&state, NULL, 1000, session_id, sizeof(session_id));
    TEST_ASSERT_EQUAL_INT(ADMIN_SESSION_ACCEPTED_NEW, result);
    TEST_ASSERT_TRUE(strlen(session_id) > 0);
}

void test_enroll_sets_password_and_authorizes(void)
{
    admin_portal_state_t state;
    init_state(&state);
    TEST_ASSERT_EQUAL_INT(ESP_OK, admin_portal_set_password(&state, "StrongPass1"));
    TEST_ASSERT_TRUE(state.password_defined);
    TEST_ASSERT_TRUE(state.authorized);
    TEST_ASSERT_EQUAL_STRING("StrongPass1", admin_portal_get_ap_password(&state));
    TEST_ASSERT_EQUAL_INT(ADMIN_PAGE_MAIN, admin_portal_select_page(&state, ADMIN_PAGE_MAIN));
}

void test_authentication_flow(void)
{
    admin_portal_state_t state;
    init_state(&state);
    TEST_ASSERT_EQUAL_INT(ESP_OK, admin_portal_set_password(&state, "StrongPass2"));

    admin_portal_session_end(&state);

    char session_id[ADMIN_PORTAL_SESSION_ID_MAX];
    TEST_ASSERT_EQUAL_INT(ADMIN_SESSION_ACCEPTED_NEW,
                         admin_portal_resolve_session(&state, NULL, 2000, session_id, sizeof(session_id)));

    TEST_ASSERT_EQUAL_INT(ADMIN_PAGE_AUTH, admin_portal_select_page(&state, ADMIN_PAGE_MAIN));
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, admin_portal_verify_password(&state, "wrong"));
    TEST_ASSERT_EQUAL_INT(ESP_OK, admin_portal_verify_password(&state, "StrongPass2"));
    TEST_ASSERT_TRUE(state.authorized);
    TEST_ASSERT_EQUAL_INT(ADMIN_PAGE_MAIN, admin_portal_select_page(&state, ADMIN_PAGE_MAIN));
}

void test_change_password_updates_value(void)
{
    admin_portal_state_t state;
    init_state(&state);
    TEST_ASSERT_EQUAL_INT(ESP_OK, admin_portal_set_password(&state, "Initial123"));
    TEST_ASSERT_EQUAL_INT(ESP_OK, admin_portal_change_password(&state, "Initial123", "NewSecret9"));
    TEST_ASSERT_EQUAL_STRING("NewSecret9", admin_portal_get_ap_password(&state));
    TEST_ASSERT_EQUAL_INT(ESP_OK, admin_portal_verify_password(&state, "NewSecret9"));
}

void test_session_timeout_results_in_off_page(void)
{
    g_mock_nvs.timeout = 1; // second
    admin_portal_state_t state;
    init_state(&state);
    TEST_ASSERT_EQUAL_INT(ESP_OK, admin_portal_set_password(&state, "Timeout99"));

    char session_id[ADMIN_PORTAL_SESSION_ID_MAX];
    TEST_ASSERT_EQUAL_INT(ADMIN_SESSION_ACCEPTED_NEW,
                         admin_portal_resolve_session(&state, NULL, 1000, session_id, sizeof(session_id)));

    state.last_activity_us = 1000;
    admin_session_result_t result = admin_portal_resolve_session(&state, session_id, 3000000, session_id, sizeof(session_id));
    TEST_ASSERT_EQUAL_INT(ADMIN_SESSION_TIMED_OUT, result);
}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    RUN_TEST(test_enroll_required_when_password_missing);
    RUN_TEST(test_enroll_sets_password_and_authorizes);
    RUN_TEST(test_authentication_flow);
    RUN_TEST(test_change_password_updates_value);
    RUN_TEST(test_session_timeout_results_in_off_page);
    return UNITY_END();
}

