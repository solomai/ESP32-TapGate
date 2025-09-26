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

static void set_password(const char *password)
{
    admin_portal_state_set_password(&state, password);
}

static void start_authorized_session(const char *token)
{
    admin_portal_state_start_session(&state, token, 0, false);
    admin_portal_state_authorize_session(&state);
}

void test_enroll_required_when_password_missing(void)
{
    admin_portal_page_t page =
        admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, ADMIN_PORTAL_SESSION_NONE);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_ENROLL, page);
}

void test_auth_page_redirects_when_no_password(void)
{
    admin_portal_page_t page =
        admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_AUTH, ADMIN_PORTAL_SESSION_NONE);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_ENROLL, page);
}

void test_auth_required_when_password_set(void)
{
    set_password("strongpass");
    admin_portal_page_t page =
        admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, ADMIN_PORTAL_SESSION_NONE);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_AUTH, page);
}

void test_main_allowed_after_authorization(void)
{
    set_password("strongpass");
    start_authorized_session("token");
    admin_portal_page_t page =
        admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, ADMIN_PORTAL_SESSION_MATCH);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_MAIN, page);
}

void test_auth_redirects_back_to_main_when_authorized(void)
{
    set_password("strongpass");
    start_authorized_session("token");
    admin_portal_page_t page =
        admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_AUTH, ADMIN_PORTAL_SESSION_MATCH);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_MAIN, page);
}

void test_busy_page_returned_when_session_busy(void)
{
    set_password("strongpass");
    admin_portal_session_status_t status = ADMIN_PORTAL_SESSION_BUSY;
    admin_portal_page_t page =
        admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, status);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_BUSY, page);
}

void test_off_page_returned_when_expired(void)
{
    admin_portal_session_status_t status = ADMIN_PORTAL_SESSION_EXPIRED;
    admin_portal_page_t page =
        admin_portal_state_resolve_page(&state, ADMIN_PORTAL_PAGE_MAIN, status);
    TEST_ASSERT_EQUAL_INT(ADMIN_PORTAL_PAGE_OFF, page);
}

int main(int argc, char **argv)
{
    UnityBegin(argv[0]);
    RUN_TEST(test_enroll_required_when_password_missing);
    RUN_TEST(test_auth_page_redirects_when_no_password);
    RUN_TEST(test_auth_required_when_password_set);
    RUN_TEST(test_main_allowed_after_authorization);
    RUN_TEST(test_auth_redirects_back_to_main_when_authorized);
    RUN_TEST(test_busy_page_returned_when_session_busy);
    RUN_TEST(test_off_page_returned_when_expired);
    return UnityEnd();
}
