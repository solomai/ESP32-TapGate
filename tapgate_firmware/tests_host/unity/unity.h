// Minimal Unity test framework header for host-side testing
#ifndef UNITY_H
#define UNITY_H

#ifdef __cplusplus
extern "C" {
#endif


void UNITY_BEGIN(void);
int UNITY_END(void);
void setUp(void);
void tearDown(void);
void UnityDefaultTestRun(void (*Func)(void), const char* FuncName, const char* FileName);
void UnityAssertTrue(int condition, const char* msg, int line);
#define TEST_ASSERT_TRUE(x) UnityAssertTrue((x), #x, __LINE__)
#define TEST_CASE(name, ...) static void name(void)

#ifdef __cplusplus
}
#endif

#endif // UNITY_H
