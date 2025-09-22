#include "http_service.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "http_service_facade.h"
#include "logs.h"
#include "pages/pages.h"

static const char *TAG = "HTTP Service";

extern const uint8_t _binary_assets_logo_png_start[] asm("_binary_assets_logo_png_start");
extern const uint8_t _binary_assets_logo_png_end[] asm("_binary_assets_logo_png_end");

#define HTTP_SERVICE_BODY_MAX_LEN 512
static const char HEX_DIGITS[] = "0123456789ABCDEF";

static esp_err_t http_service_send_redirect(httpd_req_t *req, const char *location);
static esp_err_t http_service_send_html(httpd_req_t *req, const char *html);
static esp_err_t http_service_send_json(httpd_req_t *req, const char *json);
static esp_err_t http_service_send_error_json(httpd_req_t *req, const char *status_code, const char *field, const char *message);
static esp_err_t http_service_handle_root(httpd_req_t *req);
static esp_err_t http_service_handle_ap_page(httpd_req_t *req);
static esp_err_t http_service_handle_main_page(httpd_req_t *req);
static esp_err_t http_service_handle_ap_config_get(httpd_req_t *req);
static esp_err_t http_service_handle_ap_config_post(httpd_req_t *req);
static esp_err_t http_service_handle_logo_asset(httpd_req_t *req);
static esp_err_t http_service_read_body(httpd_req_t *req, char *buffer, size_t buffer_len, size_t *out_len);
static bool http_service_extract_form_field(const char *body, const char *key, char *out, size_t out_len);
static bool http_service_url_decode(char *dest, size_t dest_size, const char *src, size_t src_len);
static int http_service_hex_value(char c);
static void http_service_trim(char *value);
static bool http_service_json_escape(const char *input, char *output, size_t output_size);

esp_err_t http_service_handle_get(httpd_req_t *req)
{
    const char *uri = req->uri;

    if (strcmp(uri, "/") == 0 || strcmp(uri, WEBAPP_LOCATION) == 0) {
        return http_service_handle_root(req);
    }

    if (strcmp(uri, "/api/v1/ap") == 0 || strcmp(uri, "/api/v1/ap/") == 0) {
        return http_service_handle_ap_page(req);
    }

    if (strcmp(uri, "/api/v1/main") == 0 || strcmp(uri, "/api/v1/main/") == 0) {
        return http_service_handle_main_page(req);
    }

    if (strcmp(uri, "/api/v1/ap/config") == 0) {
        return http_service_handle_ap_config_get(req);
    }

    if (strcmp(uri, "/assets/logo.png") == 0) {
        return http_service_handle_logo_asset(req);
    }

    if (strncmp(uri, "/api/", 5) == 0 || strncmp(uri, "/assets/", 8) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_FAIL;
    }

    return http_service_handle_root(req);
}

