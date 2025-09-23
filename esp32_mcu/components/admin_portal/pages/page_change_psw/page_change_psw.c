#include "page_change_psw.h"
#include "page_common.h"

#include <stdio.h>
#include <string.h>

extern const uint8_t _binary_assets_page_change_psw_html_start[];
extern const uint8_t _binary_assets_page_change_psw_html_end[];

esp_err_t page_change_psw_render(httpd_req_t *req,
                                 const admin_portal_state_t *state,
                                 const char *message,
                                 admin_focus_t focus)
{
    if (!state)
        return ESP_ERR_INVALID_ARG;

    char ssid_buffer[ADMIN_PORTAL_SSID_MAX + 16];
    snprintf(ssid_buffer, sizeof(ssid_buffer), "%s Admin", admin_portal_get_ap_ssid(state));

    char min_length[8];
    snprintf(min_length, sizeof(min_length), "%d", ADMIN_PORTAL_MIN_PASSWORD_LENGTH);

    char old_extra[32] = "";
    if (focus == ADMIN_FOCUS_PASSWORD_OLD)
        strncpy(old_extra, " class=\"error\" autofocus", sizeof(old_extra) - 1);

    char new_extra[32] = "";
    if (focus == ADMIN_FOCUS_PASSWORD_NEW)
        strncpy(new_extra, " class=\"error\" autofocus", sizeof(new_extra) - 1);

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
        {"PASSWORD_MIN_LENGTH", min_length},
        {"OLD_PASSWORD_EXTRA", old_extra},
        {"NEW_PASSWORD_EXTRA", new_extra},
        {"MESSAGE_BLOCK", message_block},
    };

    return admin_page_send_template(req,
                                    _binary_assets_page_change_psw_html_start,
                                    _binary_assets_page_change_psw_html_end,
                                    pairs,
                                    sizeof(pairs) / sizeof(pairs[0]));
}

