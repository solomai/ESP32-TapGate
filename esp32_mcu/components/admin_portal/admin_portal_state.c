#include "admin_portal_state.h"

#include <string.h>

static size_t strnlen_safe(const char *str, size_t max_len)
{
    if (!str)
        return 0;
    size_t len = 0;
    while (len < max_len && str[len])
        ++len;
    return len;
}

static bool page_requires_authorization(admin_portal_page_t page)
{
    switch (page)
    {
    case ADMIN_PORTAL_PAGE_MAIN:
    case ADMIN_PORTAL_PAGE_DEVICE:
    case ADMIN_PORTAL_PAGE_WIFI:
    case ADMIN_PORTAL_PAGE_CLIENTS:
    case ADMIN_PORTAL_PAGE_EVENTS:
    case ADMIN_PORTAL_PAGE_CHANGE_PASSWORD:
        return true;
    default:
        return false;
    }
}

void admin_portal_state_init(admin_portal_state_t *state,
                             uint32_t timeout_ms,
                             uint8_t min_pwd_len)
{
    if (!state)
        return;

    memset(state, 0, sizeof(*state));
    state->min_password_length = min_pwd_len;
    state->inactivity_timeout_ms = timeout_ms;
}

void admin_portal_state_update_timeout(admin_portal_state_t *state, uint32_t timeout_ms)
{
    if (!state)
        return;
    state->inactivity_timeout_ms = timeout_ms;
}

void admin_portal_state_set_ssid(admin_portal_state_t *state, const char *ssid)
{
    if (!state)
        return;

    if (!ssid)
    {
        state->ap_ssid[0] = '\0';
        return;
    }

    size_t length = strnlen_safe(ssid, MAX_SSID_SIZE - 1);
    memcpy(state->ap_ssid, ssid, length);
    state->ap_ssid[length] = '\0';
}

const char *admin_portal_state_get_ssid(const admin_portal_state_t *state)
{
    if (!state)
        return "";
    return state->ap_ssid;
}

void admin_portal_state_set_password(admin_portal_state_t *state, const char *password)
{
    if (!state)
        return;

    if (!password)
    {
        state->ap_password[0] = '\0';
        state->password_length = 0;
        return;
    }

    size_t length = strnlen_safe(password, MAX_PASSWORD_SIZE - 1);
    memcpy(state->ap_password, password, length);
    state->ap_password[length] = '\0';
    state->password_length = length;
}

bool admin_portal_state_verify_password(const admin_portal_state_t *state, const char *password)
{
    if (!state)
        return false;
    if (!password)
        return false;
    if (!state->password_length)
        return false;
    size_t length = strnlen_safe(password, MAX_PASSWORD_SIZE - 1);
    if (length != state->password_length)
        return false;
    return memcmp(state->ap_password, password, length) == 0;
}

bool admin_portal_state_has_password(const admin_portal_state_t *state)
{
    if (!state)
        return false;
    return state->password_length >= state->min_password_length;
}

bool admin_portal_state_password_valid(const admin_portal_state_t *state, const char *password)
{
    if (!state || !password)
        return false;
    size_t length = strnlen_safe(password, MAX_PASSWORD_SIZE);
    if (length >= MAX_PASSWORD_SIZE)
        return false;
    return length >= state->min_password_length;
}

static void clear_session(admin_portal_session_t *session)
{
    if (!session)
        return;
    memset(session, 0, sizeof(*session));
}

void admin_portal_state_clear_session(admin_portal_state_t *state)
{
    if (!state)
        return;
    clear_session(&state->session);
}

void admin_portal_state_start_session(admin_portal_state_t *state,
                                      const char *token,
                                      uint64_t now_ms,
                                      bool authorized)
{
    if (!state)
        return;

    clear_session(&state->session);
    if (!token || !token[0])
        return;

    size_t length = strnlen_safe(token, ADMIN_PORTAL_SESSION_TOKEN_LENGTH);
    memcpy(state->session.token, token, length);
    state->session.token[length] = '\0';
    state->session.active = true;
    state->session.authorized = authorized;
    state->session.last_activity_ms = now_ms;
}

