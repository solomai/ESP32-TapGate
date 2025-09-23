#include "page_main.h"

#include "esp_http_server.h"

#include <stdio.h>

static const char *k_version = "TapGate ver: 1";

esp_err_t http_service_page_main_get_info(struct httpd_req *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    char payload[128];
    int written = snprintf(payload, sizeof(payload), "{\"version\":\"%s\"}", k_version);
    if (written < 0 || written >= (int)sizeof(payload)) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to render response");
    }

    return httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
}
