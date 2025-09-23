#include "page_change_psw.h"

extern const uint8_t _binary_page_change_psw_html_gz_start[];
extern const uint8_t _binary_page_change_psw_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_change_psw = {
    .route = "/psw/",
    .alternate_route = "/psw",
    .page = ADMIN_PORTAL_PAGE_CHANGE_PASSWORD,
    .requires_auth = true,
    .asset_start = _binary_page_change_psw_html_gz_start,
    .asset_end = _binary_page_change_psw_html_gz_end,
    .content_type = "text/html",
};

