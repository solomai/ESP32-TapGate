#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t admin_portal_http_service_start(httpd_handle_t server);
void admin_portal_http_service_stop(void);

void admin_portal_notify_client_disconnected(const char *client_ip);

