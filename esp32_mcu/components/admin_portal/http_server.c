#include "http_server.h"

#include "esp_http_server.h"

#include "http_service.h"
#include "logs.h"

static const char TAG[] = "HTTP Server";

static httpd_handle_t httpserver_handle = NULL;

static esp_err_t http_server_get_handler(httpd_req_t *req)
{
    LOGD(TAG, "GET %s", req->uri);
    return http_service_handle_get(req);
}

static esp_err_t http_server_post_handler(httpd_req_t *req)
{
    LOGD(TAG, "POST %s", req->uri);
    return http_service_handle_post(req);
}

static esp_err_t http_server_delete_handler(httpd_req_t *req)
{
    LOGD(TAG, "DELETE %s", req->uri);
    return http_service_handle_delete(req);
}

static const httpd_uri_t http_server_get_request = {
    .uri       = "*",
    .method    = HTTP_GET,
    .handler   = http_server_get_handler,
};

static const httpd_uri_t http_server_post_request = {
    .uri       = "*",
    .method    = HTTP_POST,
    .handler   = http_server_post_handler,
};

static const httpd_uri_t http_server_delete_request = {
    .uri       = "*",
    .method    = HTTP_DELETE,
    .handler   = http_server_delete_handler,
};

esp_err_t http_server_start(void)
{
    if (httpserver_handle != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&httpserver_handle, &config);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(err));
        return err;
    }

    LOGD(TAG, "Registering URI handlers");
    httpd_register_uri_handler(httpserver_handle, &http_server_get_request);
    httpd_register_uri_handler(httpserver_handle, &http_server_post_request);
    httpd_register_uri_handler(httpserver_handle, &http_server_delete_request);

    return ESP_OK;
}

void http_server_stop(void)
{
    if (httpserver_handle != NULL) {
        http_service_on_stop();
        httpd_stop(httpserver_handle);
        httpserver_handle = NULL;
    }
}

esp_err_t http_server_restart(void)
{
    http_server_stop();
    return http_server_start();
}
