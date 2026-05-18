#pragma once

#ifdef TAPGATE_TEST_SILENT_LOG
    #define ESP_LOGI(tag, fmt, ...) ((void)0)
    #define ESP_LOGW(tag, fmt, ...) ((void)0)
    #define ESP_LOGE(tag, fmt, ...) ((void)0)
#else
    #include <cstdio>
    #define ESP_LOGI(tag, fmt, ...) std::printf("[I] " fmt "\n", ##__VA_ARGS__)
    #define ESP_LOGW(tag, fmt, ...) std::printf("[W] " fmt "\n", ##__VA_ARGS__)
    #define ESP_LOGE(tag, fmt, ...) std::printf("[E] " fmt "\n", ##__VA_ARGS__)
#endif
