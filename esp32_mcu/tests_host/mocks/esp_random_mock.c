#include "esp_random.h"
#include <stdlib.h>
#include <time.h>

static int random_initialized = 0;

static void ensure_random_init(void)
{
    if (!random_initialized) {
        srand((unsigned int)time(NULL));
        random_initialized = 1;
    }
}

void esp_fill_random(void *buf, size_t len)
{
    ensure_random_init();
    uint8_t *p = (uint8_t *)buf;
    for (size_t i = 0; i < len; i++) {
        p[i] = (uint8_t)(rand() & 0xFF);
    }
}

uint32_t esp_random(void)
{
    ensure_random_init();
    return (uint32_t)rand();
}
