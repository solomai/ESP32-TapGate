#include "page_off.h"

extern const uint8_t _binary_page_off_html_gz_start[];
extern const uint8_t _binary_page_off_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_off = {
    .route = "/off/",
    .alternate_route = "/off",
    .page = ADMIN_PORTAL_PAGE_OFF,
    .requires_auth = false,
    .asset_start = _binary_page_off_html_gz_start,
    .asset_end = _binary_page_off_html_gz_end,
    .content_type = "text/html",
};

