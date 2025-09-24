#include "http_server.h"
#include "http_service.h"

#include "esp_http_server.h"
#include "logs.h"

static const char TAG[] = "HTTP Server";

static httpd_handle_t httpserver_handle = NULL;

esp_err_t http_server_start()
{
    if (httpserver_handle != NULL)
        return ESP_ERR_INVALID_STATE;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 32;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&httpserver_handle, &config);
    if (err != ESP_OK)
        return err;

    err = admin_portal_http_service_start(httpserver_handle);
    if (err != ESP_OK)
    {
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
