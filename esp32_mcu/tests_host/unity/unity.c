#include "unity.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

UnityContext Unity = {0};

static const char *unity_filter = NULL;
static bool unity_list_only = false;
static bool unity_filter_includes(const char *name)
{
    if (!unity_filter || !name)
    {
        return true;
    }

    return strstr(name, unity_filter) != NULL;
}

void UnityConfigureFromArgs(int argc, const char *argv[])
{
    unity_filter = NULL;
    unity_list_only = false;

    if (argc <= 1 || !argv)
        return;

    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (!arg)
            continue;

        if (strncmp(arg, "--filter=", 9) == 0)
        {
            unity_filter = arg + 9;
        }
        else if (strcmp(arg, "--filter") == 0)
        {
            if (i + 1 < argc)
            {
                unity_filter = argv[++i];
            }
            else
            {
                fprintf(stderr, "Unity: missing value for --filter option\n");
            }
        }
        else if (strcmp(arg, "--list") == 0)
        {
            unity_list_only = true;
        }
        else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
        {
            printf("Unity host test options:\n");
            printf("  --filter <substring>   Run only tests whose name contains the substring.\n");
            printf("  --list                 Print discovered test names without executing them.\n");
            printf("  --help                 Show this message.\n");
            unity_list_only = true;
        }
        else
        {
            fprintf(stderr, "Unity: ignoring unknown option '%s'\n", arg);
        }
    }
}

bool UnityIsListing(void)
{
    return unity_list_only;
}

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
    if (unity_list_only)
    {
        if (unity_filter_includes(name))
        {
            printf("%s\n", name ? name : "<unnamed>");
        }
        return;
    }

    if (!unity_filter_includes(name))
    {
        return;
    }

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
