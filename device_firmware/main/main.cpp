#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "constants.h"
#include "common/event_journal/event_journal.h"
#include "contexts/ctx_device.h"

#include "channels/channel_router.h"

#ifdef CONFIG_TAPGATE_DEBUG_MODE
    #include "common/diagnostic/diagnostic.h"
#endif

static const char *TAG_MAIN = "MAIN";

extern "C" void app_main(void)
{
#ifdef CONFIG_TAPGATE_DEBUG_MODE
    ESP_LOGI(TAG_MAIN, "Available Heap %d bytes", esp_get_free_heap_size() );
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING,
        TAG_MAIN,
        "Debug Mode enabled");
    debug::Diagnostic::start();
#endif

    // Get singleton instance (automatically initializes on first call)
    auto& ctxDevice = CtxDevice::getInstance();

    auto& channelRouter = channels::ChannelRouter::getInstance();

#ifdef CONFIG_TAPGATE_DEBUG_MODE
    ESP_LOGI(TAG_MAIN, "Channel count: %zu", channelRouter.getChannelCount());
    for (size_t i = 0; i < channelRouter.getChannelCount(); ++i) {
        auto& channel = channelRouter.get(static_cast<channels::ChannelType>(i));
        ESP_LOGI(TAG_MAIN, "%zu - Channel %s Status: %s", i+1,
            channels::toString(channel.GetType()).data(),
            channels::toString(channel.GetStatus()).data());
    }
#endif

    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
        TAG_MAIN,
        "TapGate firmware startup complete");

    while (true) {

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

