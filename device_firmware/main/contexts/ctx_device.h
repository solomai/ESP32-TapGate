/**
 * Device context singleton
 * 
 */
#pragma once
#include "device_err.h"
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

protected:
    esp_err_t LoadCtxDeviceFromNVS() noexcept;
    esp_err_t StoreCtxDeviceToNVS() noexcept;

private:
    CtxDevice() = default;
    ~CtxDevice() = default;
};
