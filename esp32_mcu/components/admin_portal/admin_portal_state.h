#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ADMIN_PORTAL_TOKEN_MAX_LEN 32

typedef enum {
    ADMIN_PORTAL_PAGE_ENROLL,
    ADMIN_PORTAL_PAGE_AUTH,
    ADMIN_PORTAL_PAGE_CHANGE_PASSWORD,
    ADMIN_PORTAL_PAGE_DEVICE,
    ADMIN_PORTAL_PAGE_WIFI,
    ADMIN_PORTAL_PAGE_CLIENTS,
    ADMIN_PORTAL_PAGE_EVENTS,
    ADMIN_PORTAL_PAGE_MAIN,
    ADMIN_PORTAL_PAGE_BUSY,
    ADMIN_PORTAL_PAGE_OFF,
    ADMIN_PORTAL_PAGE_COUNT
} admin_portal_page_t;

typedef enum {
    ADMIN_PORTAL_SESSION_NONE,
    ADMIN_PORTAL_SESSION_MATCH,
    ADMIN_PORTAL_SESSION_EXPIRED,
    ADMIN_PORTAL_SESSION_BUSY,
} admin_portal_session_status_t;

typedef struct {
    bool active;
    bool authorized;
    uint64_t last_activity_ms;
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1];
} admin_portal_session_t;

typedef struct {
    admin_portal_session_t session;
    char ap_ssid[64];
    char ap_password[64];
    size_t password_length;
    uint32_t inactivity_timeout_ms;
    uint8_t min_password_length;
} admin_portal_state_t;

void admin_portal_state_init(admin_portal_state_t *state,
                             uint32_t inactivity_timeout_ms,
                             uint8_t min_password_length);

void admin_portal_state_set_ssid(admin_portal_state_t *state, const char *ssid);
const char *admin_portal_state_get_ssid(const admin_portal_state_t *state);

void admin_portal_state_set_password(admin_portal_state_t *state, const char *password);
bool admin_portal_state_has_password(const admin_portal_state_t *state);
bool admin_portal_state_password_matches(const admin_portal_state_t *state, const char *password);
bool admin_portal_state_password_valid(const admin_portal_state_t *state, const char *password);

admin_portal_session_status_t admin_portal_state_check_session(const admin_portal_state_t *state,
                                                               const char *token,
                                                               uint64_t now_ms);

void admin_portal_state_start_session(admin_portal_state_t *state,
                                      const char *token,
                                      uint64_t now_ms,
                                      bool authorized);

void admin_portal_state_update_session(admin_portal_state_t *state, uint64_t now_ms);

void admin_portal_state_clear_session(admin_portal_state_t *state);

void admin_portal_state_authorize_session(admin_portal_state_t *state);
bool admin_portal_state_session_authorized(const admin_portal_state_t *state);

admin_portal_page_t admin_portal_state_resolve_page(const admin_portal_state_t *state,
                                                    admin_portal_page_t requested_page,
                                                    admin_portal_session_status_t session_status);

bool admin_portal_state_page_requires_auth(admin_portal_page_t page);
const char *admin_portal_state_page_route(admin_portal_page_t page);

