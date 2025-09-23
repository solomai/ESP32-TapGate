#include "page_device.h"
#include "../page_common.h"

#include <stdio.h>
#include <string.h>

extern const uint8_t _binary_assets_page_device_html_start[];
extern const uint8_t _binary_assets_page_device_html_end[];

esp_err_t page_device_render(httpd_req_t *req,
                             const admin_portal_state_t *state,
                             const char *message,
                             admin_focus_t focus)
{
    if (!state)
        return ESP_ERR_INVALID_ARG;

    char message_block[ADMIN_PORTAL_MESSAGE_MAX + 64];
    if (message && message[0] != '\0')
    {
        snprintf(message_block, sizeof(message_block),
                 "<div class=\"message\">%s</div>", message);
    }
    else
    {
        message_block[0] = '\0';
    }

    char ssid_class[16] = "";
    char ssid_focus[16] = "";
    if (focus == ADMIN_FOCUS_SSID)
    {
        strncpy(ssid_class, " error", sizeof(ssid_class) - 1);
        strncpy(ssid_focus, " autofocus", sizeof(ssid_focus) - 1);
    }

    admin_template_pair_t pairs[] = {
        {"SSID_VALUE", admin_portal_get_ap_ssid(state)},
        {"SSID_CLASS", ssid_class},
        {"SSID_FOCUS", ssid_focus},
        {"MESSAGE_BLOCK", message_block},
    };

    return admin_page_send_template(req,
                                    _binary_assets_page_device_html_start,
                                    _binary_assets_page_device_html_end,
                                    pairs,
                                    sizeof(pairs) / sizeof(pairs[0]));
}

