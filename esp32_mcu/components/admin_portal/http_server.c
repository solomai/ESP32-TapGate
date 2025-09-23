#include "http_server.h"
#include "esp_http_server.h"
#include "http_service.h"
#include "logs.h"

static const char TAG[] = "HTTP Server";

static httpd_handle_t httpserver_handle = NULL;

static esp_err_t http_server_get_handler(httpd_req_t *req)
{
    LOGI(TAG, "request GET %s", req->uri);
    return admin_portal_service_handle(req);
}

static esp_err_t http_server_post_handler(httpd_req_t *req)
{
    LOGI(TAG, "request POST %s", req->uri);
    return admin_portal_service_handle(req);
}

static esp_err_t http_server_delete_handler(httpd_req_t *req)
{
    LOGI(TAG, "request DELETE %s", req->uri);
    httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Not allowed");
    return ESP_ERR_INVALID_ARG;
}

/* URI wild card for any GET request */
static const httpd_uri_t http_server_get_request = {
    .uri       = "*",
    .method    = HTTP_GET,
    .handler   = http_server_get_handler
};

static const httpd_uri_t http_server_post_request = {
	.uri	= "*",
	.method = HTTP_POST,
	.handler = http_server_post_handler
};

static const httpd_uri_t http_server_delete_request = {
	.uri	= "*",
	.method = HTTP_DELETE,
	.handler = http_server_delete_handler
};

esp_err_t http_server_start()
{
    if (httpserver_handle != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = admin_portal_service_init();
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to initialise admin portal service");
        return err;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // this is an important option that isn't set up by default.
    // We could register all URLs one by one, but this would not work while the fake DNS is active
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.lru_purge_enable = true;
    err = httpd_start(&httpserver_handle, &config);

    if (err == ESP_OK) {
        LOGD(TAG, "Registering URI handlers");
        httpd_register_uri_handler(httpserver_handle, &http_server_get_request);
        httpd_register_uri_handler(httpserver_handle, &http_server_post_request);
        httpd_register_uri_handler(httpserver_handle, &http_server_delete_request);
    }

    return err;
}

void http_server_stop()
{
    if (httpserver_handle != NULL) {
        // stop server
        httpd_stop(httpserver_handle);
        httpserver_handle = NULL;
    }

    admin_portal_service_stop();
}

esp_err_t http_server_restart()
{
    http_server_stop();
    return http_server_start();
}