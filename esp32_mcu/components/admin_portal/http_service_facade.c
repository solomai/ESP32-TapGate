#include "http_service_facade.h"

#include <string.h>

#include "logs.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "HTTP Service Facade";

static esp_err_t http_service_facade_commit_wifi_settings(void)
{
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to open NVS partition %s:%s", NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE);
        return err;
    }

    err = nvs_set_blob(handle, STORE_NVM_SETTINGS, &wifi_settings, sizeof(wifi_settings));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    if (err != ESP_OK) {
        LOGE(TAG, "Failed to store Wi-Fi settings (%s)", esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

esp_err_t http_service_facade_load_ap_credentials(http_service_credentials_t *credentials, bool *has_valid_password)
{
    if (credentials == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(credentials, 0, sizeof(*credentials));
    strlcpy(credentials->ssid, (const char *)wifi_settings.ap_ssid, sizeof(credentials->ssid));
    strlcpy(credentials->password, (const char *)wifi_settings.ap_pwd, sizeof(credentials->password));

    if (has_valid_password) {
        *has_valid_password = http_service_facade_is_password_valid(credentials->password);
    }

    return ESP_OK;
}

esp_err_t http_service_facade_store_ap_credentials(const http_service_credentials_t *credentials)
{
    if (credentials == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    strlcpy((char *)wifi_settings.ap_ssid, credentials->ssid, sizeof(wifi_settings.ap_ssid));
    strlcpy((char *)wifi_settings.ap_pwd, credentials->password, sizeof(wifi_settings.ap_pwd));

    return http_service_facade_commit_wifi_settings();
}

bool http_service_facade_is_ssid_valid(const char *ssid)
{
    if (ssid == NULL) {
        return false;
    }

    size_t len = strlen(ssid);
    return len > 0 && len <= MAX_SSID_SIZE;
}

bool http_service_facade_is_password_valid(const char *password)
{
    if (password == NULL) {
        return false;
    }

    size_t len = strlen(password);
    if (len == 0) {
        return false;
    }

    return len >= WPA2_MINIMUM_PASSWORD_LENGTH && len <= (MAX_PASSWORD_SIZE - 1);
}
