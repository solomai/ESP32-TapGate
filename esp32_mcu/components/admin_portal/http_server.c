#include "http_server.h"

#include "esp_http_server.h"
#include "http_service.h"
#include "logs.h"

static const char TAG[] = "HTTP Server";

static httpd_handle_t httpserver_handle = NULL;

esp_err_t http_server_start()
{
    if (httpserver_handle != NULL)
        return ESP_ERR_INVALID_STATE;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 64;
    config.lru_purge_enable = true;
    config.max_req_hdr_len = 1024;

    esp_err_t err = httpd_start(&httpserver_handle, &config);
    if (err != ESP_OK)
    {
        LOGE(TAG, "Start failed: %s", esp_err_to_name(err));
        return err;
    }

    esp_err_t svc_err = admin_portal_http_service_start(httpserver_handle);
    if (svc_err != ESP_OK)
    {
        LOGE(TAG, "Failed to start admin portal service: %s", esp_err_to_name(svc_err));
        http_server_stop();
        return svc_err;
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
