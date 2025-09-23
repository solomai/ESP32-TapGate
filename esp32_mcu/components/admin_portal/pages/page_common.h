#pragma once

#include "admin_portal_core.h"
#include "esp_err.h"
#include "esp_http_server.h"

#include <stddef.h>

typedef enum {
    ADMIN_FOCUS_NONE = 0,
    ADMIN_FOCUS_PASSWORD,
    ADMIN_FOCUS_PASSWORD_OLD,
    ADMIN_FOCUS_PASSWORD_NEW,
    ADMIN_FOCUS_SSID,
} admin_focus_t;

typedef struct {
    const char *key;
    const char *value;
} admin_template_pair_t;

esp_err_t admin_page_send_template(httpd_req_t *req,
                                   const uint8_t *template_start,
                                   const uint8_t *template_end,
                                   const admin_template_pair_t *pairs,
                                   size_t pair_count);

