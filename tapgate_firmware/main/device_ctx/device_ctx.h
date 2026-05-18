#pragma once

#include "device_err.h"
#include "constants.h"
#include "types.h"

#include <cstddef>
#include <mutex>
#include <span>
#include <string_view>

class DeviceContext
{
public:
    static DeviceContext& getInstance() noexcept;

    DeviceContext(const DeviceContext&)            = delete;
    DeviceContext& operator=(const DeviceContext&) = delete;
    DeviceContext(DeviceContext&&)                 = delete;
    DeviceContext& operator=(DeviceContext&&)      = delete;

public:
    // Initialize Device Context from NVM. Must be called before any other API.
    esp_err_t Init() noexcept;

    // Get/Set Device Name
    [[nodiscard]] esp_err_t get_device_name(std::span<char> out) const noexcept;
    [[nodiscard]] esp_err_t set_device_name(std::string_view name) noexcept;

private:
    DeviceContext() = default;
    ~DeviceContext() = default;

    esp_err_t load_device_name() noexcept;
    esp_err_t store_device_name() noexcept; // Must be called with m_mutex held

    mutable std::mutex m_mutex;

    // sizeof includes the NUL terminator — checks the full C-string fits in the buffer
#ifdef CONFIG_TAPGATE_DEVICE_DEFAULT_NAME
    static_assert(sizeof(CONFIG_TAPGATE_DEVICE_DEFAULT_NAME) <= NAME_MAX_SIZE,
                  "CONFIG_TAPGATE_DEVICE_DEFAULT_NAME must be <= " STR(NAME_MAX_SIZE) " bytes including NUL");
#endif
    // Default value is applied by Init() via load_device_name()
    char m_device_name[NAME_MAX_SIZE]{};

}; // class DeviceContext

// Global instance of DeviceContext
extern DeviceContext& DeviceCtx;
