#include "page_busy.h"
#include "../page_common.h"

extern const uint8_t _binary_assets_page_busy_html_start[];
extern const uint8_t _binary_assets_page_busy_html_end[];

esp_err_t page_busy_render(httpd_req_t *req)
{
    return admin_page_send_template(req,
                                    _binary_assets_page_busy_html_start,
                                    _binary_assets_page_busy_html_end,
                                    NULL,
                                    0);
}

