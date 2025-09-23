#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ADMIN_PORTAL_NAMESPACE             "admin_portal"
#define ADMIN_PORTAL_KEY_AP_SSID           "ap_ssid"
#define ADMIN_PORTAL_KEY_AP_PASSWORD       "ap_psw"
#define ADMIN_PORTAL_KEY_IDLE_TIMEOUT      "ap_timeout"

#define ADMIN_PORTAL_SESSION_ID_MAX        32
#define ADMIN_PORTAL_SSID_MAX              32
#define ADMIN_PORTAL_PASSWORD_MAX          64
#define ADMIN_PORTAL_MESSAGE_MAX           128
#define ADMIN_PORTAL_DEFAULT_TIMEOUT_SEC   (15 * 60)
#define ADMIN_PORTAL_MIN_PASSWORD_LENGTH   8

typedef enum {
    ADMIN_PAGE_MAIN,
    ADMIN_PAGE_DEVICE,
    ADMIN_PAGE_WIFI,
    ADMIN_PAGE_CLIENTS,
    ADMIN_PAGE_EVENTS,
    ADMIN_PAGE_AUTH,
    ADMIN_PAGE_ENROLL,
    ADMIN_PAGE_CHANGE_PASSWORD,
    ADMIN_PAGE_BUSY,
    ADMIN_PAGE_OFF,
} admin_portal_page_id_t;

typedef enum {
    ADMIN_SESSION_ACCEPTED = 0,
    ADMIN_SESSION_ACCEPTED_NEW,
    ADMIN_SESSION_BUSY,
    ADMIN_SESSION_TIMED_OUT,
} admin_session_result_t;

typedef struct {
    bool session_active;
    bool authorized;
    bool password_defined;
    char session_id[ADMIN_PORTAL_SESSION_ID_MAX];
    char ap_ssid[ADMIN_PORTAL_SSID_MAX];
    char ap_password[ADMIN_PORTAL_PASSWORD_MAX];
    uint32_t idle_timeout_sec;
    uint64_t last_activity_us;
} admin_portal_state_t;

esp_err_t admin_portal_state_init(admin_portal_state_t *state);

admin_session_result_t admin_portal_resolve_session(admin_portal_state_t *state,
                                                    const char *session_id,
                                                    uint64_t now_us,
                                                    char *out_session_id,
                                                    size_t out_size);

void admin_portal_session_end(admin_portal_state_t *state);

void admin_portal_touch(admin_portal_state_t *state, uint64_t now_us);

admin_portal_page_id_t admin_portal_select_page(const admin_portal_state_t *state,
                                                admin_portal_page_id_t requested);

bool admin_portal_password_valid(const char *password);

esp_err_t admin_portal_set_password(admin_portal_state_t *state, const char *password);

esp_err_t admin_portal_verify_password(admin_portal_state_t *state, const char *password);

esp_err_t admin_portal_change_password(admin_portal_state_t *state,
                                       const char *old_password,
                                       const char *new_password);

esp_err_t admin_portal_set_ap_ssid(admin_portal_state_t *state, const char *ssid);

static inline const char *admin_portal_get_ap_ssid(const admin_portal_state_t *state)
{
    return state ? state->ap_ssid : "";
}

static inline const char *admin_portal_get_ap_password(const admin_portal_state_t *state)
{
    return state ? state->ap_password : "";
}
