#pragma once

#include "admin_portal_core.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "page_common.h"

esp_err_t page_enroll_render(httpd_req_t *req,
                             const admin_portal_state_t *state,
                             const char *message,
                             admin_focus_t focus);

