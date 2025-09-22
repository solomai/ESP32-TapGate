#include "http_service.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"

#include "http_service_facade.h"
#include "logs.h"
#include "page_ap.h"
#include "page_main.h"
#include "wifi_manager.h"

static const char *TAG = "HTTP Service";

static const char *URI_ROOT = "/";
static const char *URI_MAIN = "/api/v1/main";
static const char *URI_AP = "/api/v1/ap";

static bool string_is_blank(const char *text)
{
    if (!text) {
        return true;
    }
    while (*text) {
        if (!isspace((unsigned char)*text)) {
            return false;
        }
        ++text;
    }
    return true;
}

esp_err_t http_service_init(void)
{
    return ESP_OK;
}

void http_service_deinit(void)
{
}

static esp_err_t redirect(httpd_req_t *req, const char *location)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", location);
    return httpd_resp_send(req, NULL, 0);
}

static void url_decode_inplace(char *str)
{
    char *src = str;
    char *dst = str;
    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
            ++src;
        } else if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            char hex[3] = { src[1], src[2], '\0' };
            *dst++ = (char)strtol(hex, NULL, 16);
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static bool parse_form_body(char *body, char *ssid, size_t ssid_len, char *password, size_t password_len)
{
    bool has_ssid = false;
    bool has_password = false;

    char *saveptr = NULL;
    for (char *pair = strtok_r(body, "&", &saveptr); pair; pair = strtok_r(NULL, "&", &saveptr)) {
        char *value = strchr(pair, '=');
        if (!value) {
            continue;
        }

        *value = '\0';
        ++value;
        url_decode_inplace(pair);
        url_decode_inplace(value);

        if (strcmp(pair, "ssid") == 0) {
            strlcpy(ssid, value, ssid_len);
            has_ssid = true;
        } else if (strcmp(pair, "password") == 0) {
            strlcpy(password, value, password_len);
            has_password = true;
        }
    }

    return has_ssid && has_password;
}

static esp_err_t render_ap_page(httpd_req_t *req, const char *status, bool is_error,
                                const char *ssid, const char *password,
                                bool highlight_ssid, bool highlight_password)
{
    page_ap_context_t context = {
        .ssid = ssid,
        .password = password,
        .status_message = status,
        .status_is_error = is_error,
        .show_cancel = http_service_facade_has_valid_ap_password(),
        .highlight_ssid = highlight_ssid,
        .highlight_password = highlight_password,
    };
    return page_ap_render(req, &context);
}

static esp_err_t handle_get_ap(httpd_req_t *req)
{
    char ssid[MAX_SSID_SIZE] = {0};
    char password[MAX_PASSWORD_SIZE] = {0};

    esp_err_t err = http_service_facade_get_ap_credentials(ssid, sizeof(ssid), password, sizeof(password));
    if (err != ESP_OK) {
        LOGW(TAG, "Failed to load AP credentials (%s)", esp_err_to_name(err));
    }

    return render_ap_page(req, "", false, ssid, password, false, false);
}

static esp_err_t handle_get_main(httpd_req_t *req)
{
    page_main_context_t context = {
        .version_label = "TapGate ver: 1",
    };
    return page_main_render(req, &context);
}

esp_err_t http_service_handle_get(httpd_req_t *req)
{
    const char *uri = req->uri;
    LOGI(TAG, "GET %s", uri);

    if (strcmp(uri, URI_ROOT) == 0 || strcmp(uri, "/index.html") == 0) {
        const char *target = http_service_facade_has_valid_ap_password() ? URI_MAIN : URI_AP;
        return redirect(req, target);
    }

    if (strcmp(uri, URI_MAIN) == 0) {
        return handle_get_main(req);
    }

    if (strcmp(uri, URI_AP) == 0) {
        return handle_get_ap(req);
    }

    return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
}

static esp_err_t handle_ap_post(httpd_req_t *req)
{
    if (req->content_len == 0 || req->content_len >= 512) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid payload");
    }

    char body[512] = {0};
    size_t received = 0;
    while (received < req->content_len) {
        int ret = httpd_req_recv(req, body + received, req->content_len - received);
        if (ret <= 0) {
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read request");
        }
        received += ret;
    }
    body[received] = '\0';

    char ssid[MAX_SSID_SIZE] = {0};
    char password[MAX_PASSWORD_SIZE] = {0};
    if (!parse_form_body(body, ssid, sizeof(ssid), password, sizeof(password))) {
        return render_ap_page(req, "Invalid form data", true, ssid, password, true, true);
    }

    if (string_is_blank(ssid)) {
        return render_ap_page(req, "SSID is required", true, ssid, password, true, false);
    }

    if (strlen(password) < WPA2_MINIMUM_PASSWORD_LENGTH) {
        return render_ap_page(req, "Password must be at least 8 characters", true, ssid, password, false, true);
    }

    esp_err_t err = http_service_facade_store_ap_credentials(ssid, password);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to save AP credentials (%s)", esp_err_to_name(err));
        const char *message = (err == ESP_ERR_INVALID_SIZE)
            ? "Values exceed allowed length"
            : "Failed to save credentials";
        bool highlight = (err == ESP_ERR_INVALID_SIZE);
        return render_ap_page(req, message, true, ssid, password, highlight, highlight);
    }

    return redirect(req, URI_MAIN);
}

esp_err_t http_service_handle_post(httpd_req_t *req)
{
    const char *uri = req->uri;
    LOGI(TAG, "POST %s", uri);

    if (strcmp(uri, URI_AP) == 0) {
        return handle_ap_post(req);
    }

    return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
}

esp_err_t http_service_handle_delete(httpd_req_t *req)
{
    LOGI(TAG, "DELETE %s", req->uri);
    return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Unsupported method");
}

