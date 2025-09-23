#pragma once

#include "esp_err.h"

struct httpd_req;

esp_err_t http_service_page_ap_get_config(struct httpd_req *req);

esp_err_t http_service_page_ap_post_config(struct httpd_req *req);
