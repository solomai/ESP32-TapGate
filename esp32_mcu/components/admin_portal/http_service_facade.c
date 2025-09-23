#include "http_service_facade.h"

#include "logs.h"

#include <string.h>

#ifdef ESP_PLATFORM
#include "nvs.h"
#include "nvs_flash.h"
#include "wifi_manager.h"

static const char *TAG = "HTTP FACADE";

static void copy_to_buffer(char *destination, size_t capacity, const char *source)
{
    if (!destination || capacity == 0) {
        return;
    }

    size_t length = source ? strnlen(source, capacity - 1) : 0;
    if (length > 0 && source) {
        memcpy(destination, source, length);
    }
    destination[length] = '\0';
}

esp_err_t http_service_facade_get_ap_credentials(http_service_credentials_t *credentials)
{
    if (!credentials) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(credentials, 0, sizeof(*credentials));
    copy_to_buffer(credentials->ssid, sizeof(credentials->ssid), (const char *)wifi_settings.ap_ssid);
    copy_to_buffer(credentials->password, sizeof(credentials->password), (const char *)wifi_settings.ap_pwd);
    return ESP_OK;
}

esp_err_t http_service_facade_save_ap_credentials(const http_service_credentials_t *credentials)
{
    if (!credentials) {
        return ESP_ERR_INVALID_ARG;
    }

    copy_to_buffer((char *)wifi_settings.ap_ssid, sizeof(wifi_settings.ap_ssid), credentials->ssid);
    copy_to_buffer((char *)wifi_settings.ap_pwd, sizeof(wifi_settings.ap_pwd), credentials->password);

    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition(NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to open NVM %s:%s: %s", NVM_WIFI_PARTITION, NVM_WIFI_NAMESPACE, esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(handle, STORE_NVM_SETTINGS, &wifi_settings, sizeof(wifi_settings));
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to store settings: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to commit settings: %s", esp_err_to_name(err));
    }
    nvs_close(handle);
    if (err == ESP_OK) {
        LOGI(TAG, "AP credentials stored for SSID \"%s\"", wifi_settings.ap_ssid);
    }
    return err;
}

#else

esp_err_t http_service_facade_get_ap_credentials(http_service_credentials_t *credentials)
{
    if (!credentials) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(credentials, 0, sizeof(*credentials));
    return ESP_OK;
}

esp_err_t http_service_facade_save_ap_credentials(const http_service_credentials_t *credentials)
{
    (void)credentials;
    return ESP_ERR_INVALID_STATE;
}

#endif
