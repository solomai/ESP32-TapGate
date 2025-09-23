#include "http_service.h"

#include "esp_http_server.h"

#include "http_service_facade.h"
#include "http_service_logic.h"
#include "logs.h"
#include "pages/page_ap.h"
#include "pages/page_main.h"

#include <string.h>

static const char *TAG = "HTTP SERVICE";

extern const uint8_t _binary_ap_html_gz_start[];
extern const uint8_t _binary_ap_html_gz_end[];
extern const uint8_t _binary_main_html_gz_start[];
extern const uint8_t _binary_main_html_gz_end[];
extern const uint8_t _binary_styles_css_gz_start[];
extern const uint8_t _binary_styles_css_gz_end[];
extern const uint8_t _binary_ap_js_gz_start[];
extern const uint8_t _binary_ap_js_gz_end[];
extern const uint8_t _binary_main_js_gz_start[];
extern const uint8_t _binary_main_js_gz_end[];

extern const uint8_t _binary_logo_png_start[];
extern const uint8_t _binary_logo_png_end[];
extern const uint8_t _binary_lock_svg_start[];
extern const uint8_t _binary_lock_svg_end[];
extern const uint8_t _binary_settings_svg_start[];
extern const uint8_t _binary_settings_svg_end[];
extern const uint8_t _binary_wifi0_svg_start[];
extern const uint8_t _binary_wifi0_svg_end[];
extern const uint8_t _binary_wifi1_svg_start[];
extern const uint8_t _binary_wifi1_svg_end[];
extern const uint8_t _binary_wifi2_svg_start[];
extern const uint8_t _binary_wifi2_svg_end[];
extern const uint8_t _binary_wifi3_svg_start[];
extern const uint8_t _binary_wifi3_svg_end[];

static esp_err_t send_redirect(httpd_req_t *req, const char *uri)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", uri);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t send_gzip(httpd_req_t *req, const uint8_t *start, const uint8_t *end, const char *content_type)
{
    if (!start || !end || end <= start) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Missing resource");
    }

    size_t length = (size_t)(end - start);
    httpd_resp_set_type(req, content_type);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    return httpd_resp_send(req, (const char *)start, length);
}

static esp_err_t send_binary(httpd_req_t *req, const uint8_t *start, const uint8_t *end, const char *content_type)
{
    if (!start || !end || end <= start) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Missing asset");
    }

    size_t length = (size_t)(end - start);
    httpd_resp_set_type(req, content_type);
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    return httpd_resp_send(req, (const char *)start, length);
}

static esp_err_t handle_root(httpd_req_t *req)
{
    http_service_credentials_t credentials = {0};
    esp_err_t err = http_service_facade_get_ap_credentials(&credentials);
    if (err != ESP_OK) {
        LOGW(TAG, "Failed to load credentials: %s", esp_err_to_name(err));
    }

    const char *target = http_service_logic_select_initial_uri(&credentials);
    return send_redirect(req, target);
}

static esp_err_t handle_assets(httpd_req_t *req)
{
    const char *uri = req->uri;
    if (strcmp(uri, "/assets/logo.png") == 0) {
        return send_binary(req, _binary_logo_png_start, _binary_logo_png_end, "image/png");
    }
    if (strcmp(uri, "/assets/lock.svg") == 0) {
        return send_binary(req, _binary_lock_svg_start, _binary_lock_svg_end, "image/svg+xml");
    }
    if (strcmp(uri, "/assets/settings.svg") == 0) {
        return send_binary(req, _binary_settings_svg_start, _binary_settings_svg_end, "image/svg+xml");
    }
    if (strcmp(uri, "/assets/wifi0.svg") == 0) {
        return send_binary(req, _binary_wifi0_svg_start, _binary_wifi0_svg_end, "image/svg+xml");
    }
    if (strcmp(uri, "/assets/wifi1.svg") == 0) {
        return send_binary(req, _binary_wifi1_svg_start, _binary_wifi1_svg_end, "image/svg+xml");
    }
    if (strcmp(uri, "/assets/wifi2.svg") == 0) {
        return send_binary(req, _binary_wifi2_svg_start, _binary_wifi2_svg_end, "image/svg+xml");
    }
    if (strcmp(uri, "/assets/wifi3.svg") == 0) {
        return send_binary(req, _binary_wifi3_svg_start, _binary_wifi3_svg_end, "image/svg+xml");
    }

    return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Asset not found");
}

static esp_err_t dispatch_get(httpd_req_t *req)
{
    const char *uri = req->uri;

    if (strcmp(uri, HTTP_SERVICE_URI_ROOT) == 0 || strcmp(uri, "/index.html") == 0) {
        return handle_root(req);
    }
    if (strcmp(uri, HTTP_SERVICE_URI_AP_PAGE) == 0) {
        return send_gzip(req, _binary_ap_html_gz_start, _binary_ap_html_gz_end, "text/html");
    }
    if (strcmp(uri, HTTP_SERVICE_URI_MAIN_PAGE) == 0) {
        return send_gzip(req, _binary_main_html_gz_start, _binary_main_html_gz_end, "text/html");
    }
    if (strcmp(uri, "/styles.css") == 0) {
        return send_gzip(req, _binary_styles_css_gz_start, _binary_styles_css_gz_end, "text/css");
    }
    if (strcmp(uri, "/ap.js") == 0) {
        return send_gzip(req, _binary_ap_js_gz_start, _binary_ap_js_gz_end, "application/javascript");
    }
    if (strcmp(uri, "/main.js") == 0) {
        return send_gzip(req, _binary_main_js_gz_start, _binary_main_js_gz_end, "application/javascript");
    }
    if (strncmp(uri, "/assets/", 8) == 0) {
        return handle_assets(req);
    }
    if (strcmp(uri, "/api/v1/ap/config") == 0) {
        return http_service_page_ap_get_config(req);
    }
    if (strcmp(uri, "/api/v1/main/info") == 0) {
        return http_service_page_main_get_info(req);
    }

    return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
}

static esp_err_t dispatch_post(httpd_req_t *req)
{
    if (strcmp(req->uri, "/api/v1/ap/config") == 0) {
        return http_service_page_ap_post_config(req);
    }
    return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
}

esp_err_t http_service_handle_get(struct httpd_req *req)
{
    return dispatch_get(req);
}

esp_err_t http_service_handle_post(struct httpd_req *req)
{
    return dispatch_post(req);
}

esp_err_t http_service_handle_delete(struct httpd_req *req)
{
    return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Not allowed");
}

void http_service_on_stop(void)
{
}
