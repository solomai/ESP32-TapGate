#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*UnityTestFunction)(void);

typedef struct UnityContext {
    unsigned int total_tests;
    unsigned int failed_tests;
    const char *current_test_name;
    unsigned int current_test_line;
    const char *current_test_file;
    bool current_test_failed;
} UnityContext;

extern UnityContext Unity;

void UnityBegin(const char *filename);
int UnityEnd(void);
void UnityDefaultTestRun(UnityTestFunction func, const char *name, int line_number);
void UnityFail(const char *message, unsigned int line_number);
bool UnityAssert(bool condition, const char *message, unsigned int line_number);
bool UnityAssertEqualInt(long expected, long actual, unsigned int line_number);
bool UnityAssertEqualUInt32(uint32_t expected, uint32_t actual, unsigned int line_number);
bool UnityAssertStringEqual(const char *expected, const char *actual, unsigned int line_number);
bool UnityAssertLessOrEqualUInt32(uint32_t expected, uint32_t actual, unsigned int line_number);
void UnityMessage(const char *message, unsigned int line_number);

#define UNITY_BEGIN() (UnityBegin(__FILE__))
#define UNITY_END() (UnityEnd())
#define RUN_TEST(func) (UnityDefaultTestRun((func), #func, __LINE__))

#define UNITY_TEST_ASSERT(condition, message, line)                             \
    do {                                                                         \
        if (!(UnityAssert((condition), (message), (line)))) {                    \
            return;                                                             \
        }                                                                        \
    } while (0)

#define UNITY_TEST_FAIL(message, line)                                          \
    do {                                                                         \
        UnityFail((message), (line));                                           \
        return;                                                                 \
    } while (0)

#define TEST_ASSERT(condition)                                                  \
    UNITY_TEST_ASSERT((condition), "Condition was false", __LINE__)

#define TEST_ASSERT_TRUE(condition)                                             \
    UNITY_TEST_ASSERT((condition), "Expected true", __LINE__)

#define TEST_ASSERT_FALSE(condition)                                            \
    UNITY_TEST_ASSERT(!(condition), "Expected false", __LINE__)

#define TEST_ASSERT_NULL(pointer)                                               \
    UNITY_TEST_ASSERT((pointer) == NULL, "Expected NULL", __LINE__)

#define TEST_ASSERT_NOT_NULL(pointer)                                           \
    UNITY_TEST_ASSERT((pointer) != NULL, "Expected non-NULL", __LINE__)

#define TEST_ASSERT_EQUAL_INT(expected, actual)                                 \
    do {                                                                         \
        if (!UnityAssertEqualInt((expected), (actual), __LINE__)) {             \
            return;                                                             \
        }                                                                        \
    } while (0)

#define TEST_ASSERT_EQUAL_UINT32(expected, actual)                              \
    do {                                                                         \
        if (!UnityAssertEqualUInt32((expected), (actual), __LINE__)) {          \
            return;                                                             \
        }                                                                        \
    } while (0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual)                              \
    do {                                                                         \
        if (!UnityAssertStringEqual((expected), (actual), __LINE__)) {          \
            return;                                                             \
        }                                                                        \
    } while (0)

#define TEST_ASSERT_LESS_OR_EQUAL_UINT32(expected, actual)                      \
    do {                                                                         \
        if (!UnityAssertLessOrEqualUInt32((expected), (actual), __LINE__)) {    \
            return;                                                             \
        }                                                                        \
    } while (0)

#define TEST_FAIL_MESSAGE(message)                                              \
    UNITY_TEST_FAIL((message), __LINE__)

#define TEST_IGNORE()                                                           \
    return

#define TEST_MESSAGE(message)                                                   \
    UnityMessage((message), __LINE__)

#ifdef __cplusplus
}
#endif

