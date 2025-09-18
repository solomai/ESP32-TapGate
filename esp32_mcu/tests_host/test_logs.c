#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "unity.h"
#include "logs/logs.h"

#include <stdio.h>

#if defined(_WIN32)
#include <io.h>
#define tg_dup   _dup
#define tg_dup2  _dup2
#define tg_fileno _fileno
#define tg_close _close
#else
#include <unistd.h>
#define tg_dup   dup
#define tg_dup2  dup2
#define tg_fileno fileno
#define tg_close close
#endif

typedef void (*log_invoker_t)(void *context);

static size_t capture_stream_output(FILE *stream, char *buffer, size_t buffer_size,
                                    log_invoker_t invoker, void *context)
{
    if (!stream || !buffer || buffer_size == 0)
        return 0;

    fflush(stream);

    int original_fd = tg_dup(tg_fileno(stream));
    if (original_fd < 0)
        return 0;

    FILE *temp = tmpfile();
    if (!temp)
    {
        tg_close(original_fd);
        return 0;
    }

    int temp_fd = tg_fileno(temp);
    if (temp_fd < 0)
    {
        fclose(temp);
        tg_close(original_fd);
        return 0;
    }

    if (tg_dup2(temp_fd, tg_fileno(stream)) < 0)
    {
        fclose(temp);
        tg_close(original_fd);
        return 0;
    }

    if (invoker)
        invoker(context);

    fflush(stream);
    fflush(temp);

    rewind(temp);
    size_t read = fread(buffer, 1, buffer_size - 1, temp);
    buffer[read] = '\0';

    if (tg_dup2(original_fd, tg_fileno(stream)) < 0)
        read = 0;

    tg_close(original_fd);
    fclose(temp);

    return read;
}

static void invoke_LOGD(void *context)
{
    (void)context;
    LOGD("debug-tag", "value=%d text=%s", 7, "alpha");
}

static void invoke_LOGI(void *context)
{
    (void)context;
    LOGI("info-tag", "value=%d text=%s", 42, "beta");
}

static void invoke_LOGW(void *context)
{
    (void)context;
    LOGW("warn-tag", "value=%d text=%s", -1, "gamma");
}

static void invoke_LOGE(void *context)
{
    (void)context;
    LOGE("error-tag", "value=%d text=%s", 5, "delta");
}

void setUp(void) {}
void tearDown(void) {}

void test_LOGD_formats_message(void)
{
    char output[128];
    size_t length = capture_stream_output(stdout, output, sizeof(output), invoke_LOGD, NULL);
    TEST_ASSERT_TRUE(length > 0);
    TEST_ASSERT_EQUAL_STRING("[D] debug-tag: value=7 text=alpha\n", output);
}

void test_LOGI_formats_message(void)
{
    char output[128];
    size_t length = capture_stream_output(stdout, output, sizeof(output), invoke_LOGI, NULL);
    TEST_ASSERT_TRUE(length > 0);
    TEST_ASSERT_EQUAL_STRING("[I] info-tag: value=42 text=beta\n", output);
}

void test_LOGW_formats_message(void)
{
    char output[128];
    size_t length = capture_stream_output(stderr, output, sizeof(output), invoke_LOGW, NULL);
    TEST_ASSERT_TRUE(length > 0);
    TEST_ASSERT_EQUAL_STRING("[W] warn-tag: value=-1 text=gamma\n", output);
}

void test_LOGE_formats_message(void)
{
    char output[128];
    size_t length = capture_stream_output(stderr, output, sizeof(output), invoke_LOGE, NULL);
    TEST_ASSERT_TRUE(length > 0);
    TEST_ASSERT_EQUAL_STRING("[E] error-tag: value=5 text=delta\n", output);
}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    RUN_TEST(test_LOGD_formats_message);
    RUN_TEST(test_LOGI_formats_message);
    RUN_TEST(test_LOGW_formats_message);
    RUN_TEST(test_LOGE_formats_message);
    return UNITY_END();
}

