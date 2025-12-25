#include "ctx_device.h"
#include "esp_log.h"

static const char* TAG = "CtxDevice";

CtxDevice& CtxDevice::getInstance() noexcept
{
    static CtxDevice instance;
    return instance;
}

CtxDevice::CtxDevice()
{
    ESP_LOGI(TAG, "Initializing device context...");

    LoadCtxDeviceFromNVS();
    // TODO: Add actual initialization code here
    
    ESP_LOGI(TAG, "Device context initialized successfully");
}

void CtxDevice::LoadCtxDeviceFromNVS()
{
    // TODO:
    ESP_LOGW(TAG,"Load CtxDevice is not implemented");
}

void CtxDevice::StoreCtxDeviceToNVS()
{
    // TODO:
    ESP_LOGW(TAG,"Store CtxDevice is not implemented");
}