void admin_portal_state_authorize_session(admin_portal_state_t *state)
{
    if (!state)
        return;
    if (!state->session.active)
        return;
    state->session.authorized = true;
}

bool admin_portal_state_session_active(const admin_portal_state_t *state)
{
    if (!state)
        return false;
    return state->session.active;
}

bool admin_portal_state_session_authorized(const admin_portal_state_t *state)
{
    if (!state)
        return false;
    return state->session.active && state->session.authorized;
}

static bool session_has_expired(const admin_portal_state_t *state, uint64_t now_ms)
{
    if (!state || !state->session.active)
        return false;
    uint32_t timeout = state->inactivity_timeout_ms;
    if (timeout == 0)
        return false;
    if (now_ms < state->session.last_activity_ms)
        return false;
    return (now_ms - state->session.last_activity_ms) >= timeout;
}

admin_portal_session_status_t admin_portal_state_check_session(admin_portal_state_t *state,
                                                               const char *token,
                                                               uint64_t now_ms)
{
    if (!state)
        return ADMIN_PORTAL_SESSION_NONE;

    if (session_has_expired(state, now_ms))
    {
        clear_session(&state->session);
        return ADMIN_PORTAL_SESSION_EXPIRED;
    }

    if (!state->session.active)
        return ADMIN_PORTAL_SESSION_NONE;

    if (!token || !token[0])
        return ADMIN_PORTAL_SESSION_NONE;

    size_t length = strnlen_safe(token, ADMIN_PORTAL_SESSION_TOKEN_LENGTH);
    size_t stored_length = strnlen_safe(state->session.token, ADMIN_PORTAL_SESSION_TOKEN_LENGTH);
    if (length == stored_length && strncmp(state->session.token, token, length) == 0)
    {
        state->session.last_activity_ms = now_ms;
        return ADMIN_PORTAL_SESSION_MATCH;
    }

    if (admin_portal_state_has_password(state))
        return ADMIN_PORTAL_SESSION_BUSY;

    return ADMIN_PORTAL_SESSION_NONE;
}

static bool should_redirect_to_enroll(const admin_portal_state_t *state,
                                      admin_portal_page_t requested_page)
{
    if (!state)
        return false;
    if (admin_portal_state_has_password(state))
        return false;
    return requested_page != ADMIN_PORTAL_PAGE_ENROLL &&
           requested_page != ADMIN_PORTAL_PAGE_BUSY &&
           requested_page != ADMIN_PORTAL_PAGE_OFF;
}

admin_portal_page_t admin_portal_state_resolve_page(const admin_portal_state_t *state,
                                                    admin_portal_page_t requested_page,
                                                    admin_portal_session_status_t session_status)
{
    if (!state)
        return requested_page;

    switch (session_status)
    {
    case ADMIN_PORTAL_SESSION_BUSY:
        return ADMIN_PORTAL_PAGE_BUSY;
    case ADMIN_PORTAL_SESSION_EXPIRED:
        return ADMIN_PORTAL_PAGE_OFF;
    default:
        break;
    }

    if (requested_page == ADMIN_PORTAL_PAGE_BUSY || requested_page == ADMIN_PORTAL_PAGE_OFF)
        return requested_page;

    if (should_redirect_to_enroll(state, requested_page))
        return ADMIN_PORTAL_PAGE_ENROLL;

    if (!admin_portal_state_has_password(state))
        return ADMIN_PORTAL_PAGE_ENROLL;

    if (requested_page == ADMIN_PORTAL_PAGE_ENROLL)
        return ADMIN_PORTAL_PAGE_AUTH;

    bool authorized = admin_portal_state_session_authorized(state);
    if (!authorized && page_requires_authorization(requested_page))
        return ADMIN_PORTAL_PAGE_AUTH;

    if (authorized && requested_page == ADMIN_PORTAL_PAGE_AUTH)
        return ADMIN_PORTAL_PAGE_MAIN;

    return requested_page;
}
