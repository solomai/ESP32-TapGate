#include "page_auth.h"
#include "../page_common.h"

#include <stdio.h>
#include <string.h>

extern const uint8_t _binary_assets_page_auth_html_start[];
extern const uint8_t _binary_assets_page_auth_html_end[];

esp_err_t page_auth_render(httpd_req_t *req,
                           const admin_portal_state_t *state,
                           const char *message,
                           admin_focus_t focus)
{
    if (!state)
        return ESP_ERR_INVALID_ARG;

    char ssid_buffer[ADMIN_PORTAL_SSID_MAX + 16];
    snprintf(ssid_buffer, sizeof(ssid_buffer), "%s Admin", admin_portal_get_ap_ssid(state));

    char focus_attr[16] = "";
    if (focus == ADMIN_FOCUS_PASSWORD)
        strncpy(focus_attr, " autofocus", sizeof(focus_attr) - 1);

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

    admin_template_pair_t pairs[] = {
        {"ACCESS_POINT_LABEL", ssid_buffer},
        {"PASSWORD_FOCUS", focus_attr},
        {"MESSAGE_BLOCK", message_block},
    };

    return admin_page_send_template(req,
                                    _binary_assets_page_auth_html_start,
                                    _binary_assets_page_auth_html_end,
                                    pairs,
                                    sizeof(pairs) / sizeof(pairs[0]));
}

