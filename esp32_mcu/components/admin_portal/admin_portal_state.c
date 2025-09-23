#include "admin_portal_state.h"

#include <string.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

static size_t strnlen_safe(const char *str, size_t max_len)
{
    size_t len = 0;
    if (!str)
        return 0;
    while (len < max_len && str[len] != '\0')
        ++len;
    return len;
}

static bool page_requires_auth(admin_portal_page_t page)
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
        case ADMIN_PORTAL_PAGE_ENROLL:
        case ADMIN_PORTAL_PAGE_AUTH:
        case ADMIN_PORTAL_PAGE_BUSY:
        case ADMIN_PORTAL_PAGE_OFF:
        default:
            return false;
    }
}

static const char *const page_routes[] = {
    [ADMIN_PORTAL_PAGE_ENROLL] = "/enroll/",
    [ADMIN_PORTAL_PAGE_AUTH] = "/auth/",
    [ADMIN_PORTAL_PAGE_CHANGE_PASSWORD] = "/psw/",
    [ADMIN_PORTAL_PAGE_DEVICE] = "/device/",
    [ADMIN_PORTAL_PAGE_WIFI] = "/wifi/",
    [ADMIN_PORTAL_PAGE_CLIENTS] = "/clients/",
    [ADMIN_PORTAL_PAGE_EVENTS] = "/events/",
    [ADMIN_PORTAL_PAGE_MAIN] = "/main/",
    [ADMIN_PORTAL_PAGE_BUSY] = "/busy/",
    [ADMIN_PORTAL_PAGE_OFF] = "/off/",
};

void admin_portal_state_init(admin_portal_state_t *state,
                             uint32_t inactivity_timeout_ms,
                             uint8_t min_password_length)
{
    if (!state)
        return;

    memset(state, 0, sizeof(*state));
    state->inactivity_timeout_ms = inactivity_timeout_ms;
    state->min_password_length = min_password_length;
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

    size_t length = strnlen_safe(ssid, sizeof(state->ap_ssid) - 1);
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

    size_t length = strnlen_safe(password, sizeof(state->ap_password) - 1);
    memcpy(state->ap_password, password, length);
    state->ap_password[length] = '\0';
    state->password_length = length;
}

bool admin_portal_state_has_password(const admin_portal_state_t *state)
{
    if (!state)
        return false;
    return state->password_length >= state->min_password_length;
}

bool admin_portal_state_password_matches(const admin_portal_state_t *state, const char *password)
{
    if (!state || !password)
        return false;
    if (!admin_portal_state_has_password(state))
        return false;
    return strncmp(state->ap_password, password, state->password_length) == 0 &&
           password[state->password_length] == '\0';
}

bool admin_portal_state_password_valid(const admin_portal_state_t *state, const char *password)
{
    if (!state || !password)
        return false;

    size_t length = strnlen_safe(password, sizeof(state->ap_password));
    if (length < state->min_password_length)
        return false;

    if (length >= sizeof(state->ap_password))
        return false;

    return true;
}

admin_portal_session_status_t admin_portal_state_check_session(admin_portal_state_t *state,
                                                               const char *token,
                                                               uint64_t now_ms)
{
    if (!state || !state->session.active)
        return ADMIN_PORTAL_SESSION_NONE;

    if (!state->session.claimed)
    {
        if (!token || token[0] == '\0')
            return ADMIN_PORTAL_SESSION_NONE;

        if (strncmp(token, state->session.token, ADMIN_PORTAL_TOKEN_MAX_LEN) == 0)
        {
            state->session.claimed = true;
        }
        else
        {
            return ADMIN_PORTAL_SESSION_NONE;
        }
    }

    uint64_t last_activity = state->session.last_activity_ms;
    uint64_t timeout = state->inactivity_timeout_ms;
    if (timeout > 0 && now_ms >= last_activity)
    {
        uint64_t delta = now_ms - last_activity;
        if (delta >= timeout)
            return ADMIN_PORTAL_SESSION_EXPIRED;
    }

    if (!token || strncmp(token, state->session.token, ADMIN_PORTAL_TOKEN_MAX_LEN) != 0)
        return ADMIN_PORTAL_SESSION_BUSY;

    return ADMIN_PORTAL_SESSION_MATCH;
}

