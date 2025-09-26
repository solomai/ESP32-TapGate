#include "unity.h"

#include "admin_portal_device.h"
#include "admin_portal_state.h"

static admin_portal_state_t state;

void setUp(void)
{
    admin_portal_state_init(&state, ADMIN_PORTAL_DEFAULT_IDLE_TIMEOUT_MIN * 60U * 1000U,
                            WPA2_MINIMUM_PASSWORD_LENGTH);
}

void tearDown(void)
{
}

void test_session_starts_and_matches_token(void)
{
    admin_portal_state_start_session(&state, "abcdef", 1000, false);
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&state, "abcdef", 1000);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH, status);
}

void test_session_busy_for_different_token_when_password_exists(void)
{
    admin_portal_state_set_password(&state, "strongpass");
    admin_portal_state_start_session(&state, "token", 1000, true);
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&state, "other", 1100);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_BUSY, status);
}

void test_session_not_busy_without_password(void)
{
    admin_portal_state_start_session(&state, "token", 0, false);
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&state, "other", 50);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_NONE, status);
}

void test_session_expires_after_timeout(void)
{
    admin_portal_state_set_password(&state, "strongpass");
    admin_portal_state_start_session(&state, "token", 0, true);
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&state, "token", state.inactivity_timeout_ms + 1);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_EXPIRED, status);
}

void test_password_validation_rules(void)
{
    TEST_ASSERT_FALSE(admin_portal_state_password_valid(&state, "short"));
    TEST_ASSERT_TRUE(admin_portal_state_password_valid(&state, "longenough"));
}

int main(int argc, char **argv)
{
    UnityBegin(argv[0]);
    RUN_TEST(test_session_starts_and_matches_token);
    RUN_TEST(test_session_busy_for_different_token_when_password_exists);
    RUN_TEST(test_session_not_busy_without_password);
    RUN_TEST(test_session_expires_after_timeout);
    RUN_TEST(test_password_validation_rules);
    return UnityEnd();
}
