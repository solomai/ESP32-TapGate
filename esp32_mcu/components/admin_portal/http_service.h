#pragma once

#include "esp_err.h"

struct httpd_req;

esp_err_t http_service_handle_get(struct httpd_req *req);
esp_err_t http_service_handle_post(struct httpd_req *req);
esp_err_t http_service_handle_delete(struct httpd_req *req);
void http_service_on_stop(void);
