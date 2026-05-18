#include "device_ctx.h"

#include "esp_log.h"
#include "nvm.h"
#include "nvm_partition.h"

#include <cstring>

static constexpr char TAG[]                          = "DeviceCtx";
// NVS namespace
static constexpr char NVS_CTXDEVICE_NAMESPACE[]      = "CtxDevice";
// NVS keys
static constexpr char NVS_CTXDEVICEKEY_DEVICE_NAME[] = "DeviceName";

// Single source of truth for the compile-time default device name
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

esp_err_t DeviceContext::Init() noexcept
{
    esp_err_t init_error = ESP_OK;
    ESP_LOGI(TAG, "Initializing");
    const esp_err_t err = load_device_name();
    // Apply default if not found in Non-Volative Memory (NVM) — expected on first boot before any name has been set
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::memcpy(m_device_name, DEVICE_NAME_DEFAULT, sizeof(DEVICE_NAME_DEFAULT));
        ESP_LOGW(TAG, "DeviceName not in NVS, using default: \"%s\"", m_device_name);
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read DeviceName from NVS: %s", esp_err_to_name(err));
        init_error = err;
    }

    // Dump to log all fields of Device Context
    ESP_LOGI(TAG, "DeviceName loaded: \"%s\"", m_device_name);

    return init_error;
}

esp_err_t DeviceContext::get_device_name(std::span<char> out) const noexcept
{
    if (out.empty())
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(m_mutex);
    const std::size_t name_len = std::strlen(m_device_name);
    if (out.size() <= name_len)
        return ESP_ERR_INVALID_SIZE;

    std::memcpy(out.data(), m_device_name, name_len + 1);
    return ESP_OK;
}

esp_err_t DeviceContext::set_device_name(std::string_view name) noexcept
{
    if (name.size() >= DEVICE_NAME_CAPACITY)
        return ESP_ERR_INVALID_ARG;
    // NVS stores C-strings — embedded nulls would cause silent truncation on readback
    if (name.find('\0') != std::string_view::npos)
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(m_mutex);
    std::memcpy(m_device_name, name.data(), name.size());
    m_device_name[name.size()] = '\0';
    return store_device_name();
}

esp_err_t DeviceContext::load_device_name() noexcept
{
    char buf[DEVICE_NAME_CAPACITY]{};
    const esp_err_t err = NVM.ReadString(NVM_PARTITION_CTXDEVICE,
                                         NVS_CTXDEVICE_NAMESPACE,
                                         NVS_CTXDEVICEKEY_DEVICE_NAME,
                                         buf, sizeof(buf));

    if (err == ESP_ERR_NVS_NOT_FOUND || (err == ESP_OK && buf[0] == '\0'))
        return ESP_ERR_NVS_NOT_FOUND;

    if (err != ESP_OK)
        return err;

    std::lock_guard<std::mutex> lock(m_mutex);
    std::strncpy(m_device_name, buf, DEVICE_NAME_CAPACITY - 1);
    m_device_name[DEVICE_NAME_CAPACITY - 1] = '\0';
    return ESP_OK;
}

esp_err_t DeviceContext::store_device_name() noexcept
{
    // m_mutex must be held by the caller
    esp_err_t err = NVM.WriteString(NVM_PARTITION_CTXDEVICE,
                                    NVS_CTXDEVICE_NAMESPACE,
                                    NVS_CTXDEVICEKEY_DEVICE_NAME,
                                    m_device_name);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store DeviceName: %s", esp_err_to_name(err));
    }
    return err;
}
