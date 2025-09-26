#include "page_off.h"

extern const uint8_t _binary_page_off_html_gz_start[];
extern const uint8_t _binary_page_off_html_gz_end[];

static const admin_portal_page_descriptor_t descriptor = {
    .uri = "/off/",
    .page = ADMIN_PORTAL_PAGE_OFF,
    .data = _binary_page_off_html_gz_start,
    .size = (size_t)(_binary_page_off_html_gz_end - _binary_page_off_html_gz_start),
    .content_type = "text/html",
    .compressed = true,
};

const admin_portal_page_descriptor_t *admin_portal_page_off_descriptor(void)
{
    return &descriptor;
}
