#pragma once

#include "esp_err.h"

struct httpd_req;

esp_err_t http_service_page_main_get_info(struct httpd_req *req);
