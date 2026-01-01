/**
 * Device context singleton
 * 
 */
#pragma once
#include "sdkconfig.h"
#include "types.h"
#include "device_err.h"
#include <cstddef>
#include <string_view>
class CtxDevice
{
public:
    static CtxDevice& getInstance() noexcept;

    // Delete copy constructor and assignment operator
    CtxDevice(const CtxDevice&) = delete;
    CtxDevice& operator=(const CtxDevice&) = delete;

    // Delete move constructor and assignment operator
    CtxDevice(CtxDevice&&) = delete;
    CtxDevice& operator=(CtxDevice&&) = delete;

public:
    esp_err_t Init() noexcept;

    std::string_view getDeviceName() const noexcept;
    void setDeviceName(const std::string_view& value) noexcept;

protected:
    esp_err_t LoadCtxDeviceFromNVS() noexcept;
    esp_err_t StoreCtxDeviceToNVS() noexcept;

private:
    CtxDevice() = default;
    ~CtxDevice() = default;

private:
    // Field: device name
    // Internal buffer for device name initialized at compile time from Kconfig if available
#ifdef CONFIG_TAPGATE_DEVICE_DEFAULT_NAME
static_assert(sizeof(CONFIG_TAPGATE_DEVICE_DEFAULT_NAME) <= DEVICE_NAME_CAPACITY,
              "CONFIG_TAPGATE_DEVICE_DEFAULT_NAME must be <= " STR(DEVICE_NAME_CAPACITY) " bytes including NUL");
#endif
    // Initialize with default name if defined, else empty
#ifdef CONFIG_TAPGATE_DEVICE_DEFAULT_NAME
    mutable char device_name_buffer_[DEVICE_NAME_CAPACITY] = CONFIG_TAPGATE_DEVICE_DEFAULT_NAME;
#else
    mutable char device_name_buffer_[DEVICE_NAME_CAPACITY]{};
#endif
};
