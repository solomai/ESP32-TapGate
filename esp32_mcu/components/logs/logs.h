#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void LOGD(const char *tag, const char *fmt, ...);
void LOGI(const char *tag, const char *fmt, ...);
void LOGW(const char *tag, const char *fmt, ...);
void LOGE(const char *tag, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

