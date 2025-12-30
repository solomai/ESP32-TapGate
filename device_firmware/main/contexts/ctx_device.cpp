#include "ctx_device.h"
#include "esp_log.h"
#include "common/nvm/nvm.h"
#include <cstring>
#include <algorithm>

static const char* TAG = "CtxDevice";
static const char* NVS_NAMESPACE = "CtxDevice";
static const char* NVS_KEY_DEVICE_NAME = "DeviceName";

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
    // Attempt to read the device name from NVM into our internal buffer
    esp_err_t err = NVM::getInstance().Read_String(
        NVM_PARTITION_CTXDEVICE,
        NVS_NAMESPACE,
        NVS_KEY_DEVICE_NAME,
        device_name_buffer_,
        DEVICE_NAME_CAPACITY);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load device name from NVS: " ERR_FORMAT, esp_err_to_str(err), err);
    }

    ESP_LOGI(TAG, "Loaded device name from NVS: %s", device_name_buffer_);
    return ESP_OK;
}

esp_err_t CtxDevice::StoreCtxDeviceToNVS() noexcept
{
    // Write the current device name buffer to NVM
    // Ensure null-termination
    device_name_buffer_[DEVICE_NAME_CAPACITY - 1] = '\0';

    esp_err_t err = NVM::getInstance().Write_String(
        NVM_PARTITION_CTXDEVICE,
        NVS_NAMESPACE,
        NVS_KEY_DEVICE_NAME,
        device_name_buffer_);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to store device name to NVS: " ERR_FORMAT, esp_err_to_str(err), err);
        return err;
    }

    ESP_LOGI(TAG, "Stored device name to NVS: %s", device_name_buffer_);
    return ESP_OK;
}

std::span<const char> CtxDevice::getDeviceName() const noexcept
{
    auto len = std::strlen(device_name_buffer_);
    return std::span<const char>(device_name_buffer_, static_cast<std::size_t>(len));
}

void CtxDevice::setDeviceName(std::span<const char> name) noexcept
{
    auto copy_len = std::min(name.size(), DEVICE_NAME_CAPACITY - 1);
    std::memset(device_name_buffer_, 0, DEVICE_NAME_CAPACITY);
    std::memcpy(device_name_buffer_, name.data(), copy_len);
}