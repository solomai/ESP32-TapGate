#include "page_wifi.h"

extern const uint8_t _binary_page_wifi_html_gz_start[];
extern const uint8_t _binary_page_wifi_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_wifi = {
    .route = "/wifi/",
    .alternate_route = "/wifi",
    .page = ADMIN_PORTAL_PAGE_WIFI,
    .requires_auth = true,
    .asset_start = _binary_page_wifi_html_gz_start,
    .asset_end = _binary_page_wifi_html_gz_end,
    .content_type = "text/html",
};