esp_err_t http_service_handle_post(httpd_req_t *req)
{
    const char *uri = req->uri;

    if (strcmp(uri, "/api/v1/ap/config") == 0) {
        return http_service_handle_ap_config_post(req);
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
    return ESP_FAIL;
}

esp_err_t http_service_handle_delete(httpd_req_t *req)
{
    httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
    return ESP_FAIL;
}

void http_service_on_server_stop(void)
{
    // Currently nothing to release.
}

static esp_err_t http_service_handle_root(httpd_req_t *req)
{
    http_service_credentials_t credentials;
    bool has_valid_password = false;
    esp_err_t err = http_service_facade_load_ap_credentials(&credentials, &has_valid_password);
    if (err != ESP_OK) {
        LOGW(TAG, "Unable to load AP credentials (%s)", esp_err_to_name(err));
    }

    const char *target = has_valid_password ? "/api/v1/main/" : "/api/v1/ap/";
    return http_service_send_redirect(req, target);
}

static esp_err_t http_service_handle_ap_page(httpd_req_t *req)
{
    return http_service_send_html(req, http_service_page_ap_html());
}

static esp_err_t http_service_handle_main_page(httpd_req_t *req)
{
    return http_service_send_html(req, http_service_page_main_html());
}

static esp_err_t http_service_handle_ap_config_get(httpd_req_t *req)
{
    http_service_credentials_t credentials;
    bool has_valid_password = false;
    esp_err_t err = http_service_facade_load_ap_credentials(&credentials, &has_valid_password);
    if (err != ESP_OK) {
        return http_service_send_error_json(req, "500 Internal Server Error", NULL, "Failed to read configuration");
    }

    char ssid_json[128];
    char password_json[160];
    if (!http_service_json_escape(credentials.ssid, ssid_json, sizeof(ssid_json)) ||
        !http_service_json_escape(credentials.password, password_json, sizeof(password_json))) {
        return http_service_send_error_json(req, "500 Internal Server Error", NULL, "Failed to encode configuration");
    }

    char response[256];
    int written = snprintf(response, sizeof(response),
                           "{\"ssid\":\"%s\",\"password\":\"%s\",\"hasValidPassword\":%s}",
                           ssid_json,
                           password_json,
                           has_valid_password ? "true" : "false");
    if (written < 0 || written >= (int)sizeof(response)) {
        return http_service_send_error_json(req, "500 Internal Server Error", NULL, "Failed to encode configuration");
    }

    return http_service_send_json(req, response);
}

static esp_err_t http_service_handle_ap_config_post(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len >= HTTP_SERVICE_BODY_MAX_LEN) {
        return http_service_send_error_json(req, "400 Bad Request", NULL, "Invalid request size");
    }

    char body[HTTP_SERVICE_BODY_MAX_LEN];
    esp_err_t err = http_service_read_body(req, body, sizeof(body), NULL);
    if (err != ESP_OK) {
        return http_service_send_error_json(req, "400 Bad Request", NULL, "Failed to read request");
    }

    http_service_credentials_t credentials = {0};
    if (!http_service_extract_form_field(body, "ssid", credentials.ssid, sizeof(credentials.ssid))) {
        return http_service_send_error_json(req, "400 Bad Request", "ssid", "SSID is required");
    }
    if (!http_service_extract_form_field(body, "password", credentials.password, sizeof(credentials.password))) {
        return http_service_send_error_json(req, "400 Bad Request", "password", "Password is required");
    }

    http_service_trim(credentials.ssid);
    http_service_trim(credentials.password);

    if (!http_service_facade_is_ssid_valid(credentials.ssid)) {
        return http_service_send_error_json(req, "400 Bad Request", "ssid", "SSID must be between 1 and 32 characters");
    }

    if (!http_service_facade_is_password_valid(credentials.password)) {
        return http_service_send_error_json(req, "400 Bad Request", "password", "Password must be between 8 and 63 characters");
    }

    err = http_service_facade_store_ap_credentials(&credentials);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to store AP credentials (%s)", esp_err_to_name(err));
        return http_service_send_error_json(req, "500 Internal Server Error", NULL, "Unable to save configuration");
    }

    return http_service_send_json(req, "{\"status\":\"ok\"}");
}

static esp_err_t http_service_handle_logo_asset(httpd_req_t *req)
{
    size_t length = (size_t)(_binary_assets_logo_png_end - _binary_assets_logo_png_start);
    httpd_resp_set_type(req, "image/png");
    return httpd_resp_send(req, (const char *)_binary_assets_logo_png_start, length);
}

static esp_err_t http_service_send_redirect(httpd_req_t *req, const char *location)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", location);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t http_service_send_html(httpd_req_t *req, const char *html)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t http_service_send_json(httpd_req_t *req, const char *json)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t http_service_send_error_json(httpd_req_t *req, const char *status_code, const char *field, const char *message)
{
    httpd_resp_set_status(req, status_code);

    char message_json[192];
    if (!http_service_json_escape(message ? message : "", message_json, sizeof(message_json))) {
        return http_service_send_json(req, "{\"status\":\"error\",\"message\":\"Unexpected encoding error\"}");
    }

    char response[256];
    if (field && field[0] != '\0') {
        char field_json[64];
        if (!http_service_json_escape(field, field_json, sizeof(field_json))) {
            return http_service_send_json(req, "{\"status\":\"error\",\"message\":\"Unexpected encoding error\"}");
        }
        snprintf(response, sizeof(response),
                 "{\"status\":\"error\",\"message\":\"%s\",\"field\":\"%s\"}",
                 message_json,
                 field_json);
    } else {
        snprintf(response, sizeof(response),
                 "{\"status\":\"error\",\"message\":\"%s\"}",
                 message_json);
    }

    return http_service_send_json(req, response);
}

