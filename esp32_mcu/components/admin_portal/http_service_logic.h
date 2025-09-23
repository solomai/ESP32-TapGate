#pragma once

#include "esp_err.h"
#include "http_service_types.h"

#define HTTP_SERVICE_URI_ROOT             "/"
#define HTTP_SERVICE_URI_AP_PAGE          "/api/v1/ap"
#define HTTP_SERVICE_URI_MAIN_PAGE        "/api/v1/main"
#define HTTP_SERVICE_URI_AP_PAGE_LEGACY   "/ap.html"
#define HTTP_SERVICE_URI_MAIN_PAGE_LEGACY "/main.html"
#define HTTP_SERVICE_URI_AP_CONFIG        "/api/v1/ap/config"
#define HTTP_SERVICE_URI_AP_ENDPOINT      "/api/v1/ap"
#define HTTP_SERVICE_URI_MAIN_INFO        "/api/v1/main/info"

esp_err_t http_service_logic_validate_credentials(const http_service_credentials_t *credentials,
                                                  char *message,
                                                  size_t message_size);

bool http_service_logic_password_is_valid(const char *password);

const char *http_service_logic_select_initial_uri(const http_service_credentials_t *credentials);

void http_service_logic_prepare_ap_state(const http_service_credentials_t *credentials,
                                         http_service_ap_state_t *state);

void http_service_logic_trim_credentials(http_service_credentials_t *credentials);
