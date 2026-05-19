/**
 * Persistent event journaling module for ESP-IDF.
 *
 * This module implements a simple persistent event journal intended for
 * embedded systems based on ESP-IDF. Its main goal is to preserve important
 * runtime information across reboots, allowing developers and support tools
 * to analyze system behavior and failures after the device has restarted.
 *
 * Events are recorded with an associated severity level and stored using an
 * internal persistence mechanism. At the same time, every event is forwarded
 * to the standard ESP-IDF logging system, ensuring that normal runtime logs
 * remain available during development and debugging, while critical
 * information is still retained for post-reboot inspection.
 *
 * The persistent storage backend is treated as an internal implementation
 * detail and must not be accessed directly by application code. All interaction
 * with the journal is performed through a single macro, which guarantees
 * consistent behavior and minimal integration effort.
 *
 * The module is designed to be lightweight, predictable, and suitable for
 * long-running or unattended devices where reliable diagnostics are required.
 */

#pragma once

#include "device_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// Log tag used for all event journal output.
#define EVENT_JOURNAL_TAG ">> EVENT <<"

// Event journal log levels
enum event_journal_type {
    EVENT_JOURNAL_INFO,     // Informational message
    EVENT_JOURNAL_WARNING,  // Warning message
    EVENT_JOURNAL_ERROR,    // Error message
    EVENT_JOURNAL_ALERT     // Alert message
};

// Internal function for persistent storage (private, do not use directly)
void _event_journal_store(enum event_journal_type type, const char *tag, const char *fmt, ...);

#if defined(APP_DEBUG_MODE) || defined(CONFIG_APP_DEBUG_MODE)
#include <stdio.h>
    extern unsigned int global_events_counter_per_session;
    // Cross-platform atomic post-increment (returns old value).
    // Firmware target (GCC) uses the GCC built-in; MSVC host tests are
    // single-threaded so a plain post-increment is sufficient.
    #if defined(_MSC_VER)
        #define _EJ_COUNTER_INC(p) ((unsigned)(*(p))++)
    #else
        #define _EJ_COUNTER_INC(p) __atomic_fetch_add((p), 1u, __ATOMIC_RELAXED)
    #endif
    // Formats ">> EVENT N <<" into a local buffer and binds _ej_tag to it.
#define _EVENT_JOURNAL_TAG_DECL \
    char _ej_buf[32]; \
    snprintf(_ej_buf, sizeof(_ej_buf), ">> EVENT %u <<", \
             _EJ_COUNTER_INC(&global_events_counter_per_session) + 1u); \
    const char* const _ej_tag = _ej_buf;
#else
#define _EVENT_JOURNAL_TAG_DECL \
    const char* const _ej_tag = EVENT_JOURNAL_TAG;
#endif

// Add an event to the journal (macro implementation)
// This macro logs an event with the specified type, tag, and formatted message.
#define EVENT_JOURNAL_ADD(type, tag, fmt, ...) \
    do { \
        _event_journal_store(type, tag, fmt, ##__VA_ARGS__); \
        _EVENT_JOURNAL_TAG_DECL \
        switch (type) { \
            case EVENT_JOURNAL_INFO: \
                ESP_LOGI(_ej_tag, "%s: " fmt, tag, ##__VA_ARGS__); \
                break; \
            case EVENT_JOURNAL_WARNING: \
                ESP_LOGW(_ej_tag, "%s: " fmt, tag, ##__VA_ARGS__); \
                break; \
            case EVENT_JOURNAL_ERROR: \
                ESP_LOGE(_ej_tag, "%s: " fmt, tag, ##__VA_ARGS__); \
                break; \
            case EVENT_JOURNAL_ALERT: \
                ESP_LOGW(_ej_tag, "%s: " fmt, tag, ##__VA_ARGS__); \
                break; \
            default: \
                ESP_LOGW(_ej_tag, "Unknown event journal type: %d", type); \
                break; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif