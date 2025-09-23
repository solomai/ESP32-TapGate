// Flag CONFIG_FREERTOS_USE_TRACE_FACILITY=y expected

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#ifndef SHORT_DIAGNOSTIC
#define SHORT_DIAGNOSTIC
#endif

static const char TAG[] = "DIAG";

#define MAX_BUFFER_SIZE       8192
#define INDENT                "\t\t"   // 2 tabs for readability
#define MAX_TRACKED_TASKS     64       // Adjust if you have many tasks
#define TASK_NAME_STORE_LEN   24       // Store up to 24 chars (enough for display)

static char   g_diag_buf[MAX_BUFFER_SIZE];
static size_t g_diag_len = 0;

// Keep previous free-heap to print deltas between diagnostic iterations
static bool     s_prev_heap_valid = false;
static uint32_t s_prev_heap_bytes = 0;

// Per-task HWM tracking table (stores last printed bytes for change detection)
typedef struct {
    TaskHandle_t handle;
    unsigned     last_bytes;
    char         name[TASK_NAME_STORE_LEN];
    uint8_t      used;
} hwm_entry_t;

static hwm_entry_t s_hwm_table[MAX_TRACKED_TASKS];

// Reset buffer before composing the next log batch
static inline void diag_reset_buf(void) {
    g_diag_len = 0;
    g_diag_buf[0] = '\0';
}

// Append formatted text into the buffer (truncates on overflow)
static inline void diag_appendf(const char *fmt, ...) {
    if (g_diag_len >= (MAX_BUFFER_SIZE - 1)) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(g_diag_buf + g_diag_len, MAX_BUFFER_SIZE - g_diag_len, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n >= (MAX_BUFFER_SIZE - g_diag_len)) {
        g_diag_len = MAX_BUFFER_SIZE - 1; // truncated, keep valid NUL
    } else {
        g_diag_len += (size_t)n;
    }
}

// Find an entry by task handle
static int hwm_find(TaskHandle_t h) {
    for (int i = 0; i < MAX_TRACKED_TASKS; ++i) {
        if (s_hwm_table[i].used && s_hwm_table[i].handle == h) return i;
    }
    return -1;
}

// Add or replace an entry (returns index or -1 on failure)
static int hwm_add(TaskHandle_t h, const char *name, unsigned bytes) {
    for (int i = 0; i < MAX_TRACKED_TASKS; ++i) {
        if (!s_hwm_table[i].used) {
            s_hwm_table[i].used        = 1;
            s_hwm_table[i].handle      = h;
            s_hwm_table[i].last_bytes  = bytes;
            // Store truncated, always NUL-terminated
            strncpy(s_hwm_table[i].name, name ? name : "", TASK_NAME_STORE_LEN - 1);
            s_hwm_table[i].name[TASK_NAME_STORE_LEN - 1] = '\0';
            return i;
        }
    }
    return -1;
}

// Update an existing entry
static inline void hwm_update(int idx, unsigned bytes) {
    s_hwm_table[idx].last_bytes = bytes;
}

/*
 * Collect per-task stack info, print filtered list.
 * Behavior:
 *  - Prints the HWM header lazily (only if at least one task line is going to be printed).
 *  - Appends "no changes" line if no deltas among EXISTING tasks were detected.
 *  - New tasks don't count as "existing deltas".
 * Returns whether any EXISTING task changed (delta != 0).
 */
