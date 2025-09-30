#include "admin_portal_state.h"
#include "logs.h"
#include <string.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

static const char *TAG = "AdminPortal";

// Helper function to safely measure string length
static size_t strnlen_safe(const char *str, size_t max_len)
{
    if (!str) return 0;
    
    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        ++len;
    }
    return len;
}

// Helper function to check if session has timed out
static bool is_session_expired(const admin_portal_state_t *state, uint64_t now_ms)
{
    if (!state || !state->session.active || state->inactivity_timeout_ms == 0) {
        return false;
    }
    
    uint64_t elapsed = now_ms - state->session.last_activity_ms;
    return elapsed >= state->inactivity_timeout_ms;
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
                                                               const char *client_ip,
                                                               const char *session_token,
                                                               const char *device_fingerprint,
                                                               uint64_t now_ms)
{
    if (!state || !state->session.active) {
        return ADMIN_PORTAL_SESSION_NONE;
    }

    // Check if session has timed out
    if (is_session_expired(state, now_ms)) {
        admin_portal_state_clear_session(state);
        return ADMIN_PORTAL_SESSION_EXPIRED;
    }

    // Primary identification: check if this is the same client
    bool ip_matches = client_ip && (strcmp(state->session.client_ip, client_ip) == 0);
    bool token_matches = session_token && (strcmp(state->session.session_token, session_token) == 0);
    bool fingerprint_matches = device_fingerprint && (strcmp(state->session.device_fingerprint, device_fingerprint) == 0);

    // Allow session match if either:
    // 1. IP and token match (same device, same session)
    // 2. IP and fingerprint match (same device, recovered session)
    if ((ip_matches && token_matches) || (ip_matches && fingerprint_matches)) {
        return ADMIN_PORTAL_SESSION_MATCH;
    }

    // If there's an active session but client doesn't match, it's busy
    return ADMIN_PORTAL_SESSION_BUSY;
}

void admin_portal_state_start_session(admin_portal_state_t *state,
                                      const char *client_ip,
                                      const char *session_token,
                                      const char *device_fingerprint,
                                      uint64_t now_ms,
                                      bool authorized)
{
    if (!state || !client_ip || !session_token) {
        return;
    }

    // Clear any existing session first
    admin_portal_state_clear_session(state);

    state->session.active = true;
    state->session.authorized = authorized;
    state->session.last_activity_ms = now_ms;
    
    // Store client IP
    size_t ip_length = strnlen_safe(client_ip, sizeof(state->session.client_ip) - 1);
    memcpy(state->session.client_ip, client_ip, ip_length);
    state->session.client_ip[ip_length] = '\0';
    
    // Store session token
    size_t token_length = strnlen_safe(session_token, sizeof(state->session.session_token) - 1);
    memcpy(state->session.session_token, session_token, token_length);
    state->session.session_token[token_length] = '\0';
    
    // Store device fingerprint (optional)
    if (device_fingerprint) {
        size_t fp_length = strnlen_safe(device_fingerprint, sizeof(state->session.device_fingerprint) - 1);
        memcpy(state->session.device_fingerprint, device_fingerprint, fp_length);
        state->session.device_fingerprint[fp_length] = '\0';
    } else {
        state->session.device_fingerprint[0] = '\0';
    }
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
    bool password_set = admin_portal_state_has_password(state);
    bool authorized = admin_portal_state_session_authorized(state);

    LOGI(TAG, "Resolve page: session status: %s, requested page: %s, AP PSW: %s, authorized: %s, state obj: %s",
        admin_portal_session_status_to_str(session_status),
        admin_portal_page_to_str(requested_page),
        password_set?"set":"not set",
        authorized?"yes":"no",
        state?"yes":"NULL");

    if (!state)
        return ADMIN_PORTAL_PAGE_AUTH;

    if (session_status == ADMIN_PORTAL_SESSION_BUSY)
        return ADMIN_PORTAL_PAGE_BUSY;

    if (session_status == ADMIN_PORTAL_SESSION_EXPIRED)
        return ADMIN_PORTAL_PAGE_AUTH;

    // Enroll admin first. redirect to enroll page.
    if (!password_set)
        return ADMIN_PORTAL_PAGE_ENROLL;

    // Auth required first. redirect to auth page.
    if (session_status != ADMIN_PORTAL_SESSION_MATCH || !authorized)
        return ADMIN_PORTAL_PAGE_AUTH;

    // Enroll page could be called once. redirect to auth page.
    if (requested_page == ADMIN_PORTAL_PAGE_ENROLL)
    {
        if (!password_set)
            return ADMIN_PORTAL_PAGE_ENROLL;
        else
            requested_page = ADMIN_PORTAL_PAGE_AUTH;
    }

    // No reason open Auth page in the case autorized. redirect to main page.
    if (requested_page == ADMIN_PORTAL_PAGE_AUTH)
    {
        if (session_status == ADMIN_PORTAL_SESSION_MATCH && authorized)
            requested_page = ADMIN_PORTAL_PAGE_MAIN;
    }

    // Authenticated users should not access busy page - redirect to main
    if (requested_page == ADMIN_PORTAL_PAGE_BUSY)
    {
        if (session_status == ADMIN_PORTAL_SESSION_MATCH && authorized)
            requested_page = ADMIN_PORTAL_PAGE_MAIN;
    }
        
    return requested_page;
}

const char *admin_portal_state_page_route(admin_portal_page_t page)
{
    if ((size_t)page >= ARRAY_SIZE(page_routes))
        return "/";
    const char *route = page_routes[page];
    return route ? route : "/";
}

