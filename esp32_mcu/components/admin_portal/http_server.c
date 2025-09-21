#include "http_server.h"
#include "esp_http_server.h"

static const char TAG[] = "HTTP Server";

httpd_handle_t httpserver_handle = NULL;

esp_err_t http_server_start()
{
    if (httpserver_handle != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

void http_server_stop()
{

}

esp_err_t http_server_restart()
{
    http_server_stop();
    return http_server_start();
}