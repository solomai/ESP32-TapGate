#include "page_ap.h"

#include "esp_http_server.h"

#include "http_service_facade.h"
#include "http_service_logic.h"
#include "logs.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define FORM_BUFFER_SIZE 256
#define MESSAGE_BUFFER_SIZE 128

static const char *TAG = "HTTP AP";

static esp_err_t send_json(httpd_req_t *req, const char *payload)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
}

static int from_hex(char value)
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

static void url_decode_inplace(char *value)
{
    char *src = value;
    char *dst = value;
    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
            ++src;
        } else if (*src == '%' && src[1] && src[2]) {
            int hi = from_hex(src[1]);
            int lo = from_hex(src[2]);
            if (hi >= 0 && lo >= 0) {
                *dst++ = (char)((hi << 4) | lo);
                src += 3;
            } else {
                *dst++ = *src++;
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static void escape_json(const char *value, char *buffer, size_t buffer_size)
{
    size_t index = 0;
    if (!value || !buffer || buffer_size == 0) {
        return;
    }

    for (const unsigned char *ptr = (const unsigned char *)value; *ptr && index + 1 < buffer_size; ++ptr) {
        if (*ptr < 0x20) {
            continue;
        }
        if (*ptr == '"' || *ptr == '\\') {
            if (index + 2 >= buffer_size) {
                break;
            }
            buffer[index++] = '\\';
            buffer[index++] = (char)*ptr;
        } else {
            buffer[index++] = (char)*ptr;
        }
    }
    buffer[index] = '\0';
}

static esp_err_t build_success_response(httpd_req_t *req)
{
    char payload[128] = {0};
    snprintf(payload,
             sizeof(payload),
             "{\"status\":\"ok\",\"redirect\":\"%s\"}",
             HTTP_SERVICE_URI_MAIN_PAGE);
    return send_json(req, payload);
}

static esp_err_t build_error_response(httpd_req_t *req, const char *message)
{
    char escaped[MESSAGE_BUFFER_SIZE * 2] = {0};
    escape_json(message ? message : "Unknown error", escaped, sizeof(escaped));
    char payload[MESSAGE_BUFFER_SIZE * 2 + 64] = {0};
    snprintf(payload, sizeof(payload), "{\"status\":\"error\",\"message\":\"%s\"}", escaped);
    httpd_resp_set_status(req, "400 Bad Request");
    return send_json(req, payload);
}

static esp_err_t build_internal_error(httpd_req_t *req)
{
    httpd_resp_set_status(req, "500 Internal Server Error");
    return send_json(req, "{\"status\":\"error\",\"message\":\"Internal error\"}");
}

static esp_err_t parse_form(char *body, http_service_credentials_t *credentials)
{
    if (!body || !credentials) {
        return ESP_ERR_INVALID_ARG;
    }

    http_service_credentials_t temp = {0};
    bool ssid_found = false;
    bool password_found = false;

    char *save_ptr = NULL;
    for (char *token = strtok_r(body, "&", &save_ptr); token; token = strtok_r(NULL, "&", &save_ptr)) {
        if (strncmp(token, "ssid=", 5) == 0) {
            char *value = token + 5;
            url_decode_inplace(value);
            strncpy(temp.ssid, value, sizeof(temp.ssid) - 1);
            ssid_found = true;
        } else if (strncmp(token, "password=", 9) == 0) {
            char *value = token + 9;
            url_decode_inplace(value);
            strncpy(temp.password, value, sizeof(temp.password) - 1);
            password_found = true;
        }
    }

    if (!ssid_found || !password_found) {
        return ESP_ERR_INVALID_ARG;
    }

    http_service_logic_trim_credentials(&temp);
    *credentials = temp;
    return ESP_OK;
}

esp_err_t http_service_page_ap_get_config(httpd_req_t *req)
{
    http_service_credentials_t credentials = {0};
    esp_err_t err = http_service_facade_get_ap_credentials(&credentials);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to read AP credentials: %s", esp_err_to_name(err));
        return build_internal_error(req);
    }

    http_service_ap_state_t state = {0};
    http_service_logic_prepare_ap_state(&credentials, &state);

    char escaped_ssid[MAX_SSID_SIZE * 2] = {0};
    char escaped_pwd[MAX_PASSWORD_SIZE * 2] = {0};
    escape_json(state.credentials.ssid, escaped_ssid, sizeof(escaped_ssid));
    escape_json(state.credentials.password, escaped_pwd, sizeof(escaped_pwd));

    char payload[256] = {0};
    snprintf(payload, sizeof(payload),
             "{\"ssid\":\"%s\",\"password\":\"%s\",\"cancel\":%s}",
             escaped_ssid,
             escaped_pwd,
             state.cancel_enabled ? "true" : "false");

    return send_json(req, payload);
}

esp_err_t http_service_page_ap_post_config(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len >= FORM_BUFFER_SIZE) {
        LOGW(TAG, "Invalid content length: %zu", req->content_len);
        return build_error_response(req, "Invalid request");
    }

    char body[FORM_BUFFER_SIZE] = {0};
    size_t remaining = req->content_len;
    size_t offset = 0;
    while (remaining > 0) {
        int received = httpd_req_recv(req, body + offset, remaining);
        if (received <= 0) {
            LOGE(TAG, "Failed to receive body: %d", received);
            return build_internal_error(req);
        }
        offset += (size_t)received;
        remaining -= (size_t)received;
    }
    body[offset] = '\0';

    http_service_credentials_t credentials = {0};
    esp_err_t err = parse_form(body, &credentials);
    if (err != ESP_OK) {
        return build_error_response(req, "Invalid form data");
    }

    char message[MESSAGE_BUFFER_SIZE] = {0};
    err = http_service_logic_validate_credentials(&credentials, message, sizeof(message));
    if (err != ESP_OK) {
        return build_error_response(req, message);
    }

    http_service_logic_trim_credentials(&credentials);

    err = http_service_facade_save_ap_credentials(&credentials);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to save credentials: %s", esp_err_to_name(err));
        return build_internal_error(req);
    }

    return build_success_response(req);
}
