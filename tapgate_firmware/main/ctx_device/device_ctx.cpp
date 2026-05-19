#include <cstring>

#include "esp_log.h"

#include "device_ctx.h"
#include "nvm.h"
#include "nvm_partition.h"
#include "fcall.h"
#include "uuid.h"

static constexpr char TAG[] = "DeviceCtx";

// Entity: stored as a single blob in NVM_PARTITION_ENTITY
static constexpr char NVS_CTXDEVICE_NS[]          = "CtxDevice";
static constexpr char NVS_CTXDEVICE_KEY_ENTITY[]  = "Entity";

// Nonce: stored separately in NVM_PARTITION_NONCE for independent update cycles
static constexpr char NVS_DEVICENONCE_NS[]        = "CtxDevice";
static constexpr char NVS_DEVICENONCE_KEY_NONCE[] = "Nonce";

static constexpr char DEVICE_NAME_DEFAULT[] =
#ifdef CONFIG_TAPGATE_DEVICE_DEFAULT_NAME
    CONFIG_TAPGATE_DEVICE_DEFAULT_NAME;
#else
    "";
#endif

DeviceContext& DeviceContext::getInstance() noexcept
{
    static DeviceContext instance;
    return instance;
}

DeviceContext& DeviceCtx = DeviceContext::getInstance();

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

esp_err_t DeviceContext::Init() noexcept
{
    ESP_LOGI(TAG, "Initializing");

    esp_err_t init_err = ESP_OK;
    {
        const esp_err_t err = load_entity();
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            // Generate device ID outside the lock — crypto operation is expensive
            tg_uid_t device_id = {};
            const esp_err_t uid_err = generate_rng_uid(device_id);
            if (uid_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to generate device ID: %s", esp_err_to_name(uid_err));
                init_err = uid_err;
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                std::memset(&m_entity, 0, sizeof(m_entity));
                std::memcpy(m_entity.name, DEVICE_NAME_DEFAULT, sizeof(DEVICE_NAME_DEFAULT));
                if (uid_err == ESP_OK) {
                    std::memcpy(m_entity.device_id, device_id, UID_CAP);
                }
            }
            ESP_LOGW(TAG, "Entity not in NVS, applying defaults");
        } else if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load entity: %s", esp_err_to_name(err));
            init_err = err;
        }
    }

    {
        const esp_err_t err = load_nonce();
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            m_nonce.store(0, std::memory_order_relaxed);
            ESP_LOGW(TAG, "Nonce not in NVS, starting at 0");
        } else if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load nonce: %s", esp_err_to_name(err));
            init_err = err;
        }
    }

#ifdef CONFIG_APP_DEBUG_MODE
    {
        device_entity_t entity{};
        tg_nonce_t      nonce{};
        CALLW(TAG, DeviceCtx.get_device_entity(&entity));
        CALLW(TAG, DeviceCtx.get_nonce(&nonce));

        auto is_set = [](const uint8_t* data, std::size_t n) -> bool {
            for (std::size_t i = 0; i < n; ++i)
                if (data[i]) return true;
            return false;
        };

        // Extract C++ expressions before passing to ESP_LOGI —
        // IDF v6 macro chain does not tolerate cast syntax inside __VA_ARGS__
        const char* const name_str   = reinterpret_cast<const char*>(entity.name);
        const char* const pubkey_str = is_set(entity.pub_key,     PUBKEY_CAP) ? "Yes" : "No";
        const char* const prvkey_str = is_set(entity.private_key, PRVKEY_CAP) ? "Yes" : "No";
        const unsigned long nonce_val = static_cast<unsigned long>(nonce);

        char id_str[UID_STR_LEN]{};
        uid_to_str(entity.device_id, {id_str, sizeof(id_str)});

        ESP_LOGI(TAG,
            "Device Context:"
            "\n  Name:       \"%s\""
            "\n  Id:         \"%s\""
            "\n  PublicKey:  %s"
            "\n  PrivateKey: %s"
            "\n  Nonce:      %lu",
            name_str, id_str, pubkey_str, prvkey_str, nonce_val);
    }
#endif
    return init_err;
}

// ---------------------------------------------------------------------------
// Entity
// ---------------------------------------------------------------------------

esp_err_t DeviceContext::get_device_entity(device_entity_t* entity) const noexcept
{
    if (!entity)
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(m_mutex);
    std::memcpy(entity, &m_entity, sizeof(m_entity));
    return ESP_OK;
}

esp_err_t DeviceContext::update_device_entity(const device_entity_t* entity) noexcept
{
    if (!entity)
        return ESP_ERR_INVALID_ARG;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::memcpy(&m_entity, entity, sizeof(m_entity));
    }
    return store_entity();
}

