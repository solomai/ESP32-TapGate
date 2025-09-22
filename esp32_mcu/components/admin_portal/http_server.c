#include "http_server.h"
#include "esp_http_server.h"
#include "logs.h"
#include "http_service.h"

static const char TAG[] = "HTTP Server";

httpd_handle_t httpserver_handle = NULL;

/* strings holding the URLs of the wifi manager */
static esp_err_t http_server_get_handler(httpd_req_t *req)
{
    return http_service_handle_get(req);
}

static esp_err_t http_server_post_handler(httpd_req_t *req)
{
    return http_service_handle_post(req);
}

static esp_err_t http_server_delete_handler(httpd_req_t *req)
{
    return http_service_handle_delete(req);
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

    esp_err_t init_err = http_service_init();
    if (init_err != ESP_OK) {
        return init_err;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // Allow slightly larger URIs and request headers because the admin portal
    // exchanges longer query strings when posting the AP form.
    config.max_uri_len = 256;
    config.max_req_hdr_len = 1024;
    config.max_resp_headers = 16;
    // this is an important option that isn't set up by default.
    // We could register all URLs one by one, but this would not work while the fake DNS is active
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.lru_purge_enable = true;
    esp_err_t err = httpd_start(&httpserver_handle, &config);

    if (err == ESP_OK) {
        LOGD(TAG, "Registering URI handlers");
        httpd_register_uri_handler(httpserver_handle, &http_server_get_request);
        httpd_register_uri_handler(httpserver_handle, &http_server_post_request);
        httpd_register_uri_handler(httpserver_handle, &http_server_delete_request);
    } else {
        http_service_deinit();
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

    http_service_deinit();

}

esp_err_t http_server_restart()
{
    http_server_stop();
    return http_server_start();
}