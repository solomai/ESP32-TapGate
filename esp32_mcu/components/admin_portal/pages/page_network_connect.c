#include "page_network_connect.h"

extern const uint8_t _binary_page_network_connect_html_gz_start[];
extern const uint8_t _binary_page_network_connect_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_network_connect = {
    .route = "/network-connect/",
    .alternate_route = "/network-connect",
    .page = ADMIN_PORTAL_PAGE_NETWORK_CONNECT,
    .requires_auth = true,
    .asset_start = _binary_page_network_connect_html_gz_start,
    .asset_end = _binary_page_network_connect_html_gz_end,
    .content_type = "text/html",
};