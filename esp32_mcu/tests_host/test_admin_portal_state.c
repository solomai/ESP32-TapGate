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

void test_protected_pages_redirect_to_enroll_without_password(void)
{
    const admin_portal_page_t pages[] = {
        ADMIN_PORTAL_PAGE_MAIN,
        ADMIN_PORTAL_PAGE_DEVICE,
        ADMIN_PORTAL_PAGE_WIFI,
        ADMIN_PORTAL_PAGE_CLIENTS,
        ADMIN_PORTAL_PAGE_EVENTS,
        ADMIN_PORTAL_PAGE_CHANGE_PASSWORD,
        ADMIN_PORTAL_PAGE_AUTH,
    };

    for (size_t i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i)
    {
        admin_portal_page_t resolved = admin_portal_state_resolve_page(&state, pages[i], ADMIN_PORTAL_SESSION_NONE);
        TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_ENROLL, resolved);
    }
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
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_AUTH, page);
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

void test_authorized_session_prevents_new_session_creation(void)
{
    set_password("superpass");
    start_session("token1", 1000, true);
    
    // First client should have authorized session
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH,
                          admin_portal_state_check_session(&state, "token1", 1000));
    TEST_ASSERT_TRUE(admin_portal_state_session_authorized(&state));
    
    // Attempting to start a new unauthorized session should be blocked when password is set
    admin_portal_state_start_session(&state, "token2", 1100, false);
    
    // Session should still belong to the first client and remain authorized
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH,
                          admin_portal_state_check_session(&state, "token1", 1200));
    TEST_ASSERT_TRUE(admin_portal_state_session_authorized(&state));
    
    // Second client should get BUSY status
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_BUSY,
                          admin_portal_state_check_session(&state, "token2", 1300));
}

void test_enrollment_allows_session_takeover_when_no_password(void)
{
    // During enrollment (no password), session takeover should be allowed
    start_session("token1", 1000, false);  // Start with unauthorized session
    
    // First client should have session
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH,
                          admin_portal_state_check_session(&state, "token1", 1000));
    TEST_ASSERT_FALSE(admin_portal_state_session_authorized(&state));
    
    // Without password set, new session should be allowed to take over
    admin_portal_state_start_session(&state, "token2", 1100, false);
    
    // Session should now belong to the second client
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH,
                          admin_portal_state_check_session(&state, "token2", 1200));
    TEST_ASSERT_FALSE(admin_portal_state_session_authorized(&state));
    
    // First client should no longer have valid session (token doesn't match)
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_NONE,
                          admin_portal_state_check_session(&state, "token1", 1300));
}

void test_clear_session_by_ip(void)
{
    set_password("superpass");
    admin_portal_state_start_session_by_ip(&state, "10.10.0.2", 1000, true);
    
    // Session should be active
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH,
                          admin_portal_state_check_session_by_ip(&state, "10.10.0.2", 1100));
    TEST_ASSERT_TRUE(admin_portal_state_session_authorized(&state));
    
    // Clear session for the specific IP
    admin_portal_state_clear_session_by_ip(&state, "10.10.0.2");
    
    // Session should be cleared
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_NONE,
                          admin_portal_state_check_session_by_ip(&state, "10.10.0.2", 1200));
    TEST_ASSERT_FALSE(admin_portal_state_session_authorized(&state));
}

void test_clear_session_by_ip_wrong_ip(void)
{
    set_password("superpass");
    admin_portal_state_start_session_by_ip(&state, "10.10.0.2", 1000, true);
    
    // Session should be active
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH,
                          admin_portal_state_check_session_by_ip(&state, "10.10.0.2", 1100));
    TEST_ASSERT_TRUE(admin_portal_state_session_authorized(&state));
    
    // Clear session for a different IP - should not affect the session
    admin_portal_state_clear_session_by_ip(&state, "10.10.0.3");
    
    // Session should still be active
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_SESSION_MATCH,
                          admin_portal_state_check_session_by_ip(&state, "10.10.0.2", 1200));
    TEST_ASSERT_TRUE(admin_portal_state_session_authorized(&state));
}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    RUN_TEST(test_enroll_redirect_when_no_password);
    RUN_TEST(test_protected_pages_redirect_to_enroll_without_password);
    RUN_TEST(test_auth_required_when_password_set_but_not_authorized);
    RUN_TEST(test_main_access_when_authorized);
    RUN_TEST(test_enroll_redirects_to_auth_when_password_exists);
    RUN_TEST(test_busy_state_blocks_other_clients);
    RUN_TEST(test_enroll_session_takeover_when_no_password);
    RUN_TEST(test_timeout_moves_to_off_page);
    RUN_TEST(test_password_validation_rules);
    RUN_TEST(test_pending_session_allows_new_client_to_claim);
    RUN_TEST(test_authorized_session_without_cookie_remains_reclaimable);
    RUN_TEST(test_authorized_session_prevents_new_session_creation);
    RUN_TEST(test_enrollment_allows_session_takeover_when_no_password);
    RUN_TEST(test_clear_session_by_ip);
    RUN_TEST(test_clear_session_by_ip_wrong_ip);
    return UNITY_END();
}
