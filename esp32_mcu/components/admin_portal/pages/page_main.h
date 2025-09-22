#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

typedef struct {
    const char *version_label;
} page_main_context_t;

esp_err_t page_main_render(httpd_req_t *req, const page_main_context_t *context);

