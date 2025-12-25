#include "diagnostic.h"

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

namespace debug
{

static const char* TAG = "DIAG";
constexpr uint32_t ms_multiplier = 1000;
constexpr const char* INDENT = "\t\t";

/* ======================= CONFIG ======================= */

#if CONFIG_TAPGATE_DIAG_FULL
constexpr size_t LOG_BUFFER_SIZE = 4096;
#else
constexpr size_t LOG_BUFFER_SIZE = 2048;
#endif

/* ======================= LOG BUFFER ======================= */

static char   s_log_buf[LOG_BUFFER_SIZE];
static size_t s_log_len = 0;

static void buf_reset()
{
    s_log_len = 0;
    s_log_buf[0] = '\0';
}

static void buf_append(const char* fmt, ...)
{
    if (s_log_len >= LOG_BUFFER_SIZE - 1) return;

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(
        s_log_buf + s_log_len,
        LOG_BUFFER_SIZE - s_log_len,
        fmt,
        ap
    );
    va_end(ap);

    if (n > 0) {
        s_log_len += (size_t)n;
        if (s_log_len >= LOG_BUFFER_SIZE)
            s_log_len = LOG_BUFFER_SIZE - 1;
    }
}

/* ======================= HEAP STATE ======================= */

struct HeapSnapshot {
    uint32_t free_heap;
    uint32_t largest_block;
    uint32_t total_free;
    uint16_t free_blocks;
    bool     valid;
};

static HeapSnapshot s_prev_heap{};

/* ======================= TASK HWM ======================= */

constexpr size_t MAX_TASKS = 64;

struct TaskEntry {
    TaskHandle_t handle;
    uint16_t     hwm_words;
};

static TaskEntry s_tasks[MAX_TASKS];
static uint8_t   s_task_count = 0;

static int find_task(TaskHandle_t h)
{
    for (uint8_t i = 0; i < s_task_count; ++i)
        if (s_tasks[i].handle == h)
            return i;
    return -1;
}

/* =========== Used RAM by Diagnostic module ========== */
constexpr size_t DIAG_STATIC_RAM =
    sizeof(s_log_buf) +
    sizeof(s_tasks) +
    sizeof(s_prev_heap) +
    sizeof(s_task_count);

constexpr size_t DIAG_STACK_RAM =
    STACK_SIZE * sizeof(StackType_t);

constexpr size_t DIAG_RAM_USED =
    DIAG_STATIC_RAM /*+ DIAG_STACK_RAM*/;

/* ======================= CORE ======================= */

static void diagnostic_collect()
{
    buf_reset();
    buf_append("device diagnostic information\n");

    /* -------- Heap -------- */

    HeapSnapshot cur{};
    cur.free_heap = esp_get_free_heap_size();

    multi_heap_info_t info{};
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);

    cur.largest_block = info.largest_free_block;
    cur.total_free    = info.total_free_bytes;
    cur.free_blocks   = info.free_blocks;
    cur.valid         = true;

    if (s_prev_heap.valid) {
        int dh = (int)cur.free_heap - (int)s_prev_heap.free_heap;
        buf_append(
            "%sFree heap: %u bytes",
            INDENT,
            cur.free_heap
        );
        if (dh != 0)
            buf_append(" (%+d bytes)", dh);
        buf_append(" [~%u and stack %u used by diag]", (unsigned)DIAG_RAM_USED, STACK_SIZE);
        buf_append("\n");
    } else {
        buf_append("%sFree heap: %u bytes [~%u and stack %u used by diag]\n",
            INDENT,
            cur.free_heap,
            (unsigned)DIAG_RAM_USED, STACK_SIZE);
    }

    buf_append(
        "%sLargest free block: %u, Free blocks: %u\n",
        INDENT,
        cur.largest_block,
        cur.free_blocks
    );

    const float frag_now =
        1.0f - (float)cur.largest_block / (float)cur.total_free;

    if (s_prev_heap.valid) {
        const float frag_prev =
            1.0f - (float)s_prev_heap.largest_block /
                   (float)s_prev_heap.total_free;
        const float df = frag_now - frag_prev;

        if (df != 0.0f)
            buf_append(
                "%sMemory fragmentation: %f (%+f)\n",
                INDENT,
                frag_now,
                df
            );
        else
            buf_append(
                "%sMemory fragmentation: %f\n",
                INDENT,
                frag_now
            );
    } else {
        buf_append(
            "%sMemory fragmentation: %f\n",
            INDENT,
            frag_now
        );
    }

    s_prev_heap = cur;

#if CONFIG_TAPGATE_DIAG_FULL
    /* -------- Tasks (FULL) -------- */

    const UBaseType_t task_count = uxTaskGetNumberOfTasks();
    if (task_count == 0) return;

    TaskStatus_t* tasks =
        static_cast<TaskStatus_t*>(malloc(task_count * sizeof(TaskStatus_t)));
    if (!tasks) return;

    uint32_t total_runtime = 0;
    const UBaseType_t real_count =
        uxTaskGetSystemState(tasks, task_count, &total_runtime);

    buf_append(
        "%sStack usage (per task): HWM = HighWaterMark "
        "(lowest-ever free stack since task start; lower is worse)\n",
        INDENT
    );

    for (UBaseType_t i = 0; i < real_count; ++i) {
        const TaskHandle_t h = tasks[i].xHandle;
        const char* name = tasks[i].pcTaskName ? tasks[i].pcTaskName : "";

        const uint16_t words = tasks[i].usStackHighWaterMark;
        const uint32_t bytes = words * sizeof(StackType_t);

        const char* status =
            (words < 256) ? "critical" :
            (words < 512) ? "low" : "ok";

        int idx = find_task(h);
        int delta = 0;

        if (idx >= 0) {
            delta = (int)words - (int)s_tasks[idx].hwm_words;
            s_tasks[idx].hwm_words = words;
        } else if (s_task_count < MAX_TASKS) {
            s_tasks[s_task_count++] = { h, words };
        }

        buf_append(
            "%sTask=%-20.20s HWM=%6u bytes [%s]",
            INDENT,
            name,
            bytes,
            status
        );

        if (idx >= 0 && delta != 0)
            buf_append(" (%+d bytes)", delta * (int)sizeof(StackType_t));

        buf_append("\n");
    }

    free(tasks);
#endif
}

void Diagnostic::run()
{
    ESP_LOGI(TAG, "Diagnostic thread started");
    
    #ifdef CONFIG_TAPGATE_DIAG_INITIAL_PAUSE
    const uint32_t initial_pause = CONFIG_TAPGATE_DIAG_INITIAL_PAUSE * ms_multiplier;
    #else
    const uint32_t initial_pause = 0;
    #endif
    vTaskDelay(pdMS_TO_TICKS(
        initial_pause));

    #ifdef CONFIG_TAPGATE_DIAG_INTERVAL
    const uint32_t sleep_timeout = CONFIG_TAPGATE_DIAG_INTERVAL * ms_multiplier;
    #else
    const uint32_t sleep_timeout = 100 * ms_multiplier;
    #endif        

    while (true) {
        diagnostic_collect();
        ESP_LOGI(TAG, "%s", s_log_buf);
        vTaskDelay(pdMS_TO_TICKS(sleep_timeout));
    }
}

} // namespace debug