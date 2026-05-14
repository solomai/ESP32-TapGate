#pragma once

#include "unity.h"
#include "esp_log.h"

// Convenience macro: log and assert
#define TEST_ASSERT_LOG(condition, msg) \
    do { \
        if (!(condition)) { ESP_LOGE("TEST", "%s", msg); } \
        TEST_ASSERT_MESSAGE(condition, msg); \
    } while(0)