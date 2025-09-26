#pragma once

#include "admin_portal_state.h"

#include "esp_err.h"

#define ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_SSID "AP_SSID"
#define ADMIN_PORTAL_NVM_NAMESPACE_KEY_AP_PSW "AP_PSW"
#define ADMIN_PORTAL_NVM_NAMESPACE_KEY_IDLE_TIMEOUT "AP_IDLE_TIMEOUT"

#define ADMIN_PORTAL_DEFAULT_IDLE_TIMEOUT_MIN 15U

esp_err_t admin_portal_device_load(admin_portal_state_t *state);
esp_err_t admin_portal_device_save_password(admin_portal_state_t *state, const char *password);
esp_err_t admin_portal_device_save_ssid(admin_portal_state_t *state, const char *ssid);
esp_err_t admin_portal_device_save_timeout(admin_portal_state_t *state, uint32_t minutes);
