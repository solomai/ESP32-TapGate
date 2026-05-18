#define APP_DEBUG_MODE

#include "unity.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Log capture
// ---------------------------------------------------------------------------

struct LogCapture {
    int  level;
    char tag[64];
    char message[256];
    bool called;
};

static LogCapture g_log{};

static void capture_log(int level, const char* tag, const char* fmt, ...) {
    g_log.level = level;
    snprintf(g_log.tag, sizeof(g_log.tag), "%s", tag);
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_log.message, sizeof(g_log.message), fmt, args);
    va_end(args);
    g_log.called = true;
}

#include "esp_log.h"
#undef ESP_LOGI
#undef ESP_LOGW
#undef ESP_LOGE
#define ESP_LOGI(tag, fmt, ...) capture_log(0, tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) capture_log(1, tag, fmt, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) capture_log(2, tag, fmt, ##__VA_ARGS__)

#include "event_journal.h"

// ---------------------------------------------------------------------------
// Store stub
// ---------------------------------------------------------------------------

extern "C" void _event_journal_store(enum event_journal_type, const char*, const char*, ...) {}

// Definition required because event_journal.c is not compiled in host tests.
unsigned int global_events_counter_per_session = 0;

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

extern "C" void setUp() {
    g_log = {};
    global_events_counter_per_session = 0;
}
extern "C" void tearDown() {}

static constexpr const char* TAG = "COMP";

// ---------------------------------------------------------------------------
// Counter starts at 1 on the first call
// ---------------------------------------------------------------------------

void EventJournal_Debug_FirstCall_TagContainsCounter1()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO, TAG, "msg");
    TEST_ASSERT_EQUAL_STRING(">> EVENT 1 <<", g_log.tag);
}

// ---------------------------------------------------------------------------
// Counter increments with each call
// ---------------------------------------------------------------------------

void EventJournal_Debug_SecondCall_TagContainsCounter2()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO, TAG, "first");
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO, TAG, "second");
    TEST_ASSERT_EQUAL_STRING(">> EVENT 2 <<", g_log.tag);
}

void EventJournal_Debug_MultipleTypes_CounterIncrementsEachCall()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,    TAG, "a");
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING, TAG, "b");
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR,   TAG, "c");
    TEST_ASSERT_EQUAL_STRING(">> EVENT 3 <<", g_log.tag);
}

// ---------------------------------------------------------------------------
// Counter resets across setUp() calls (each test starts at 0)
// ---------------------------------------------------------------------------

void EventJournal_Debug_AfterReset_CounterStartsAt1Again()
{
    // setUp() resets counter to 0 before this test
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR, TAG, "reset check");
    TEST_ASSERT_EQUAL_STRING(">> EVENT 1 <<", g_log.tag);
}

// ---------------------------------------------------------------------------
// Message content is unaffected by debug mode
// ---------------------------------------------------------------------------

void EventJournal_Debug_MessageStillContainsCallerTagPrefix()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO, TAG, "hello %d", 7);
    TEST_ASSERT_EQUAL_STRING("COMP: hello 7", g_log.message);
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main()
{
    UNITY_BEGIN();

    UnityDefaultTestRun(EventJournal_Debug_FirstCall_TagContainsCounter1,
                        "EventJournal_Debug_FirstCall_TagContainsCounter1", __FILE__);
    UnityDefaultTestRun(EventJournal_Debug_SecondCall_TagContainsCounter2,
                        "EventJournal_Debug_SecondCall_TagContainsCounter2", __FILE__);
    UnityDefaultTestRun(EventJournal_Debug_MultipleTypes_CounterIncrementsEachCall,
                        "EventJournal_Debug_MultipleTypes_CounterIncrementsEachCall", __FILE__);
    UnityDefaultTestRun(EventJournal_Debug_AfterReset_CounterStartsAt1Again,
                        "EventJournal_Debug_AfterReset_CounterStartsAt1Again", __FILE__);
    UnityDefaultTestRun(EventJournal_Debug_MessageStillContainsCallerTagPrefix,
                        "EventJournal_Debug_MessageStillContainsCallerTagPrefix", __FILE__);

    return UNITY_END();
}
