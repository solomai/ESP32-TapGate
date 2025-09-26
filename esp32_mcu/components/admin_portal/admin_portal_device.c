#include "admin_portal_device.h"

#include "wifi_manager.h"

#include <string.h>

static size_t strnlen_safe(const char *str, size_t max_len)
{
    size_t len = 0;
    if (!str)
        return 0;
    while (len < max_len && str[len] != '\0')
        ++len;
    return len;
}

void admin_portal_device_get_ap_ssid(char *ssid, size_t size)
{
    if (!ssid || size == 0)
        return;

    const char *source = (const char *)wifi_settings.ap_ssid;
    size_t length = strnlen_safe(source, sizeof(wifi_settings.ap_ssid));
    if (length >= size)
        length = size - 1;

    memcpy(ssid, source, length);
    ssid[length] = '\0';
}

void admin_portal_device_set_ap_ssid(const char *ssid)
{
    if (!ssid)
        return;

    size_t length = strnlen_safe(ssid, sizeof(wifi_settings.ap_ssid));
    memcpy(wifi_settings.ap_ssid, ssid, length);
    if (length < sizeof(wifi_settings.ap_ssid))
        wifi_settings.ap_ssid[length] = '\0';
}

void admin_portal_device_set_ap_password(const char *password)
{
    if (!password)
        return;

    size_t length = strnlen_safe(password, sizeof(wifi_settings.ap_pwd));
    memcpy(wifi_settings.ap_pwd, password, length);
    if (length < sizeof(wifi_settings.ap_pwd))
        wifi_settings.ap_pwd[length] = '\0';
}
