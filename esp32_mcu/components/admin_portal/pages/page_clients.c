#include "page_clients.h"

extern const uint8_t _binary_page_clients_html_gz_start[];
extern const uint8_t _binary_page_clients_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_clients = {
    .route = "/clients/",
    .alternate_route = "/clients",
    .page = ADMIN_PORTAL_PAGE_CLIENTS,
    .requires_auth = true,
    .asset_start = _binary_page_clients_html_gz_start,
    .asset_end = _binary_page_clients_html_gz_end,
    .content_type = "text/html",
};

