#include "event_journal.h"
#include <stdarg.h>

#if defined(APP_DEBUG_MODE) || defined(CONFIG_APP_DEBUG_MODE)
unsigned int global_events_counter_per_session = 0;
#endif

static const char* TAG = "EventJournal";

/**
 * @brief Internal function to handle persistent storage of journal events
 * 
 * @param type Event journal type
 * @param tag Log tag
 * @param fmt Format string
 * @param ... Variable arguments
 */
void _event_journal_store(enum event_journal_type type, const char *tag, const char *fmt, ...)
{
    // TODO: implement persistent storage of journal events
    // For now, this function is empty but ready for future implementation

    // Format the message
    va_list args;
    va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
 
    (void)type;
    (void)tag;    
    (void)buf;
     
    // TODO:
    ESP_LOG_NOTIMPLEMENTED(TAG);
}