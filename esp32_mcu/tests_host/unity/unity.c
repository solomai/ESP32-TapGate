#include "unity.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

UnityContext Unity = {0};

extern void setUp(void);
extern void tearDown(void);

static void unity_print_test_header(const char *name)
{
    if (name)
    {
        printf("[ RUN      ] %s\n", name);
    }
}

static void unity_print_test_result(const char *name, bool failed)
{
    if (!name)
        return;

    if (failed)
    {
        printf("[  FAILED  ] %s\n", name);
    }
    else
    {
        printf("[       OK ] %s\n", name);
    }
}

void UnityBegin(const char *filename)
{
    Unity.total_tests = 0U;
    Unity.failed_tests = 0U;
    Unity.current_test_name = NULL;
    Unity.current_test_line = 0U;
    Unity.current_test_file = filename;
    Unity.current_test_failed = false;

    printf("==== Unity host test runner (%s) ====\n", filename ? filename : "<unknown>");
}

int UnityEnd(void)
{
    printf("==== Tests: %u, Failures: %u ====\n", Unity.total_tests, Unity.failed_tests);
    return (int)Unity.failed_tests;
}

void UnityMessage(const char *message, unsigned int line_number)
{
    if (message)
    {
        printf("%s:%u: %s\n", Unity.current_test_name ? Unity.current_test_name : "<test>", line_number, message);
    }
}

void UnityFail(const char *message, unsigned int line_number)
{
    Unity.current_test_failed = true;
    if (message)
    {
        fprintf(stderr, "%s:%u: %s\n", Unity.current_test_name ? Unity.current_test_name : "<test>", line_number, message);
    }
    else
    {
        fprintf(stderr, "%s:%u: Test failed\n", Unity.current_test_name ? Unity.current_test_name : "<test>", line_number);
    }
}

bool UnityAssert(bool condition, const char *message, unsigned int line_number)
{
    if (condition)
        return true;

    UnityFail(message, line_number);
    return false;
}

bool UnityAssertEqualInt(long expected, long actual, unsigned int line_number)
{
    if (expected == actual)
        return true;

    char buffer[128];
    (void)snprintf(buffer, sizeof(buffer), "Expected %ld but was %ld", expected, actual);
    UnityFail(buffer, line_number);
    return false;
}

bool UnityAssertEqualUInt32(uint32_t expected, uint32_t actual, unsigned int line_number)
{
    if (expected == actual)
        return true;

    char buffer[128];
    (void)snprintf(buffer, sizeof(buffer), "Expected %" PRIu32 " but was %" PRIu32, expected, actual);
    UnityFail(buffer, line_number);
    return false;
}

bool UnityAssertStringEqual(const char *expected, const char *actual, unsigned int line_number)
{
    if (expected == NULL && actual == NULL)
        return true;

    if (expected != NULL && actual != NULL && strcmp(expected, actual) == 0)
        return true;

    char buffer[256];
    (void)snprintf(buffer, sizeof(buffer), "Expected '%s' but was '%s'",
                   expected ? expected : "(null)",
                   actual ? actual : "(null)");
    UnityFail(buffer, line_number);
    return false;
}

bool UnityAssertLessOrEqualUInt32(uint32_t expected, uint32_t actual, unsigned int line_number)
{
    if (actual <= expected)
        return true;

    char buffer[128];
    (void)snprintf(buffer, sizeof(buffer), "Expected <= %" PRIu32 " but was %" PRIu32, expected, actual);
    UnityFail(buffer, line_number);
    return false;
}

void UnityDefaultTestRun(UnityTestFunction func, const char *name, int line_number)
{
    Unity.total_tests++;
    Unity.current_test_name = name;
    Unity.current_test_line = (unsigned int)line_number;
    Unity.current_test_failed = false;

    unity_print_test_header(name);

    setUp();
    func();
    tearDown();

    if (Unity.current_test_failed)
    {
        Unity.failed_tests++;
    }

    unity_print_test_result(name, Unity.current_test_failed);
}
