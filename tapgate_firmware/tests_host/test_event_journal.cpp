#include "unity.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Log capture
// ---------------------------------------------------------------------------

struct LogCapture {
    int  level;         // 0=INFO  1=WARN  2=ERROR
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

// Include mock first to satisfy #pragma once, then override with captures.
// EVENT_JOURNAL_ADD is a macro: its ESP_LOGx calls expand at the call site,
// so whatever definitions are active here will be used in the tests below.
#include "esp_log.h"
#undef ESP_LOGI
#undef ESP_LOGW
#undef ESP_LOGE
#define ESP_LOGI(tag, fmt, ...) capture_log(0, tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) capture_log(1, tag, fmt, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) capture_log(2, tag, fmt, ##__VA_ARGS__)

// event_journal.h defines event_journal_type; must come before StoreCapture.
#include "event_journal.h"

// ---------------------------------------------------------------------------
// Store capture
// ---------------------------------------------------------------------------

struct StoreCapture {
    int                call_count;
    char               tag[64];
    char               message[256];
    event_journal_type type;
};

static StoreCapture g_store{};

extern "C" void _event_journal_store(enum event_journal_type type,
                                     const char* tag, const char* fmt, ...) {
    g_store.type = type;
    snprintf(g_store.tag, sizeof(g_store.tag), "%s", tag);
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_store.message, sizeof(g_store.message), fmt, args);
    va_end(args);
    ++g_store.call_count;
}

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

extern "C" void setUp()    { g_log = {}; g_store = {}; }
extern "C" void tearDown() {}

static constexpr const char* CALLER_TAG = "MYTAG";

// ---------------------------------------------------------------------------
// Log tag — must always be "EVENT", not the caller tag
// ---------------------------------------------------------------------------

void EventJournal_Add_InfoType_LogTagIsEvent()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO, CALLER_TAG, "hello");
    TEST_ASSERT_EQUAL_STRING(EVENT_JOURNAL_TAG, g_log.tag);
}

void EventJournal_Add_WarningType_LogTagIsEvent()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING, CALLER_TAG, "hello");
    TEST_ASSERT_EQUAL_STRING(EVENT_JOURNAL_TAG, g_log.tag);
}

void EventJournal_Add_ErrorType_LogTagIsEvent()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR, CALLER_TAG, "hello");
    TEST_ASSERT_EQUAL_STRING(EVENT_JOURNAL_TAG, g_log.tag);
}

// ---------------------------------------------------------------------------
// Message format — caller tag must be prepended as "<TAG>: <message>"
// ---------------------------------------------------------------------------

void EventJournal_Add_PlainMessage_CallerTagPrepended()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO, CALLER_TAG, "hello world");
    TEST_ASSERT_EQUAL_STRING("MYTAG: hello world", g_log.message);
}

void EventJournal_Add_FormatArgs_ExpandedWithCallerTagPrefix()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO, CALLER_TAG, "val=%d", 42);
    TEST_ASSERT_EQUAL_STRING("MYTAG: val=42", g_log.message);
}

void EventJournal_Add_MultipleFormatArgs_AllExpanded()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR, CALLER_TAG, "%s (0x%x)", "ERR", 0x102);
    TEST_ASSERT_EQUAL_STRING("MYTAG: ERR (0x102)", g_log.message);
}

// ---------------------------------------------------------------------------
// Log level routing — INFO→LOGI  WARNING→LOGW  ERROR→LOGE  ALERT→LOGW
// ---------------------------------------------------------------------------

void EventJournal_Add_InfoType_UsesLogi()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO, CALLER_TAG, "msg");
    TEST_ASSERT_EQUAL(0, g_log.level);
}

void EventJournal_Add_WarningType_UsesLogw()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING, CALLER_TAG, "msg");
    TEST_ASSERT_EQUAL(1, g_log.level);
}

void EventJournal_Add_ErrorType_UsesLoge()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR, CALLER_TAG, "msg");
    TEST_ASSERT_EQUAL(2, g_log.level);
}

void EventJournal_Add_AlertType_UsesLogw()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_ALERT, CALLER_TAG, "msg");
    TEST_ASSERT_EQUAL(1, g_log.level);
}

// ---------------------------------------------------------------------------
// Persistent store — must receive original caller tag and raw format string
// ---------------------------------------------------------------------------

void EventJournal_Store_CalledOnce()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO, CALLER_TAG, "msg");
    TEST_ASSERT_EQUAL(1, g_store.call_count);
}

void EventJournal_Store_ReceivesOriginalTag()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR, CALLER_TAG, "persist this");
    TEST_ASSERT_EQUAL_STRING(CALLER_TAG, g_store.tag);
}

void EventJournal_Store_ReceivesCorrectType()
{
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING, CALLER_TAG, "msg");
    TEST_ASSERT_EQUAL(EVENT_JOURNAL_WARNING, g_store.type);
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main()
{
    UNITY_BEGIN();

    UnityDefaultTestRun(EventJournal_Add_InfoType_LogTagIsEvent,
                        "EventJournal_Add_InfoType_LogTagIsEvent", __FILE__);
    UnityDefaultTestRun(EventJournal_Add_WarningType_LogTagIsEvent,
                        "EventJournal_Add_WarningType_LogTagIsEvent", __FILE__);
    UnityDefaultTestRun(EventJournal_Add_ErrorType_LogTagIsEvent,
                        "EventJournal_Add_ErrorType_LogTagIsEvent", __FILE__);

    UnityDefaultTestRun(EventJournal_Add_PlainMessage_CallerTagPrepended,
                        "EventJournal_Add_PlainMessage_CallerTagPrepended", __FILE__);
    UnityDefaultTestRun(EventJournal_Add_FormatArgs_ExpandedWithCallerTagPrefix,
                        "EventJournal_Add_FormatArgs_ExpandedWithCallerTagPrefix", __FILE__);
    UnityDefaultTestRun(EventJournal_Add_MultipleFormatArgs_AllExpanded,
                        "EventJournal_Add_MultipleFormatArgs_AllExpanded", __FILE__);

    UnityDefaultTestRun(EventJournal_Add_InfoType_UsesLogi,
                        "EventJournal_Add_InfoType_UsesLogi", __FILE__);
    UnityDefaultTestRun(EventJournal_Add_WarningType_UsesLogw,
                        "EventJournal_Add_WarningType_UsesLogw", __FILE__);
    UnityDefaultTestRun(EventJournal_Add_ErrorType_UsesLoge,
                        "EventJournal_Add_ErrorType_UsesLoge", __FILE__);
    UnityDefaultTestRun(EventJournal_Add_AlertType_UsesLogw,
                        "EventJournal_Add_AlertType_UsesLogw", __FILE__);

    UnityDefaultTestRun(EventJournal_Store_CalledOnce,
                        "EventJournal_Store_CalledOnce", __FILE__);
    UnityDefaultTestRun(EventJournal_Store_ReceivesOriginalTag,
                        "EventJournal_Store_ReceivesOriginalTag", __FILE__);
    UnityDefaultTestRun(EventJournal_Store_ReceivesCorrectType,
                        "EventJournal_Store_ReceivesCorrectType", __FILE__);

    return UNITY_END();
}
