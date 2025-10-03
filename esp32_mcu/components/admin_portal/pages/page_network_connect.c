#include "page_network_connect.h"

extern const uint8_t _binary_network_connect_html_gz_start[];
extern const uint8_t _binary_network_connect_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_network_connect = {
    .route = "/network_connect/",
    .alternate_route = "/network_connect",
    .page = ADMIN_PORTAL_PAGE_WIFI, // Reuse WiFi page enum for auth requirements
    .requires_auth = true,
    .asset_start = _binary_network_connect_html_gz_start,
    .asset_end = _binary_network_connect_html_gz_end,
    .content_type = "text/html",
};