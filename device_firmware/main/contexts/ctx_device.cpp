#include "ctx_device.h"
#include "esp_log.h"

static const char* TAG = "CtxDevice";

CtxDevice& CtxDevice::getInstance() noexcept
{
    static CtxDevice instance;
    return instance;
}

esp_err_t CtxDevice::Init() noexcept
{
    ESP_LOGI(TAG, "CtxDevice Initializing");
    return LoadCtxDeviceFromNVS();
}

esp_err_t CtxDevice::LoadCtxDeviceFromNVS() noexcept
{
    // TODO:
    ESP_LOG_NOTIMPLEMENTED(TAG);
    return ESP_ERR_DEV_NOT_IMPLEMENTED;
}

esp_err_t CtxDevice::StoreCtxDeviceToNVS() noexcept
{
    // TODO:
    ESP_LOG_NOTIMPLEMENTED(TAG);
    return ESP_ERR_DEV_NOT_IMPLEMENTED;
}