// ---------------------------------------------------------------------------
// Device name (convenience — name lives inside the entity blob)
// ---------------------------------------------------------------------------

esp_err_t DeviceContext::get_device_name(std::span<char> out) const noexcept
{
    if (out.empty())
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(m_mutex);
    const auto* name = reinterpret_cast<const char*>(m_entity.name);
    const std::size_t name_len = std::strlen(name);
    if (out.size() <= name_len)
        return ESP_ERR_INVALID_SIZE;

    std::memcpy(out.data(), name, name_len + 1);
    return ESP_OK;
}

esp_err_t DeviceContext::set_device_name(std::string_view name) noexcept
{
    if (name.size() >= NAME_MAX_SIZE)
        return ESP_ERR_INVALID_ARG;
    // NVS stores C-strings — embedded nulls would cause silent truncation on readback
    if (name.find('\0') != std::string_view::npos)
        return ESP_ERR_INVALID_ARG;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::memcpy(m_entity.name, name.data(), name.size());
        m_entity.name[name.size()] = '\0';
    }
    return store_entity();
}

// ---------------------------------------------------------------------------
// Nonce
// ---------------------------------------------------------------------------

esp_err_t DeviceContext::get_nonce(tg_nonce_t* nonce) const noexcept
{
    if (!nonce)
        return ESP_ERR_INVALID_ARG;

    *nonce = m_nonce.load(std::memory_order_relaxed);
    return ESP_OK;
}

esp_err_t DeviceContext::set_nonce(tg_nonce_t nonce) noexcept
{
    m_nonce.store(nonce, std::memory_order_relaxed);
    return store_nonce();
}

// ---------------------------------------------------------------------------
// Individual field getters (read-only views into the cached entity)
// ---------------------------------------------------------------------------

esp_err_t DeviceContext::get_public_key(tg_public_key_t pubkey) const noexcept
{
    if (!pubkey)
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(m_mutex);
    std::memcpy(pubkey, m_entity.pub_key, PUBKEY_CAP);
    return ESP_OK;
}

esp_err_t DeviceContext::get_private_key(tg_private_key_t prvkey) const noexcept
{
    if (!prvkey)
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(m_mutex);
    std::memcpy(prvkey, m_entity.private_key, PRVKEY_CAP);
    return ESP_OK;
}

esp_err_t DeviceContext::get_device_id(tg_uid_t device_id) const noexcept
{
    if (!device_id)
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(m_mutex);
    std::memcpy(device_id, m_entity.device_id, UID_CAP);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// NVM helpers
// ---------------------------------------------------------------------------

esp_err_t DeviceContext::load_entity() noexcept
{
    device_entity_t buf{};
    const esp_err_t err = NVM.ReadBlob(NVM_PARTITION_ENTITY,
                                       NVS_CTXDEVICE_NS,
                                       NVS_CTXDEVICE_KEY_ENTITY,
                                       &buf, sizeof(buf));
    if (err != ESP_OK)
        return err;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_entity = buf;
    return ESP_OK;
}

esp_err_t DeviceContext::store_entity() noexcept
{
    device_entity_t entity{};
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        entity = m_entity;
    }
    const esp_err_t err = NVM.WriteBlob(NVM_PARTITION_ENTITY,
                                        NVS_CTXDEVICE_NS,
                                        NVS_CTXDEVICE_KEY_ENTITY,
                                        &entity, sizeof(entity));
    if (err == ESP_OK){
        ESP_LOGI(TAG, "Entity stored successfully");
    } else {
        ESP_LOGE(TAG, "Failed to store entity: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t DeviceContext::load_nonce() noexcept
{
    tg_nonce_t val = 0;
    const esp_err_t err = NVM.ReadU32(NVM_PARTITION_NONCE,
                                      NVS_DEVICENONCE_NS,
                                      NVS_DEVICENONCE_KEY_NONCE,
                                      &val);
    if (err != ESP_OK)
        return err;

    m_nonce.store(val, std::memory_order_relaxed);
    return ESP_OK;
}

esp_err_t DeviceContext::store_nonce() noexcept
{
    const esp_err_t err = NVM.WriteU32(NVM_PARTITION_NONCE,
                                       NVS_DEVICENONCE_NS,
                                       NVS_DEVICENONCE_KEY_NONCE,
                                       m_nonce.load(std::memory_order_relaxed));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Nonce stored successfully. Value: %lu",
            static_cast<unsigned long>(m_nonce.load(std::memory_order_relaxed)));
    } else {
        ESP_LOGE(TAG, "Failed to store nonce: %s", esp_err_to_name(err));
    }
    return err;
}
