#include "page_wifi.h"

extern const uint8_t _binary_page_wifi_html_gz_start[];
extern const uint8_t _binary_page_wifi_html_gz_end[];

static const admin_portal_page_descriptor_t descriptor = {
    .uri = "/wifi/",
    .page = ADMIN_PORTAL_PAGE_WIFI,
    .data = _binary_page_wifi_html_gz_start,
    .size = (size_t)(_binary_page_wifi_html_gz_end - _binary_page_wifi_html_gz_start),
    .content_type = "text/html",
    .compressed = true,
};

const admin_portal_page_descriptor_t *admin_portal_page_wifi_descriptor(void)
{
    return &descriptor;
}
