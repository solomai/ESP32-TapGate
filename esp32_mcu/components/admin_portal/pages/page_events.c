#include "page_events.h"

extern const uint8_t _binary_page_events_html_gz_start[];
extern const uint8_t _binary_page_events_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_events = {
    .route = "/events/",
    .alternate_route = "/events",
    .page = ADMIN_PORTAL_PAGE_EVENTS,
    .requires_auth = true,
    .asset_start = _binary_page_events_html_gz_start,
    .asset_end = _binary_page_events_html_gz_end,
    .content_type = "text/html",
};

