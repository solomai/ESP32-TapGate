#include "admin_portal_core.h"

#include "esp_log.h"
#include "logs.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "nvm/nvm_partition.h"
#ifndef ADMIN_PORTAL_DEFAULT_AP_SSID
#define ADMIN_PORTAL_DEFAULT_AP_SSID "TapGate AP"
#endif

#ifndef NVM_WIFI_PARTITION
#define NVM_WIFI_PARTITION NVM_PARTITION_DEFAULT
#endif

#include <string.h>

static const char *TAG = "admin_core";

static size_t clamp_copy(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0)
        return 0;
    if (!src)
    {
        dst[0] = '\0';
        return 0;
    }
    size_t length = 0;
    while (length + 1 < dst_size && src[length] != '\0')
    {
        dst[length] = src[length];
        ++length;
    }
    dst[length] = '\0';
    return length;
}

static esp_err_t load_string(nvs_handle_t handle, const char *key, char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0)
        return ESP_ERR_INVALID_ARG;

    size_t required = buffer_size;
    esp_err_t err = nvs_get_str(handle, key, buffer, &required);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        buffer[0] = '\0';
        return ESP_OK;
    }
    if (err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        required = buffer_size;
        err = nvs_get_str(handle, key, buffer, &required);
    }
    return err;
}

