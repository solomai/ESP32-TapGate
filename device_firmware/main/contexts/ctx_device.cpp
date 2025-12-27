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
    ESP_LOGI(TAG, "CtxDevice Initialize");
    return LoadCtxDeviceFromNVS();
}

esp_err_t CtxDevice::LoadCtxDeviceFromNVS() noexcept
{
    // TODO:
    ESP_LOGW(TAG,"Load CtxDevice is not implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t CtxDevice::StoreCtxDeviceToNVS() noexcept
{
    // TODO:
    ESP_LOGW(TAG,"Store CtxDevice is not implemented");
    return ESP_ERR_NOT_SUPPORTED;
}
