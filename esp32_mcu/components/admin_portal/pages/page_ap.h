#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_http_server.h"

typedef struct {
    const char *ssid;
    const char *password;
    const char *status_message;
    bool status_is_error;
    bool show_cancel;
    bool highlight_ssid;
    bool highlight_password;
} page_ap_context_t;

esp_err_t page_ap_render(httpd_req_t *req, const page_ap_context_t *context);

