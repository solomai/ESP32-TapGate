#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t page_busy_render(httpd_req_t *req);

