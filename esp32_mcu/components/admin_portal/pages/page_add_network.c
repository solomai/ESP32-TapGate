#include "page_add_network.h"

extern const uint8_t _binary_page_add_network_html_gz_start[];
extern const uint8_t _binary_page_add_network_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_add_network = {
    .route = "/add-network/",
    .alternate_route = "/add-network", 
    .page = ADMIN_PORTAL_PAGE_ADD_NETWORK,
    .requires_auth = true,
    .asset_start = _binary_page_add_network_html_gz_start,
    .asset_end = _binary_page_add_network_html_gz_end,
    .content_type = "text/html",
};