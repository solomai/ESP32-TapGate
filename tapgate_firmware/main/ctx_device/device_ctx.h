#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <span>
#include <string_view>

#include "device_err.h"
#include "constants.h"
#include "types.h"
#include "device_entity.h"

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

    // Set/Get Device Entity (all fields as a unit, stored as blob in NVM_PARTITION_ENTITY)
    [[nodiscard]] esp_err_t get_device_entity(device_entity_t *entity) const noexcept;
    [[nodiscard]] esp_err_t update_device_entity(const device_entity_t *entity) noexcept;

    // Get/Set Device Name (convenience accessor — name lives inside the entity blob)
    [[nodiscard]] esp_err_t get_device_name(std::span<char> out) const noexcept;
    [[nodiscard]] esp_err_t set_device_name(std::string_view name) noexcept;

    // Get/Set Nonce (stored as u32 in NVM_PARTITION_NONCE, separate from entity)
    [[nodiscard]] esp_err_t get_nonce(tg_nonce_t *nonce) const noexcept;
    [[nodiscard]] esp_err_t set_nonce(tg_nonce_t nonce) noexcept;

    // Read-only accessors for individual entity fields
    [[nodiscard]] esp_err_t get_public_key(tg_public_key_t pubkey) const noexcept;
    [[nodiscard]] esp_err_t get_private_key(tg_private_key_t prvkey) const noexcept;
    [[nodiscard]] esp_err_t get_device_id(tg_uid_t device_id) const noexcept;

private:
    DeviceContext() = default;
    ~DeviceContext() = default;

    // Entity helpers — store_entity() must be called with m_mutex held
    esp_err_t load_entity() noexcept;
    esp_err_t store_entity() noexcept;

    // Nonce helpers — no mutex required; m_nonce is atomic
    esp_err_t load_nonce() noexcept;
    esp_err_t store_nonce() noexcept;

    mutable std::mutex m_mutex;

    // All uint8_t[] fields — no struct padding possible
    static_assert(sizeof(device_entity_t) == UID_CAP + NAME_MAX_SIZE + PUBKEY_CAP + PRVKEY_CAP,
                  "device_entity_t has unexpected padding — blob layout would be broken");

#ifdef CONFIG_TAPGATE_DEVICE_DEFAULT_NAME
    static_assert(sizeof(CONFIG_TAPGATE_DEVICE_DEFAULT_NAME) <= NAME_MAX_SIZE,
                  "CONFIG_TAPGATE_DEVICE_DEFAULT_NAME must be <= " STR(NAME_MAX_SIZE) " bytes including NUL");
#endif

    device_entity_t          m_entity{};
    std::atomic<tg_nonce_t>     m_nonce{0};

}; // class DeviceContext

// Global instance of DeviceContext
extern DeviceContext& DeviceCtx;