static esp_err_t http_service_read_body(httpd_req_t *req, char *buffer, size_t buffer_len, size_t *out_len)
{
    size_t received = 0;
    while (received < req->content_len && received < buffer_len - 1) {
        size_t remaining = req->content_len - received;
        if (remaining > (buffer_len - 1 - received)) {
            remaining = buffer_len - 1 - received;
        }

        int chunk = httpd_req_recv(req, buffer + received, remaining);
        if (chunk == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (chunk <= 0) {
            return ESP_FAIL;
        }
        received += (size_t)chunk;
    }

    if (received >= buffer_len) {
        return ESP_ERR_INVALID_SIZE;
    }

    buffer[received] = '\0';
    if (out_len) {
        *out_len = received;
    }
    return ESP_OK;
}

static bool http_service_extract_form_field(const char *body, const char *key, char *out, size_t out_len)
{
    const size_t key_len = strlen(key);
    const char *cursor = body;

    while (cursor && *cursor) {
        const char *match = strstr(cursor, key);
        if (match == NULL) {
            return false;
        }

        bool at_start = (match == body) || (*(match - 1) == '&');
        if (at_start && match[key_len] == '=') {
            const char *value_start = match + key_len + 1;
            const char *value_end = strchr(value_start, '&');
            size_t encoded_len = value_end ? (size_t)(value_end - value_start) : strlen(value_start);
            return http_service_url_decode(out, out_len, value_start, encoded_len);
        }

        cursor = strchr(match, '&');
        if (cursor) {
            cursor++;
        }
    }

    return false;
}

static bool http_service_url_decode(char *dest, size_t dest_size, const char *src, size_t src_len)
{
    size_t di = 0;
    size_t si = 0;

    while (si < src_len) {
        if (di + 1 >= dest_size) {
            dest[0] = '\0';
            return false;
        }

        char c = src[si++];
        if (c == '+') {
            dest[di++] = ' ';
            continue;
        }

        if (c == '%' && si + 1 < src_len) {
            int hi = http_service_hex_value(src[si]);
            int lo = http_service_hex_value(src[si + 1]);
            if (hi >= 0 && lo >= 0) {
                dest[di++] = (char)((hi << 4) | lo);
                si += 2;
                continue;
            }
        }

        dest[di++] = c;
    }

    dest[di] = '\0';
    return true;
}

static int http_service_hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    return -1;
}

static void http_service_trim(char *value)
{
    if (value == NULL) {
        return;
    }

    size_t len = strlen(value);
    size_t start = 0;
    while (start < len && isspace((unsigned char)value[start])) {
        start++;
    }

    size_t end = len;
    while (end > start && isspace((unsigned char)value[end - 1])) {
        end--;
    }

    if (start > 0) {
        memmove(value, value + start, end - start);
    }
    value[end - start] = '\0';
}

static bool http_service_json_escape(const char *input, char *output, size_t output_size)
{
    size_t oi = 0;
    for (size_t ii = 0; input[ii] != '\0'; ++ii) {
        unsigned char ch = (unsigned char)input[ii];
        switch (ch) {
            case '"':
            case '\\':
                if (oi + 2 >= output_size) {
                    return false;
                }
                output[oi++] = '\\';
                output[oi++] = (char)ch;
                break;
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                if (oi + 2 >= output_size) {
                    return false;
                }
                output[oi++] = '\\';
                switch (ch) {
                    case '\b': output[oi++] = 'b'; break;
                    case '\f': output[oi++] = 'f'; break;
                    case '\n': output[oi++] = 'n'; break;
                    case '\r': output[oi++] = 'r'; break;
                    case '\t': output[oi++] = 't'; break;
                    default: break;
                }
                break;
            default:
                if (ch < 0x20) {
                    if (oi + 6 >= output_size) {
                        return false;
                    }
                    output[oi++] = '\\';
                    output[oi++] = 'u';
                    output[oi++] = '0';
                    output[oi++] = '0';
                    output[oi++] = HEX_DIGITS[(ch >> 4) & 0x0F];
                    output[oi++] = HEX_DIGITS[ch & 0x0F];
                } else {
                    if (oi + 1 >= output_size) {
                        return false;
                    }
                    output[oi++] = (char)ch;
                }
                break;
        }
    }

    if (oi >= output_size) {
        return false;
    }

    output[oi] = '\0';
    return true;
}
