#include "page_auth.h"

extern const uint8_t _binary_page_auth_html_gz_start[];
extern const uint8_t _binary_page_auth_html_gz_end[];

static const admin_portal_page_descriptor_t descriptor = {
    .uri = "/auth/",
    .page = ADMIN_PORTAL_PAGE_AUTH,
    .data = _binary_page_auth_html_gz_start,
    .size = (size_t)(_binary_page_auth_html_gz_end - _binary_page_auth_html_gz_start),
    .content_type = "text/html",
    .compressed = true,
};

const admin_portal_page_descriptor_t *admin_portal_page_auth_descriptor(void)
{
    return &descriptor;
}
