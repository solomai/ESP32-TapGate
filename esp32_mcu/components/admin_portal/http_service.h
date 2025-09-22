#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

// Initialize any state required by the HTTP service.
esp_err_t http_service_init(void);

// Release resources allocated by the HTTP service.
void http_service_deinit(void);

// Handle HTTP GET requests routed by the server.
esp_err_t http_service_handle_get(httpd_req_t *req);

// Handle HTTP POST requests routed by the server.
esp_err_t http_service_handle_post(httpd_req_t *req);

// Handle HTTP DELETE requests routed by the server.
esp_err_t http_service_handle_delete(httpd_req_t *req);
