#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "admin_portal_core.h"

esp_err_t page_wifi_render(httpd_req_t *req, const admin_portal_state_t *state);

