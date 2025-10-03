#include "http_server.h"
#include "http_service.h"

#include "esp_http_server.h"
#include "logs.h"

static const char TAG[] = "HTTP Server";

static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");
    
    httpd_resp_set_type(req, "image/x-icon");
    return httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);
}

static httpd_handle_t httpserver_handle = NULL;

esp_err_t http_server_start()
{
    if (httpserver_handle != NULL)
        return ESP_ERR_INVALID_STATE;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 48; // Increased from 32 to accommodate all endpoints
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&httpserver_handle, &config);
    if (err != ESP_OK)
    {
        LOGE(TAG, "Start failed: %s", esp_err_to_name(err));
        return err;
    }

    // Register favicon handler
    extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");
    httpd_uri_t favicon_uri = {
        .uri = "/favicon.ico",
        .method = HTTP_GET,
        .handler = favicon_get_handler,
        .user_ctx = NULL
    };
    err = httpd_register_uri_handler(httpserver_handle, &favicon_uri);
    if (err != ESP_OK) {
        LOGW(TAG, "Failed to register favicon handler: %s", esp_err_to_name(err));
        // Continue anyway as this is not critical
    }

    err = admin_portal_http_service_start(httpserver_handle);
    if (err != ESP_OK)
    {
        LOGE(TAG, "Admim Portal start failed: %s", esp_err_to_name(err));
        httpd_stop(httpserver_handle);
        httpserver_handle = NULL;
        return err;
    }

    LOGI(TAG, "HTTP server started");
    return ESP_OK;
}

void http_server_stop()
{
    if (httpserver_handle == NULL)
        return;

    admin_portal_http_service_stop();
    httpd_stop(httpserver_handle);
    httpserver_handle = NULL;
    LOGI(TAG, "HTTP server stopped");
}

esp_err_t http_server_restart()
{
    http_server_stop();
    return http_server_start();
}
