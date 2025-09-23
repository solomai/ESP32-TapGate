#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t admin_portal_service_init(void);

void admin_portal_service_stop(void);

esp_err_t admin_portal_service_handle(httpd_req_t *req);

