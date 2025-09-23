#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "admin_portal_state.h"

typedef struct {
    const char *route;
    const char *alternate_route;
    admin_portal_page_t page;
    bool requires_auth;
    const uint8_t *asset_start;
    const uint8_t *asset_end;
    const char *content_type;
} admin_portal_page_descriptor_t;

