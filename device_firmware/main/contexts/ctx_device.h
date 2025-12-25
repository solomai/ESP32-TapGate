/**
 * Device context singleton
 * 
 */
#pragma once

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

protected:
    void LoadCtxDeviceFromNVS();
    void StoreCtxDeviceToNVS();

private:
    CtxDevice();
    ~CtxDevice() = default;
};
