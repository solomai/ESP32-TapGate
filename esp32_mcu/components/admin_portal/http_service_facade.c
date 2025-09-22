#include "http_service_facade.h"

#include <ctype.h>
#include <string.h>

#include "esp_err.h"
#include "esp_wifi.h"
#include "nvs.h"

#include "logs.h"
#include "wifi_manager.h"

static const char *TAG = "HTTP Service Facade";

static esp_err_t save_wifi_settings_to_nvm(void)
{
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to open NVM %s:%s (%s)", NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(handle, STORE_NVM_SETTINGS, &wifi_settings, sizeof(wifi_settings));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    if (err != ESP_OK) {
        LOGE(TAG, "Failed to store AP settings (%s)", esp_err_to_name(err));
    } else {
        LOGI(TAG, "Stored AP settings: ssid=%s", wifi_settings.ap_ssid);
    }

    nvs_close(handle);
    return err;
}

static bool string_is_blank(const char *text)
{
    if (!text) {
        return true;
    }

    while (*text) {
        if (!isspace((unsigned char)*text)) {
            return false;
        }
        ++text;
    }
    return true;
}

esp_err_t http_service_facade_get_ap_credentials(char *ssid, size_t ssid_len,
                                                 char *password, size_t password_len)
{
    if (!ssid || !password || ssid_len == 0 || password_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    strlcpy(ssid, (const char *)wifi_settings.ap_ssid, ssid_len);
    strlcpy(password, (const char *)wifi_settings.ap_pwd, password_len);
    return ESP_OK;
}

bool http_service_facade_has_valid_ap_password(void)
{
    size_t length = strlen((const char *)wifi_settings.ap_pwd);
    return length >= WPA2_MINIMUM_PASSWORD_LENGTH;
}

esp_err_t http_service_facade_store_ap_credentials(const char *ssid, const char *password)
{
    if (!ssid || !password) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t ssid_len = strlen(ssid);
    size_t password_len = strlen(password);

    if (string_is_blank(ssid) || ssid_len == 0 || ssid_len >= sizeof(wifi_settings.ap_ssid)) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (password_len < WPA2_MINIMUM_PASSWORD_LENGTH || password_len >= sizeof(wifi_settings.ap_pwd)) {
        return ESP_ERR_INVALID_SIZE;
    }

    wifi_config_t ap_config = { 0 };
    ap_config.ap.channel = wifi_settings.ap_channel;
    ap_config.ap.ssid_hidden = wifi_settings.ap_ssid_hidden;
    ap_config.ap.max_connection = DEFAULT_AP_MAX_CONNECTIONS;
    ap_config.ap.beacon_interval = DEFAULT_AP_BEACON_INTERVAL;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.ssid_len = 0;
    strlcpy((char *)ap_config.ap.ssid, ssid, sizeof(ap_config.ap.ssid));
    strlcpy((char *)ap_config.ap.password, password, sizeof(ap_config.ap.password));

    esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    if (err == ESP_ERR_WIFI_NOT_INIT || err == ESP_ERR_WIFI_NOT_STARTED) {
        LOGW(TAG, "WiFi not ready, storing credentials only (%s)", esp_err_to_name(err));
        err = ESP_OK;
    } else if (err != ESP_OK) {
        LOGE(TAG, "Failed to apply AP config (%s)", esp_err_to_name(err));
        return err;
    }

    esp_err_t bw_err = esp_wifi_set_bandwidth(WIFI_IF_AP, wifi_settings.ap_bandwidth);
    if (bw_err == ESP_ERR_WIFI_NOT_INIT || bw_err == ESP_ERR_WIFI_NOT_STARTED) {
        LOGW(TAG, "WiFi not ready to apply bandwidth (%s)", esp_err_to_name(bw_err));
    } else if (bw_err != ESP_OK) {
        LOGE(TAG, "Failed to apply AP bandwidth (%s)", esp_err_to_name(bw_err));
        return bw_err;
    }

    memset(wifi_settings.ap_ssid, 0, sizeof(wifi_settings.ap_ssid));
    memset(wifi_settings.ap_pwd, 0, sizeof(wifi_settings.ap_pwd));
    strlcpy((char *)wifi_settings.ap_ssid, ssid, sizeof(wifi_settings.ap_ssid));
    strlcpy((char *)wifi_settings.ap_pwd, password, sizeof(wifi_settings.ap_pwd));

    err = save_wifi_settings_to_nvm();
    if (err != ESP_OK) {
        return err;
    }

    LOGI(TAG, "Updated AP credentials");
    return ESP_OK;
}

