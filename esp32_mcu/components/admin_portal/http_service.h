#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t http_service_handle_get(httpd_req_t *req);
esp_err_t http_service_handle_post(httpd_req_t *req);
esp_err_t http_service_handle_delete(httpd_req_t *req);
void http_service_on_server_stop(void);