void admin_portal_state_start_session(admin_portal_state_t *state,
                                      const char *token,
                                      uint64_t now_ms,
                                      bool authorized)
{
    if (!state || !token)
        return;

    state->session.active = true;
    state->session.authorized = authorized;
    state->session.claimed = false;
    state->session.last_activity_ms = now_ms;
    size_t length = strnlen_safe(token, ADMIN_PORTAL_TOKEN_MAX_LEN);
    memcpy(state->session.token, token, length);
    state->session.token[length] = '\0';
}

void admin_portal_state_update_session(admin_portal_state_t *state, uint64_t now_ms)
{
    if (!state || !state->session.active)
        return;

    state->session.last_activity_ms = now_ms;
}

void admin_portal_state_clear_session(admin_portal_state_t *state)
{
    if (!state)
        return;

    memset(&state->session, 0, sizeof(state->session));
}

void admin_portal_state_authorize_session(admin_portal_state_t *state)
{
    if (!state)
        return;

    if (state->session.active)
    {
        state->session.authorized = true;
        state->session.claimed = true;
    }
}

bool admin_portal_state_session_authorized(const admin_portal_state_t *state)
{
    if (!state)
        return false;
    return state->session.active && state->session.authorized;
}

admin_portal_page_t admin_portal_state_resolve_page(const admin_portal_state_t *state,
                                                    admin_portal_page_t requested_page,
                                                    admin_portal_session_status_t session_status)
{
    if (!state)
        return ADMIN_PORTAL_PAGE_OFF;

    if (session_status == ADMIN_PORTAL_SESSION_BUSY)
        return ADMIN_PORTAL_PAGE_BUSY;

    if (session_status == ADMIN_PORTAL_SESSION_EXPIRED)
        return ADMIN_PORTAL_PAGE_OFF;

    bool password_set = admin_portal_state_has_password(state);
    bool authorized = admin_portal_state_session_authorized(state);

    switch (requested_page)
    {
        case ADMIN_PORTAL_PAGE_ENROLL:
            return password_set ? ADMIN_PORTAL_PAGE_AUTH : ADMIN_PORTAL_PAGE_ENROLL;
        case ADMIN_PORTAL_PAGE_AUTH:
            if (!password_set)
                return ADMIN_PORTAL_PAGE_ENROLL;
            if (session_status == ADMIN_PORTAL_SESSION_MATCH && authorized)
                return ADMIN_PORTAL_PAGE_MAIN;
            return ADMIN_PORTAL_PAGE_AUTH;
        case ADMIN_PORTAL_PAGE_BUSY:
            return ADMIN_PORTAL_PAGE_BUSY;
        case ADMIN_PORTAL_PAGE_OFF:
            return ADMIN_PORTAL_PAGE_OFF;
        default:
            break;
    }

    if (!password_set)
        return ADMIN_PORTAL_PAGE_ENROLL;

    if (!page_requires_auth(requested_page))
        return requested_page;

    if (session_status != ADMIN_PORTAL_SESSION_MATCH)
        return ADMIN_PORTAL_PAGE_AUTH;

    if (!authorized)
        return ADMIN_PORTAL_PAGE_AUTH;

    return requested_page;
}

bool admin_portal_state_page_requires_auth(admin_portal_page_t page)
{
    return page_requires_auth(page);
}

const char *admin_portal_state_page_route(admin_portal_page_t page)
{
    if ((size_t)page >= ARRAY_SIZE(page_routes))
        return "/";
    const char *route = page_routes[page];
    return route ? route : "/";
}

