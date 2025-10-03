#include "page_add_network.h"

extern const uint8_t _binary_add_network_html_gz_start[];
extern const uint8_t _binary_add_network_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_add_network = {
    .route = "/add_network/",
    .alternate_route = "/add_network",
    .page = ADMIN_PORTAL_PAGE_WIFI, // Reuse WiFi page enum for auth requirements
    .requires_auth = true,
    .asset_start = _binary_add_network_html_gz_start,
    .asset_end = _binary_add_network_html_gz_end,
    .content_type = "text/html",
};