#pragma once

#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Event journal log levels
 */
enum event_journal_type {
    EVENT_JOURNAL_INFO,     /*!< Informational message */
    EVENT_JOURNAL_WARNING,  /*!< Warning message */
    EVENT_JOURNAL_ERROR,    /*!< Error message */
    EVENT_JOURNAL_ALERT     /*!< Alert message */
};

/**
 * @brief Internal function for persistent storage (private, do not use directly)
 */
void _event_journal_store(enum event_journal_type type, const char *tag, const char *fmt, ...);

/**
 * @brief Add an event to the journal (macro implementation)
 * 
 * This macro logs an event with the specified type, tag, and formatted message.
 */
#define EVENT_JOURNAL_ADD(type, tag, fmt, ...) \
    do { \
        _event_journal_store(type, tag, fmt, ##__VA_ARGS__); \
        switch (type) { \
            case EVENT_JOURNAL_INFO: \
                ESP_LOGI(tag, fmt, ##__VA_ARGS__); \
                break; \
            case EVENT_JOURNAL_WARNING: \
                ESP_LOGW(tag, fmt, ##__VA_ARGS__); \
                break; \
            case EVENT_JOURNAL_ERROR: \
                ESP_LOGE(tag, fmt, ##__VA_ARGS__); \
                break; \
            case EVENT_JOURNAL_ALERT: \
                ESP_LOGW(tag, fmt, ##__VA_ARGS__); \
                break; \
            default: \
                ESP_LOGW("EventJournal", "Unknown event journal type: %d", type); \
                break; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif