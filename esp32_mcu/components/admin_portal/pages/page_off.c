#include "page_off.h"

extern const uint8_t _binary_page_off_html_gz_start[];
extern const uint8_t _binary_page_off_html_gz_end[];

static admin_portal_page_descriptor_t descriptor = {
    .uri = "/off/",
    .page = ADMIN_PORTAL_PAGE_OFF,
    .data = _binary_page_off_html_gz_start,
    .size = 0,
    .content_type = "text/html",
    .compressed = true,
};

const admin_portal_page_descriptor_t *admin_portal_page_off_descriptor(void)
{
    if (descriptor.size == 0)
        descriptor.size = (size_t)(_binary_page_off_html_gz_end - _binary_page_off_html_gz_start);
    return &descriptor;
}