static bool log_all_tasks_stack_to_buf(void)
{
    bool any_existing_delta  = false;
    bool printed_any_task_ln = false;   // track whether we emitted any task line
    bool printed_header      = false;   // lazy header

    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    if (task_count == 0) {
        diag_appendf(INDENT "no tasks\n");
        return false;
    }

    uint32_t total_runtime = 0;
    TaskStatus_t *task_array = (TaskStatus_t *)malloc(task_count * sizeof(TaskStatus_t));
    if (!task_array) {
        diag_appendf(INDENT "allocation failed for %u tasks\n", (unsigned)task_count);
        return false;
    }

    task_count = uxTaskGetSystemState(task_array, task_count, &total_runtime);

    // Track if table overflow happens
    bool table_overflow = false;

    for (UBaseType_t i = 0; i < task_count; i++) {
        TaskHandle_t h    = task_array[i].xHandle;
        const char  *nm   = task_array[i].pcTaskName ? task_array[i].pcTaskName : "";
        UBaseType_t words = task_array[i].usStackHighWaterMark;
        unsigned    bytes = (unsigned)(words * sizeof(StackType_t));

        const char *status =
            (words < 256) ? "critical" :
            (words < 512) ? "low" : "ok";

        int  idx    = hwm_find(h);
        bool is_new = (idx < 0);
        int  delta  = 0;

        if (!is_new) {
            delta = (int)bytes - (int)s_hwm_table[idx].last_bytes;
            if (delta != 0) {
                any_existing_delta = true; // only mark when an existing task changed
            }
        }

#if defined(SHORT_DIAGNOSTIC)
        bool should_print = false;

        if (is_new) {
            // Always print new tasks (but they don't count toward delta-change condition)
            should_print = true;
            if (hwm_add(h, nm, bytes) < 0) table_overflow = true;
        } else {
            // Print if bytes changed since last time
            if (delta != 0) {
                should_print = true;
                hwm_update(idx, bytes);
            }
        }

        // Always print critical tasks (even if no delta)
        if (words < 256) {
            should_print = true;
            if (!is_new) hwm_update(idx, bytes);
        }

        if (should_print) {
            if (!printed_header) {
                diag_appendf(INDENT "Stack usage (per task): HWM = HighWaterMark (lowest-ever free stack since task start; lower is worse)\n");
                printed_header = true;
            }
            // Task=<20 chars left-justified>, HWM=<6 digits right-aligned> bytes [status]
            // Add delta only for existing tasks and only when delta != 0
            if (!is_new && delta != 0) {
                diag_appendf(INDENT "Task=%-20.20s HWM=%6u bytes (%+d bytes) [%s]\n",
                             nm, bytes, delta, status);
            } else {
                diag_appendf(INDENT "Task=%-20.20s HWM=%6u bytes [%s]\n",
                             nm, bytes, status);
            }
            printed_any_task_ln = true;
        }
#else
        // Full mode: always print and update the table so next switch to short mode works immediately
        if (is_new) {
            if (hwm_add(h, nm, bytes) < 0) table_overflow = true;
        } else {
            if (delta != 0) hwm_update(idx, bytes);
        }

        if (!printed_header) {
            diag_appendf(INDENT "Stack usage (per task): HWM = HighWaterMark (lowest-ever free stack since task start; lower is worse)\n");
            printed_header = true;
        }

        if (!is_new && delta != 0) {
            diag_appendf(INDENT "Task=%-20.20s HWM=%6u bytes (%+d bytes) [%s]\n",
                         nm, bytes, delta, status);
        } else {
            diag_appendf(INDENT "Task=%-20.20s HWM=%6u bytes [%s]\n",
                         nm, bytes, status);	
        }
        printed_any_task_ln = true;
#endif
    }

    if (table_overflow) {
        // Keep the warning visible regardless of header presence
        diag_appendf(INDENT "[warning] HWM table overflow (>%d tasks); some tasks not tracked for delta output\n",
                     MAX_TRACKED_TASKS);
    }

    free(task_array);

    // If there were NO deltas among existing tasks, print the requested message.
    // Thanks to lazy header, if we didn't print any task line, there will be no header either.
    if (!any_existing_delta && !printed_header) {
        diag_appendf(INDENT "Stack usage has not changed since the last diagnostic\n");
    }

    return any_existing_delta;
}

// Periodic diagnostic task: single ESP_LOGI call per iteration
void diagnostic_task(void *pvParameter)
{
    const uint32_t timeout_on_start_ms = 3000;
    const uint32_t period_ms = 5 * 60000; // refresh status in minutes

    vTaskDelay(pdMS_TO_TICKS(timeout_on_start_ms));

    for (;;) {
        diag_reset_buf();

        // Fixed header
        diag_appendf("device diagnostic information\n");

        // Free heap with optional delta
        uint32_t cur_heap = esp_get_free_heap_size();
        if (s_prev_heap_valid) {
            int32_t dh = (int32_t)cur_heap - (int32_t)s_prev_heap_bytes;
            if (dh != 0) {
                diag_appendf(INDENT "Free heap: %u bytes (%+d bytes)\n",
                             (unsigned)cur_heap, (int)dh);
            } else {
                // No change -> do not print delta at all
                diag_appendf(INDENT "Free heap: %u bytes\n", (unsigned)cur_heap);
            }
        } else {
            // First run -> no delta available
            diag_appendf(INDENT "Free heap: %u bytes\n", (unsigned)cur_heap);
            s_prev_heap_valid = true;
        }
        s_prev_heap_bytes = cur_heap;

        // NOTE: We no longer print the HWM header here.
        // The HWM header is printed lazily inside log_all_tasks_stack_to_buf()
        // only if at least one per-task line is actually emitted.
        (void)log_all_tasks_stack_to_buf();

        // Single print
        ESP_LOGI(TAG, "%s", g_diag_buf);

        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}
