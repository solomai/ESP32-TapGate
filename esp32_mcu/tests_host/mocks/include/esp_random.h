#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fill a buffer with random bytes from hardware RNG
 * 
 * Mock implementation for host testing using standard library random.
 */
void esp_fill_random(void *buf, size_t len);

/**
 * @brief Get random number from hardware RNG
 */
uint32_t esp_random(void);

#ifdef __cplusplus
}
#endif
