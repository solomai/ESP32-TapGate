#include "logs.h"

#include "esp_log.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef LOGS_MESSAGE_BUFFER_SIZE
#define LOGS_MESSAGE_BUFFER_SIZE 256
#endif

static void logs_vlog(esp_log_level_t level, const char *tag, const char *fmt, va_list args)
{
    char message[LOGS_MESSAGE_BUFFER_SIZE];
    int written;

    if (!fmt)
        fmt = "";

    written = vsnprintf(message, sizeof(message), fmt, args);
    if (written < 0)
    {
        message[0] = '\0';
    }
    else if ((size_t)written >= sizeof(message))
    {
        message[sizeof(message) - 1] = '\0';
    }

    if (!tag)
        tag = "";

    switch (level)
    {
    case ESP_LOG_ERROR:
        ESP_LOGE(tag, "%s", message);
        break;
    case ESP_LOG_WARN:
        ESP_LOGW(tag, "%s", message);
        break;
    case ESP_LOG_INFO:
        ESP_LOGI(tag, "%s", message);
        break;
    case ESP_LOG_DEBUG:
    default:
        ESP_LOGD(tag, "%s", message);
        break;
    }
}

void LOGD(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logs_vlog(ESP_LOG_DEBUG, tag, fmt, args);
    va_end(args);
}

void LOGI(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logs_vlog(ESP_LOG_INFO, tag, fmt, args);
    va_end(args);
}

void LOGW(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logs_vlog(ESP_LOG_WARN, tag, fmt, args);
    va_end(args);
}

void LOGE(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logs_vlog(ESP_LOG_ERROR, tag, fmt, args);
    va_end(args);
}

