#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t esp_err_t;

#define ESP_OK           0
#define ESP_FAIL        -1
#define ESP_ERR_NO_MEM   0x101
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_NOT_FOUND   0x103
#define ESP_ERR_TIMEOUT     0x104
#define ESP_ERR_INVALID_STATE 0x105
#define ESP_ERR_INVALID_SIZE  0x106

static inline const char *esp_err_to_name(esp_err_t code)
{
    switch (code)
    {
        case ESP_OK:
            return "ESP_OK";
        case ESP_FAIL:
            return "ESP_FAIL";
        case ESP_ERR_NO_MEM:
            return "ESP_ERR_NO_MEM";
        case ESP_ERR_INVALID_ARG:
            return "ESP_ERR_INVALID_ARG";
        case ESP_ERR_NOT_FOUND:
            return "ESP_ERR_NOT_FOUND";
        case ESP_ERR_TIMEOUT:
            return "ESP_ERR_TIMEOUT";
        case ESP_ERR_INVALID_STATE:
            return "ESP_ERR_INVALID_STATE";
        case ESP_ERR_INVALID_SIZE:
            return "ESP_ERR_INVALID_SIZE";
        default:
            return "ESP_ERR_UNKNOWN";
    }
}

#define ESP_ERROR_CHECK(x)                                                      \
    do {                                                                        \
        esp_err_t __err_rc = (x);                                               \
        if (__err_rc != ESP_OK)                                                 \
        {                                                                       \
            fprintf(stderr, "ESP_ERROR_CHECK failed: %s (%d)\n",              \
                    esp_err_to_name(__err_rc), (int)__err_rc);                  \
            abort();                                                            \
        }                                                                       \
    } while (0)

#define ESP_ERROR_CHECK_WITHOUT_ABORT(x)                                        \
    do {                                                                        \
        esp_err_t __err_rc = (x);                                               \
        if (__err_rc != ESP_OK)                                                 \
        {                                                                       \
            fprintf(stderr, "ESP_ERROR_CHECK_WITHOUT_ABORT failed: %s (%d)\n", \
                    esp_err_to_name(__err_rc), (int)__err_rc);                  \
        }                                                                       \
    } while (0)

#ifdef __cplusplus
}
#endif

