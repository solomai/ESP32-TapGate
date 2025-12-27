#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "constants.h"
#include "common/event_journal/event_journal.h"
#include "contexts/ctx_device.h"
#include "common/nvm/nvm.h"

#include "channels/channel_router.h"

#ifdef CONFIG_TAPGATE_DEBUG_MODE
    #include "common/diagnostic/diagnostic.h"
#endif

static const char *TAG_MAIN = "MAIN";
#ifdef CONFIG_TAPGATE_DEBUG_MODE
    static const char* TAG_DEBUG = "DEBUG";
#endif

// Application entry point
extern "C" void app_main(void)
{
    // Initialize Diagnostic module if in debug mode
#ifdef CONFIG_TAPGATE_DEBUG_MODE
    ESP_LOGI(TAG_DEBUG, "Available Heap %d bytes", esp_get_free_heap_size() );
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING,
        TAG_MAIN,
        "Debug Mode enabled");
    debug::Diagnostic::start();
#endif

    // Initialize NVS partitions.
    // Critical for application operation. Will be before any NVM access.
    esp_err_t err = NVM::getInstance().Init();
    if (err != ESP_OK)
    {
        // Critical error
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR,
                          TAG_MAIN,
                          "Internal NVM access failed '%s'", esp_err_to_name(err));
        esp_system_abort("Verify NVM partition label in 'nvm_partition.h' and 'partitions.csv'");
    }

    // Initialize device context singleton with restore from NVS
    auto& ctxDevice = CtxDevice::getInstance();
    err = ctxDevice.Init();
    if (err != ESP_OK) {
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING,
            TAG_MAIN,
            "CtxDevice initialization failed: '%s'", esp_err_to_name(err));
    }

    // Initialize Channel Router and restore channel configurations
    auto& channelRouter = channels::ChannelRouter::getInstance();
#ifdef CONFIG_TAPGATE_DEBUG_MODE
    ESP_LOGI(TAG_DEBUG, "Channel count: %zu", channelRouter.getChannelCount());
    for (size_t i = 0; i < channelRouter.getChannelCount(); ++i) {
        auto& channel = channelRouter.get(static_cast<channels::ChannelType>(i));
        ESP_LOGI(TAG_DEBUG, "Channel %zu: \"%s\" Status: %s", i+1,
            channels::toString(channel.GetType()).data(),
            channels::toString(channel.GetStatus()).data());
    }
#endif
    channelRouter.RestoreConfigs();

    // Initialization complete. Main application loop
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
        TAG_MAIN,
        "TapGate firmware startup complete");

    while (true) {

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

