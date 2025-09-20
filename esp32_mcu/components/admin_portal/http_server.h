#pragma once

#include "esp_err.h"

static const char TAG[] = "HTTP Server";

// spawns the http server 
esp_err_t http_server_start();

// stops the http server 
void http_server_stop();

// restart the http server
esp_err_t http_server_restart();


