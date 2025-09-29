#pragma once

#include <stdio.h>

// Mock logs header for testing

#define LOGI(tag, format, ...) printf("[INFO] %s: " format "\n", tag, ##__VA_ARGS__)
#define LOGW(tag, format, ...) printf("[WARN] %s: " format "\n", tag, ##__VA_ARGS__)
#define LOGE(tag, format, ...) printf("[ERROR] %s: " format "\n", tag, ##__VA_ARGS__)
#define LOGD(tag, format, ...) printf("[DEBUG] %s: " format "\n", tag, ##__VA_ARGS__)
#define LOGV(tag, format, ...) printf("[VERBOSE] %s: " format "\n", tag, ##__VA_ARGS__)