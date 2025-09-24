#include "page_auth.h"

extern const uint8_t _binary_page_auth_html_gz_start[];
extern const uint8_t _binary_page_auth_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_auth = {
    .route = "/auth/",
    .alternate_route = "/auth",
    .page = ADMIN_PORTAL_PAGE_AUTH,
    .requires_auth = false,
    .asset_start = _binary_page_auth_html_gz_start,
    .asset_end = _binary_page_auth_html_gz_end,
    .content_type = "text/html",
};

