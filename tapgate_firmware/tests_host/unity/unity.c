
// Minimal Unity test framework implementation for host-side testing
#include "unity.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#define COLOR_RED    "\033[91m"
#define COLOR_GREEN  "\033[92m"
#define COLOR_YELLOW "\033[93m"
#define COLOR_RESET  "\033[0m"


static size_t unity_total_tests = 0;
static size_t unity_failed_tests = 0;

void UNITY_BEGIN(void)
{
    unity_total_tests = 0;
    unity_failed_tests = 0;
    printf("%sTest suite start:%s\n", COLOR_YELLOW, COLOR_RESET);
}

int UNITY_END(void)
{
    printf("%sTest suite end:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("- Total tests:  %zd\n", unity_total_tests);
    if (unity_failed_tests) {
        printf("- Failed tests: %zd\n", unity_failed_tests);
        printf("- Passed tests: %zd\n", unity_total_tests - unity_failed_tests);
    }
    return unity_failed_tests == 0 ? 0 : 1;
}

static int unity_current_test_failed = 0;

void UnityAssertTrue(int condition, const char* msg, int line)
{
    if (!condition) {
        printf("\n\tAssertion failed: %s at line %d", msg, line);
        unity_current_test_failed = 1;
    }
}

void UnityAssertEqual(long long expected, long long actual, int line)
{
    if (expected != actual) {
        printf("\n\tAssertion failed at line %d: expected %lld but was %lld", line, expected, actual);
        unity_current_test_failed = 1;
    }
}

void UnityAssertEqualMemory(const void* expected, const void* actual, size_t size, int line)
{
    if (!expected || !actual || memcmp(expected, actual, size) != 0) {
        printf("\n\tAssertion failed at line %d: memory blocks differ (%zu bytes)", line, size);
        unity_current_test_failed = 1;
    }
}

void UnityAssertEqualString(const char* expected, const char* actual, int line)
{
    if (!expected || !actual || strcmp(expected, actual) != 0) {
        printf("\n\tAssertion failed at line %d: expected \"%s\" but was \"%s\"",
               line,
               expected ? expected : "(null)",
               actual   ? actual   : "(null)");
        unity_current_test_failed = 1;
    }
}

void UnityDefaultTestRun(void (*Func)(void), const char* FuncName, const char* FileName)
{
    // Extract just the filename from the full path
    const char* just_filename = FileName;
    for (const char* p = FileName; *p; ++p) {
        if (*p == '/' || *p == '\\') just_filename = p + 1;
    }
    // Print test name
    printf("- Running test: %s : %s", just_filename, FuncName );
    unity_total_tests++;
    unity_current_test_failed = 0;
    setUp();
    Func();
    tearDown();
    if (unity_current_test_failed) {
        printf(" - %sFAILED%s\n", COLOR_RED, COLOR_RESET);
        unity_failed_tests++;
    } else {
        printf(" - %sPASSED%s\n", COLOR_GREEN, COLOR_RESET);
    }
}
