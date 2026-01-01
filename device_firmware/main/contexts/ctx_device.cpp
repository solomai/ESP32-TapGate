#include "ctx_device.h"
#include "esp_log.h"
#include "common/nvm/nvm.h"
#include <cstring>
#include <algorithm>

static const char* TAG = "CtxDevice";
static const char* NVS_CTXDEVICE_NAMESPACE = "CtxDevice";
// NVS Keys
static const char* NVS_CTXDEVICEKEY_DEVICE_NAME = "DeviceName";

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
    ESP_LOGI(TAG, "Loading CtxDevice from NVS");

    // Attempt to read the device name from NVM into our internal buffer
    esp_err_t err = NVM::getInstance().ReadString(
        NVM_PARTITION_CTXDEVICE,
        NVS_CTXDEVICE_NAMESPACE,
        NVS_CTXDEVICEKEY_DEVICE_NAME,
        device_name_buffer_,
        DEVICE_NAME_CAPACITY);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load device name: " ERR_FORMAT, esp_err_to_str(err), err);
    }

    return ESP_OK;
}

esp_err_t CtxDevice::StoreCtxDeviceToNVS() noexcept
{
    // Write the current device name buffer to NVM
    // Ensure null-termination
    device_name_buffer_[DEVICE_NAME_CAPACITY - 1] = '\0';

    esp_err_t err = NVM::getInstance().WriteString(
        NVM_PARTITION_CTXDEVICE,
        NVS_CTXDEVICE_NAMESPACE,
        NVS_CTXDEVICEKEY_DEVICE_NAME,
        device_name_buffer_);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to store device name to NVS: " ERR_FORMAT, esp_err_to_str(err), err);
        return err;
    }

    ESP_LOGI(TAG, "Stored CtxDevice to NVS." );
    return ESP_OK;
}

std::string_view CtxDevice::getDeviceName() const noexcept
{
    return std::string_view {
        device_name_buffer_,
        std::strlen(device_name_buffer_)
    };
}

void CtxDevice::setDeviceName(const std::string_view& value) noexcept
{
    const size_t copy_len = std::min(value.size(), DEVICE_NAME_CAPACITY - 1);
    std::memset(device_name_buffer_, 0, DEVICE_NAME_CAPACITY);
    std::memcpy(device_name_buffer_, value.data(), copy_len);
}