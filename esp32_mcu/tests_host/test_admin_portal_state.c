#include "unity.h"
#include "admin_portal_state.h"

static admin_portal_state_t state;

void setUp(void)
{
    admin_portal_state_init(&state, 900000, 8);
}

void tearDown(void)
{
}

static void set_password(const char *password)
{
    admin_portal_state_set_password(&state, password);
}

static void start_session(const char *token, uint64_t now_ms, bool authorized)
{
    admin_portal_state_start_session(&state, token, now_ms, authorized);
}

void test_enroll_redirect_when_no_password(void)
{
    admin_portal_page_t page = admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, ADMIN_PORTAL_SESSION_NONE);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_ENROLL, page);
}

void test_auth_required_when_password_set_but_not_authorized(void)
{
    set_password("superpass");
    start_session("token", 0, false);
    admin_portal_page_t page = admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, ADMIN_PORTAL_SESSION_MATCH);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_AUTH, page);
}

void test_main_access_when_authorized(void)
{
    set_password("superpass");
    start_session("token", 0, false);
    admin_portal_state_authorize_session(&state);
    admin_portal_page_t page = admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, ADMIN_PORTAL_SESSION_MATCH);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_MAIN, page);
}

void test_enroll_redirects_to_auth_when_password_exists(void)
{
    set_password("superpass");
    admin_portal_page_t page = admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_ENROLL, ADMIN_PORTAL_SESSION_NONE);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_AUTH, page);
}

void test_busy_state_blocks_other_clients(void)
{
    set_password("superpass");
    start_session("token", 1000, true);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH,
                          admin_portal_state_check_session(&state, "token", 1000));
    admin_portal_session_status_t status = admin_portal_state_check_session(&state, "other", 1200);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_BUSY, status);
    admin_portal_page_t page = admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, status);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_BUSY, page);
}

void test_enroll_session_takeover_when_no_password(void)
{
    start_session("token", 1000, false);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH,
                          admin_portal_state_check_session(&state, "token", 1000));
    admin_portal_session_status_t status = admin_portal_state_check_session(&state, "other", 1200);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_NONE, status);
}

void test_timeout_moves_to_off_page(void)
{
    set_password("superpass");
    start_session("token", 0, true);
    uint64_t now = state.inactivity_timeout_ms;
    admin_portal_session_status_t status = admin_portal_state_check_session(&state, "token", now);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_EXPIRED, status);
    admin_portal_page_t page = admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, status);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_OFF, page);
}

void test_password_validation_rules(void)
{
    TEST_ASSERT_FALSE(admin_portal_state_password_valid(&state, "short"));
    TEST_ASSERT_TRUE(admin_portal_state_password_valid(&state, "longenough"));
}

void test_pending_session_allows_new_client_to_claim(void)
{
    start_session("token", 0, false);
    admin_portal_session_status_t status = admin_portal_state_check_session(&state, NULL, 0);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_NONE, status);
    status = admin_portal_state_check_session(&state, "different", 0);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_NONE, status);
}

void test_authorized_session_without_cookie_remains_reclaimable(void)
{
    set_password("superpass");
    start_session("token", 0, false);
    admin_portal_state_authorize_session(&state);

    admin_portal_session_status_t status = admin_portal_state_check_session(&state, NULL, 1000);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_NONE, status);

    status = admin_portal_state_check_session(&state, "token", 2000);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH, status);
}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    RUN_TEST(test_enroll_redirect_when_no_password);
    RUN_TEST(test_auth_required_when_password_set_but_not_authorized);
    RUN_TEST(test_main_access_when_authorized);
    RUN_TEST(test_enroll_redirects_to_auth_when_password_exists);
    RUN_TEST(test_busy_state_blocks_other_clients);
    RUN_TEST(test_enroll_session_takeover_when_no_password);
    RUN_TEST(test_timeout_moves_to_off_page);
    RUN_TEST(test_password_validation_rules);
    RUN_TEST(test_pending_session_allows_new_client_to_claim);
    RUN_TEST(test_authorized_session_without_cookie_remains_reclaimable);
    return UNITY_END();
}
