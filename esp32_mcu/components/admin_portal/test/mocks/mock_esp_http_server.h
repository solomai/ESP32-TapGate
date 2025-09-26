#pragma once

#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

typedef void *httpd_handle_t;

typedef struct httpd_req {
    void *user_ctx;
    size_t content_len;
} httpd_req_t;

typedef enum {
    HTTP_GET = 0,
    HTTP_POST = 1,
} httpd_method_t;

typedef struct httpd_uri {
    const char *uri;
    httpd_method_t method;
    esp_err_t (*handler)(httpd_req_t *req);
    void *user_ctx;
    void *uri_match_fn;
} httpd_uri_t;
