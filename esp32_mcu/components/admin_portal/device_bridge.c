#include "device_bridge.h"

#include "logs.h"
#include "wifi_manager.h"

#include <string.h>

static const char *TAG = "AdminPortalDevice";

static size_t strnlen_safe(const char *str, size_t max_len)
{
    size_t len = 0;
    if (!str)
        return 0;
    while (len < max_len && str[len] != '\0')
        ++len;
    return len;
}

static void apply_string(uint8_t *destination, size_t destination_size, const char *source)
{
    if (!destination || destination_size == 0)
        return;

    memset(destination, 0, destination_size);

    if (!source)
        return;

    size_t length = strnlen_safe(source, destination_size - 1);
    memcpy(destination, source, length);
    destination[length] = '\0';
}

void admin_portal_device_set_ap_password(const char *password)
{
    apply_string(wifi_settings.ap_pwd, sizeof(wifi_settings.ap_pwd), password);
    LOGI(TAG, "AP password updated (length=%u)", (unsigned)strnlen_safe((const char *)wifi_settings.ap_pwd,
                                                                        sizeof(wifi_settings.ap_pwd)));
}

void admin_portal_device_set_ap_ssid(const char *ssid)
{
    apply_string(wifi_settings.ap_ssid, sizeof(wifi_settings.ap_ssid), ssid);
    LOGI(TAG, "AP SSID updated to '%s'", (const char *)wifi_settings.ap_ssid);
}
