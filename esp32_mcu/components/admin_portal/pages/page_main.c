#include "page_main.h"
#include "page_common.h"

extern const uint8_t _binary_assets_page_main_html_start[];
extern const uint8_t _binary_assets_page_main_html_end[];

esp_err_t page_main_render(httpd_req_t *req, const admin_portal_state_t *state)
{
    (void)state;
    return admin_page_send_template(req,
                                    _binary_assets_page_main_html_start,
                                    _binary_assets_page_main_html_end,
                                    NULL,
                                    0);
}