esp_err_t admin_portal_state_init(admin_portal_state_t *state)
{
    if (!state)
        return ESP_ERR_INVALID_ARG;

    memset(state, 0, sizeof(*state));
    state->idle_timeout_sec = ADMIN_PORTAL_DEFAULT_TIMEOUT_SEC;

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION,
                                            ADMIN_PORTAL_NAMESPACE,
                                            NVS_READWRITE,
                                            &handle);
    if (err != ESP_OK)
    {
        LOGE(TAG, "Failed to open NVS partition %s namespace %s", NVM_WIFI_PARTITION, ADMIN_PORTAL_NAMESPACE);
        return err;
    }

    err = load_string(handle, ADMIN_PORTAL_KEY_AP_SSID, state->ap_ssid, sizeof(state->ap_ssid));
    if (err != ESP_OK)
    {
        LOGE(TAG, "Failed to load AP SSID: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    if (state->ap_ssid[0] == '\0')
    {
        clamp_copy(state->ap_ssid, sizeof(state->ap_ssid), ADMIN_PORTAL_DEFAULT_AP_SSID);
    }

    err = load_string(handle, ADMIN_PORTAL_KEY_AP_PASSWORD, state->ap_password, sizeof(state->ap_password));
    if (err != ESP_OK)
    {
        LOGE(TAG, "Failed to load AP password: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    state->password_defined = state->ap_password[0] != '\0';

    uint32_t timeout = 0;
    err = nvs_get_u32(handle, ADMIN_PORTAL_KEY_IDLE_TIMEOUT, &timeout);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        timeout = ADMIN_PORTAL_DEFAULT_TIMEOUT_SEC;
        nvs_set_u32(handle, ADMIN_PORTAL_KEY_IDLE_TIMEOUT, timeout);
        nvs_commit(handle);
        err = ESP_OK;
    }
    if (err == ESP_OK && timeout > 0)
    {
        state->idle_timeout_sec = timeout;
    }

    nvs_close(handle);
    return ESP_OK;
}

static bool session_expired(const admin_portal_state_t *state, uint64_t now_us)
{
    if (!state->session_active)
        return false;
    uint64_t timeout_us = (uint64_t)state->idle_timeout_sec * 1000000ULL;
    if (timeout_us == 0)
        return false;
    return now_us - state->last_activity_us > timeout_us;
}

admin_session_result_t admin_portal_resolve_session(admin_portal_state_t *state,
                                                    const char *session_id,
                                                    uint64_t now_us,
                                                    char *out_session_id,
                                                    size_t out_size)
{
    if (!state)
        return ADMIN_SESSION_BUSY;

    if (session_expired(state, now_us))
    {
        LOGI(TAG, "Session timed out");
        admin_portal_session_end(state);
        return ADMIN_SESSION_TIMED_OUT;
    }

    if (state->session_active)
    {
        if (session_id && state->session_id[0] != '\0' && strcmp(session_id, state->session_id) == 0)
        {
            if (out_session_id && out_size)
                clamp_copy(out_session_id, out_size, state->session_id);
            return ADMIN_SESSION_ACCEPTED;
        }
        return ADMIN_SESSION_BUSY;
    }

    state->session_active = true;
    state->authorized = false;
    state->session_id[0] = '\0';

    if (out_session_id && out_size)
    {
        snprintf(state->session_id, sizeof(state->session_id), "%016llX", (unsigned long long)now_us);
        clamp_copy(out_session_id, out_size, state->session_id);
    }
    else
    {
        snprintf(state->session_id, sizeof(state->session_id), "%016llX", (unsigned long long)now_us);
    }

    state->last_activity_us = now_us;
    return ADMIN_SESSION_ACCEPTED_NEW;
}

void admin_portal_session_end(admin_portal_state_t *state)
{
    if (!state)
        return;
    state->session_active = false;
    state->authorized = false;
    state->session_id[0] = '\0';
    state->last_activity_us = 0;
}

void admin_portal_touch(admin_portal_state_t *state, uint64_t now_us)
{
    if (!state)
        return;
    state->last_activity_us = now_us;
}

static bool page_requires_auth(admin_portal_page_id_t page)
{
    switch (page)
    {
        case ADMIN_PAGE_MAIN:
        case ADMIN_PAGE_DEVICE:
        case ADMIN_PAGE_WIFI:
        case ADMIN_PAGE_CLIENTS:
        case ADMIN_PAGE_EVENTS:
        case ADMIN_PAGE_CHANGE_PASSWORD:
            return true;
        default:
            return false;
    }
}

admin_portal_page_id_t admin_portal_select_page(const admin_portal_state_t *state,
                                                admin_portal_page_id_t requested)
{
    if (!state)
        return ADMIN_PAGE_BUSY;

    if (!state->password_defined)
    {
        return ADMIN_PAGE_ENROLL;
    }

    if (!state->authorized)
    {
        if (requested == ADMIN_PAGE_AUTH)
            return ADMIN_PAGE_AUTH;
        return ADMIN_PAGE_AUTH;
    }

    if (requested == ADMIN_PAGE_AUTH || requested == ADMIN_PAGE_ENROLL)
        return ADMIN_PAGE_MAIN;

    if (!page_requires_auth(requested))
        return requested;

    return requested;
}

bool admin_portal_password_valid(const char *password)
{
    if (!password)
        return false;
    size_t len = strlen(password);
    return len >= ADMIN_PORTAL_MIN_PASSWORD_LENGTH && len < ADMIN_PORTAL_PASSWORD_MAX;
}

static esp_err_t save_string(const char *value, const char *key)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION,
                                            ADMIN_PORTAL_NAMESPACE,
                                            NVS_READWRITE,
                                            &handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_str(handle, key, value ? value : "");
    if (err == ESP_OK)
        err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t admin_portal_set_password(admin_portal_state_t *state, const char *password)
{
    if (!state || !password)
        return ESP_ERR_INVALID_ARG;

    if (!admin_portal_password_valid(password))
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = save_string(password, ADMIN_PORTAL_KEY_AP_PASSWORD);
    if (err != ESP_OK)
        return err;

    clamp_copy(state->ap_password, sizeof(state->ap_password), password);
    state->password_defined = true;
    state->authorized = true;
    return ESP_OK;
}

esp_err_t admin_portal_verify_password(admin_portal_state_t *state, const char *password)
{
    if (!state || !password)
        return ESP_ERR_INVALID_ARG;
    if (!state->password_defined)
        return ESP_ERR_INVALID_STATE;

    if (strncmp(password, state->ap_password, sizeof(state->ap_password)) == 0)
    {
        state->authorized = true;
        return ESP_OK;
    }
    state->authorized = false;
    return ESP_FAIL;
}

esp_err_t admin_portal_change_password(admin_portal_state_t *state,
                                       const char *old_password,
                                       const char *new_password)
{
    if (!state || !old_password || !new_password)
        return ESP_ERR_INVALID_ARG;

    if (strncmp(old_password, state->ap_password, sizeof(state->ap_password)) != 0)
        return ESP_ERR_INVALID_STATE;

    if (!admin_portal_password_valid(new_password))
        return ESP_ERR_INVALID_ARG;

    return admin_portal_set_password(state, new_password);
}

esp_err_t admin_portal_set_ap_ssid(admin_portal_state_t *state, const char *ssid)
{
    if (!state || !ssid)
        return ESP_ERR_INVALID_ARG;

    if (ssid[0] == '\0')
        return ESP_ERR_INVALID_ARG;

    char buffer[ADMIN_PORTAL_SSID_MAX];
    clamp_copy(buffer, sizeof(buffer), ssid);

    esp_err_t err = save_string(buffer, ADMIN_PORTAL_KEY_AP_SSID);
    if (err != ESP_OK)
        return err;

    clamp_copy(state->ap_ssid, sizeof(state->ap_ssid), buffer);
    return ESP_OK;
}

