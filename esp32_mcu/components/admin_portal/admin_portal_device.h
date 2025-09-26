#pragma once

#include <stddef.h>

/**
 * @brief Copy the currently configured AP SSID into the provided buffer.
 *
 * The buffer is always null-terminated when size > 0.
 */
void admin_portal_device_get_ap_ssid(char *ssid, size_t size);

/**
 * @brief Update the in-memory AP SSID used by the device.
 */
void admin_portal_device_set_ap_ssid(const char *ssid);

/**
 * @brief Update the in-memory AP password used by the device.
 */
void admin_portal_device_set_ap_password(const char *password);
