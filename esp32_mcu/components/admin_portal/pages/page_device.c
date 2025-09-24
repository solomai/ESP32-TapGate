#include "page_device.h"

extern const uint8_t _binary_page_device_html_gz_start[];
extern const uint8_t _binary_page_device_html_gz_end[];

const admin_portal_page_descriptor_t admin_portal_page_device = {
    .route = "/device/",
    .alternate_route = "/device",
    .page = ADMIN_PORTAL_PAGE_DEVICE,
    .requires_auth = true,
    .asset_start = _binary_page_device_html_gz_start,
    .asset_end = _binary_page_device_html_gz_end,
    .content_type = "text/html",
};

