#include "admin_portal_device.h"

#include "logs.h"
#include "nvm/nvm.h"

#include <stdint.h>
#include <string.h>

static const char *TAG = "admin_portal_device";

static uint32_t minutes_to_ms(uint32_t minutes)
{
    if (minutes == 0)
        return 0;
    uint64_t value = (uint64_t)minutes * 60ULL * 1000ULL;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}

esp_err_t admin_portal_device_load(admin_portal_state_t *state)
{
    if (!state)
        return ESP_ERR_INVALID_ARG;

    char ssid[MAX_SSID_SIZE] = {0};
    esp_err_t err = nvm_wifi_read_string(ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_SSID, ssid, sizeof(ssid));
    if (err != ESP_OK || ssid[0] == '\0')
    {
        if (err != ESP_OK)
        {
            LOGW(TAG, "Falling back to default SSID due to read error: %s", esp_err_to_name(err));
        }
        strncpy(ssid, DEFAULT_AP_SSID, sizeof(ssid) - 1);
        ssid[sizeof(ssid) - 1] = '\0';
        err = ESP_OK;
    }
    admin_portal_state_set_ssid(state, ssid);

    char password[MAX_PASSWORD_SIZE] = {0};
    esp_err_t pass_err = nvm_wifi_read_string(ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_PSW, password, sizeof(password));
    if (pass_err != ESP_OK)
    {
        LOGW(TAG, "AP password not found in NVM: %s", esp_err_to_name(pass_err));
        password[0] = '\0';
        pass_err = ESP_OK;
    }
    admin_portal_state_set_password(state, password);

    uint32_t timeout_minutes = ADMIN_PORTAL_DEFAULT_IDLE_TIMEOUT_MIN;
    esp_err_t timeout_err = nvm_wifi_read_u32(ADMIN_PORTAL_NVM_NAMESPACE_KEY_IDLE_TIMEOUT, &timeout_minutes);
    if (timeout_err != ESP_OK || timeout_minutes == 0)
    {
        if (timeout_err != ESP_OK)
        {
            LOGW(TAG, "Using default inactivity timeout due to read error: %s", esp_err_to_name(timeout_err));
        }
        timeout_minutes = ADMIN_PORTAL_DEFAULT_IDLE_TIMEOUT_MIN;
    }
    admin_portal_state_update_timeout(state, minutes_to_ms(timeout_minutes));

    return (err != ESP_OK) ? err : (pass_err != ESP_OK ? pass_err : timeout_err);
}

esp_err_t admin_portal_device_save_password(admin_portal_state_t *state, const char *password)
{
    if (!state || !password)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = nvm_wifi_write_string(ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_PSW, password);
    if (err != ESP_OK)
    {
        LOGE(TAG, "Failed to store admin password: %s", esp_err_to_name(err));
        return err;
    }

    admin_portal_state_set_password(state, password);
    return ESP_OK;
}

esp_err_t admin_portal_device_save_ssid(admin_portal_state_t *state, const char *ssid)
{
    if (!state || !ssid)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = nvm_wifi_write_string(ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_SSID, ssid);
    if (err != ESP_OK)
    {
        LOGE(TAG, "Failed to store admin portal SSID: %s", esp_err_to_name(err));
        return err;
    }

    admin_portal_state_set_ssid(state, ssid);
    return ESP_OK;
}

esp_err_t admin_portal_device_save_timeout(admin_portal_state_t *state, uint32_t minutes)
{
    if (!state)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = nvm_wifi_write_u32(ADMIN_PORTAL_NVM_NAMESPACE_KEY_IDLE_TIMEOUT, minutes);
    if (err != ESP_OK)
    {
        LOGE(TAG, "Failed to persist inactivity timeout: %s", esp_err_to_name(err));
        return err;
    }

    admin_portal_state_update_timeout(state, minutes_to_ms(minutes));
    return ESP_OK;
}
