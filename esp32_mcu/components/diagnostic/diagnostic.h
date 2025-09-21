#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

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
            s_hwm_table[i].used   = 1;
            s_hwm_table[i].handle = h;
            s_hwm_table[i].last_bytes = bytes;
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

// Collect per-task stack info; honors SHORT_DIAGNOSTIC if defined
static void log_all_tasks_stack_to_buf(void)
{
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    if (task_count == 0) {
        diag_appendf(INDENT "no tasks\n");
        return;
    }

    uint32_t total_runtime;
    TaskStatus_t *task_array = (TaskStatus_t *)malloc(task_count * sizeof(TaskStatus_t));
    if (!task_array) {
        diag_appendf(INDENT "allocation failed for %u tasks\n", (unsigned)task_count);
        return;
    }

    task_count = uxTaskGetSystemState(task_array, task_count, &total_runtime);

    // Track if table overflow happens
    int table_overflow = 0;

    for (UBaseType_t i = 0; i < task_count; i++) {
        TaskHandle_t h   = task_array[i].xHandle;
        const char  *nm  = task_array[i].pcTaskName ? task_array[i].pcTaskName : "";
        UBaseType_t words = task_array[i].usStackHighWaterMark;
        unsigned bytes    = (unsigned)(words * sizeof(StackType_t));

        const char *status =
            (words < 256) ? "critical" :
            (words < 512) ? "low" : "ok";

        int idx = hwm_find(h);
        int is_new = (idx < 0);

#if defined(SHORT_DIAGNOSTIC)
        int should_print = 0;

        if (is_new) {
            // Always print new tasks
            should_print = 1;
            if (hwm_add(h, nm, bytes) < 0) table_overflow = 1;
        } else {
            // Print if bytes changed since last time
            if (s_hwm_table[idx].last_bytes != bytes) {
                should_print = 1;
                hwm_update(idx, bytes);
            }
        }

        // Always print critical tasks
        if (words < 256) {
            should_print = 1;
            if (!is_new) hwm_update(idx, bytes);
        }

        if (should_print) {
            // Task=<20 chars left-justified>, HWM=<6 digits right-aligned> bytes [status]
            diag_appendf(INDENT "Task=%-20.20s HWM=%6u bytes [%s]\n", nm, bytes, status);
        }
#else
        // Full mode: always print and update the table so next switch to short mode works immediately
        if (is_new) {
            if (hwm_add(h, nm, bytes) < 0) table_overflow = 1;
        } else {
            hwm_update(idx, bytes);
        }
        diag_appendf(INDENT "Task=%-20.20s HWM=%6u bytes [%s]\n", nm, bytes, status);
#endif
    }

    if (table_overflow) {
        diag_appendf(INDENT "[warning] HWM table overflow (>%d tasks); some tasks not tracked for delta output\n",
                     MAX_TRACKED_TASKS);
    }

    free(task_array);
}

// Periodic diagnostic task: single ESP_LOGI call per iteration
void diagnostic_task(void *pvParameter)
{
    const uint32_t timeout_on_start_ms = 3000;
    const uint32_t period_ms = 60000;

    vTaskDelay(pdMS_TO_TICKS(timeout_on_start_ms));

    for (;;) {
        diag_reset_buf();

        // Fixed header
        diag_appendf("device diagnostic information\n");

        // Summary lines
        diag_appendf(INDENT "Free heap: %u bytes\n", (unsigned)esp_get_free_heap_size());
        diag_appendf(INDENT "Stack usage (per task): HWM = HighWaterMark (lowest-ever free stack since task start; lower is worse)\n");

        // Per-task lines (filtered in SHORT_DIAGNOSTIC mode)
        log_all_tasks_stack_to_buf();

        // Single print
        ESP_LOGI(TAG, "%s", g_diag_buf);

        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}
