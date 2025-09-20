#include "http_server.h"

httpd_handle_t httpserver_handle = NULL;

esp_err_t http_server_start()
{
    if (httpserver_handle != NULL) {
        return ESP_ERR_
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