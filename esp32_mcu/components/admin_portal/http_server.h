#pragma once

#include "esp_err.h"

// Defines the URL where the wifi manager is located
//  By default it is at the server root (ie "/"). If you wish to add your own webpages
//  you may want to relocate the wifi manager to another URL, for instance /wifimanager

#define WEBAPP_LOCATION     "/"

// spawns the http server 
esp_err_t http_server_start();

// stops the http server 
void http_server_stop();

// restart the http server
esp_err_t http_server_restart();


