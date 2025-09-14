#ifndef SIMPLE_UNITY_H
#define SIMPLE_UNITY_H

#include <stdio.h>
#include <stdint.h>

static unsigned int UnityTestsPassed = 0;
static unsigned int UnityTestsFailed = 0;

static inline void UnityBegin(void) {
    UnityTestsPassed = 0;
    UnityTestsFailed = 0;
}

static inline int UnityEnd(void) {
    printf("%u Tests %u Failures\n", UnityTestsPassed + UnityTestsFailed, UnityTestsFailed);
    return UnityTestsFailed;
}

#define UNITY_BEGIN() (UnityBegin(), 0)
#define UNITY_END() UnityEnd()

#define RUN_TEST(func) do { \
    printf("RUNNING %s\n", #func); \
    func(); \
} while (0)

#define TEST_ASSERT_LESS_OR_EQUAL_UINT32(expected, actual) do { \
    if ((uint32_t)(actual) <= (uint32_t)(expected)) { \
        UnityTestsPassed++; \
    } else { \
        UnityTestsFailed++; \
        printf("Assertion failed: expected <= %u but was %u\n", (unsigned)(expected), (unsigned)(actual)); \
    } \
} while (0)

#endif // SIMPLE_UNITY_H
