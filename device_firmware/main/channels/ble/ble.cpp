#include "ble.h"

// Enable or Disable BLE channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_BLE

// NimBLE includes
#include "host/ble_hs.h"
#include "host/ble_att.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "nimble/nimble_port_freertos.h"

//  Standard includes
#include <algorithm>
#include <ranges>
#include <cstring>

// Project includes
#include "common/event_journal/event_journal.h"
#include "contexts/ctx_device.h"

namespace channels {

static const char* TAG = "Ch::BLE";
BLE* BLE::instance_ = nullptr;

constexpr uint16_t chunkHeaderSize = 4; // Size of chunk header (sequence + total)

BLE::BLE()
    : ChannelBase(ChannelType::BLEChannel)
    , last_cleanup_(std::chrono::steady_clock::now())
    , advertising_(false)
    , conn_handle_(0)
#ifdef CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU
    , negotiated_mtu_(CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU) // Default MTU size before negotiation
#else
    , negotiated_mtu_(23) // Default MTU size before negotiation
#endif
    , tx_val_handle_(0)
    , rx_val_handle_(0)
{
    // Channel Objects will be constructed before main app_main starts.
    // Constructor will not perform any heavy operations and access to NVS and hardware

    // Set static instance
    instance_ = this;    
}

BLE::~BLE()
{
    Stop();
    instance_ = nullptr;
}

bool BLE::Start()
{
    if (GetStatus() >= Status::Connecting) {
        return false;
    }

    // Reset last esp error code on new Start attempt
    ResetLastError();

    esp_err_t ret = InitializeNimBLE();
    if (ESP_OK != ret) {
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR,
            TAG,
            "Init Bluetooth failed: " ERR_FORMAT, esp_err_to_str(ret), ret);
        SetLastError(ret);
        return false;
    }
    
    // Register GATT service
    int rc = RegisterGATTService();
    if ( rc != 0 ) {
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR,
            TAG,
            "Failed to register GATT service %d", rc);
        DeinitNimBLE();
        SetLastError(ESP_FAIL);
        return false;
    }

    SetStatus(Status::Connecting);
    return true;
}

void BLE::Stop()
{
    if (GetStatus() < Status::Connecting) {
        // If not connected or connecting, no need to stop
        return;
    }

    SetStatus(Status::Offline);
    ESP_LOG_NOTIMPLEMENTED(TAG);
    // Stop BLE channel implementation
}

bool BLE::Send(std::span<const std::uint8_t> data)
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
    // Send data over BLE channel implementation
    return false;
}

bool BLE::OnConfigValidate(const BLEConfig& config)
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
    // Validate BLE-specific configuration here
    return true;
}

void BLE::OnSetConfig(const BLEConfig& config)
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
    // Handle BLE-specific configuration here
}

esp_err_t BLE::InitializeNimBLE()
{
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE port init failed: " ERR_FORMAT, esp_err_to_str(ret), ret);
        return ret;
    }

    // Set sync and reset callbacks
    ble_hs_cfg.sync_cb = OnSyncCallback;
    ble_hs_cfg.reset_cb = OnResetCallback;

    // Set preferred MTU
    int rc = ble_att_set_preferred_mtu(negotiated_mtu_);
    if (rc != 0) {
        ESP_LOGW(TAG, "Failed to set preferred MTU: %d", rc);
    }

    // Initialize GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Set device name
    CtxDevice& ctx_device = CtxDevice::getInstance();
    ret = ble_svc_gap_device_name_set(ctx_device.getDeviceName().data());
    if (ret != 0) {
        ESP_LOGW(TAG, "Failed to set device name: " ERR_FORMAT, esp_err_to_str(ret), ret);
    }

    // Start NimBLE host task
    nimble_port_freertos_init(HostTaskCallback);
 
    ESP_LOGI(TAG, "NimBLE initialized");
    
    return ESP_OK;
}

void BLE::DeinitNimBLE()
{
    // Stop NimBLE host
    int ret = nimble_port_stop();
    if (ret != 0) {
        ESP_LOGW(TAG, "nimble_port_stop failed: %d", ret);
    }

    // Deinitialize NimBLE (handles both host and controller in ESP-IDF 5.x+)
    nimble_port_deinit();

    ESP_LOGI(TAG, "NimBLE deinitialized");
}

int BLE::RegisterGATTService()
{
    // Big Endian -> Little Endian
    auto assign_uuid = [](const auto& src_array, ble_uuid128_t& dest_struct) {    
    std::ranges::reverse_copy(src_array, dest_struct.value);
    };
    // Initialize UUIDs
    service_uuid_.u.type = BLE_UUID_TYPE_128;
    assign_uuid(config_.serviceUUID, service_uuid_);
    rx_char_uuid_.u.type = BLE_UUID_TYPE_128;
    assign_uuid(config_.rxCharUUID,  rx_char_uuid_);
    tx_char_uuid_.u.type = BLE_UUID_TYPE_128;
    assign_uuid(config_.txCharUUID,  tx_char_uuid_);

    // Define characteristics
    std::memset(&gatt_characteristics_[0], 0, sizeof(gatt_characteristics_[0]));
    gatt_characteristics_[0].uuid = &rx_char_uuid_.u;
    gatt_characteristics_[0].access_cb = GATTAccessCallback;
    gatt_characteristics_[0].arg = this;
    gatt_characteristics_[0].descriptors = nullptr;
    gatt_characteristics_[0].flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP;
    gatt_characteristics_[0].min_key_size = 0;
    gatt_characteristics_[0].val_handle = &rx_val_handle_;

    std::memset(&gatt_characteristics_[1], 0, sizeof(gatt_characteristics_[1]));
    gatt_characteristics_[1].uuid = &tx_char_uuid_.u;
    gatt_characteristics_[1].access_cb = GATTAccessCallback;
    gatt_characteristics_[1].arg = this;
    gatt_characteristics_[1].descriptors = nullptr;
    gatt_characteristics_[1].flags = BLE_GATT_CHR_F_NOTIFY;
    gatt_characteristics_[1].min_key_size = 0;
    gatt_characteristics_[1].val_handle = &tx_val_handle_;

    std::memset(&gatt_characteristics_[2], 0, sizeof(gatt_characteristics_[2])); // Terminator

    // Define service
    std::memset(&gatt_service_[0], 0, sizeof(gatt_service_[0]));
    gatt_service_[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
    gatt_service_[0].uuid = &service_uuid_.u;
    gatt_service_[0].includes = nullptr;
    gatt_service_[0].characteristics = gatt_characteristics_;

    std::memset(&gatt_service_[1], 0, sizeof(gatt_service_[1])); // Terminator
    
    // Register service
    int rc = ble_gatts_count_cfg(gatt_service_);
    if (rc != 0) {
        ESP_LOGE(TAG, "GATT count cfg failed: %d", rc);
        return rc;
    }
    
    rc = ble_gatts_add_svcs(gatt_service_);
    if (rc != 0) {
        ESP_LOGE(TAG, "GATT add service failed: %d", rc);
        return rc;
    }

    ESP_LOGI(TAG, "GATT service registered (TX handle: %d, RX handle: %d)",
             tx_val_handle_, rx_val_handle_);
    return ESP_OK;
}

bool BLE::StartAdvertising()
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    
    memset(&adv_params, 0, sizeof(adv_params));
    memset(&fields, 0, sizeof(fields));

    // Set advertising parameters
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = (config_.advIntervalMin * 1000) / 625;  // Convert ms to 0.625ms units
    adv_params.itvl_max = (config_.advIntervalMax * 1000) / 625;

    // Set advertising data
    CtxDevice& ctx_device = CtxDevice::getInstance();
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = reinterpret_cast<uint8_t*>(const_cast<char*>(ctx_device.getDeviceName().data()));
    fields.name_len = static_cast<int>(ctx_device.getDeviceName().size());
    fields.name_is_complete = 1;
    
    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set adv fields: %d", rc);
        return false;
    }

    // Start advertising
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, nullptr, BLE_HS_FOREVER,
                           &adv_params, GAPEventCallback, this);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start advertising: %d", rc);
        return false;
    }

    advertising_ = true;
    ESP_LOGI(TAG, "Advertising started");
    return true;
}

void BLE::StopAdvertising()
{
    if (advertising_) {
        ble_gap_adv_stop();
        advertising_ = false;
        ESP_LOGI(TAG, "Advertising stopped");
    }
}

void BLE::OnSyncCallback()
{
    if (instance_) {
        int rc = ble_hs_util_ensure_addr(0);
        if (rc == 0) {
            instance_->StartAdvertising();
        }
    }
}

void BLE::OnResetCallback(int reason)
{
    ESP_LOGW(TAG, "NimBLE host reset: %d", reason);
}

void BLE::HostTaskCallback(void* param)
{
    nimble_port_run();
}

int BLE::GATTAccessCallback(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt* ctxt, void* arg)
{
    BLE* ble = static_cast<BLE*>(arg);
    return ble ? ble->HandleGATTAccess(conn_handle, attr_handle, ctxt) : BLE_ATT_ERR_UNLIKELY;
}

int BLE::HandleGATTAccess(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt* ctxt)
{
    if (attr_handle == rx_val_handle_) {
        // RX characteristic - receiving data from client
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // Extract chunk header
            if (ctxt->om->om_len < chunkHeaderSize) {
                ESP_LOGE(TAG, "Invalid chunk (too small)");
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            uint16_t sequence = ctxt->om->om_data[0] | (ctxt->om->om_data[1] << 8);
            uint16_t totalChunks = ctxt->om->om_data[2] | (ctxt->om->om_data[3] << 8);

            // Extract data
            std::span<const uint8_t> data(
                ctxt->om->om_data + chunkHeaderSize,
                ctxt->om->om_len - chunkHeaderSize
            );

            ReassembleData(sequence, totalChunks, data);

            // Periodic cleanup
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cleanup_).count() > 10) {
                CleanupStaleReassemblyBuffers();
                last_cleanup_ = now;
            }
            return 0;
        }
    }
    return BLE_ATT_ERR_UNLIKELY;
}

int BLE::GAPEventCallback(struct ble_gap_event* event, void* arg)
{
    BLE* ble = static_cast<BLE*>(arg);
    return ble ? ble->HandleGAPEvent(event) : 0;
}

int BLE::HandleGAPEvent(struct ble_gap_event* event)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            HandleConnect(event->connect.conn_handle);
        } else {
            ESP_LOGE(TAG, "Connection failed: %d", event->connect.status);
            StartAdvertising();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        HandleDisconnect();
        break;

    case BLE_GAP_EVENT_MTU:
        HandleMTUUpdate(event->mtu.value);
        break;

    case BLE_GAP_EVENT_CONN_UPDATE:
        ESP_LOGI(TAG, "Connection updated");
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "Advertising complete");
        break;

    default:
        break;
    }

    return 0;
}

void BLE::HandleConnect(uint16_t conn_handle)
{
    conn_handle_ = conn_handle;
    StopAdvertising();
    SetStatus(Status::Online);
    
    ESP_LOGI(TAG, "Connected (handle: %d)", conn_handle);
}

void BLE::HandleDisconnect()
{
    conn_handle_ = BLE_HS_CONN_HANDLE_NONE;

#ifdef CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU
    negotiated_mtu_ = CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU; // Default MTU size before negotiation
#else
    negotiated_mtu_ = 23; // Default MTU size before negotiation
#endif

    ESP_LOG_NOTIMPLEMENTED(TAG);
}

void BLE::HandleMTUUpdate(uint16_t mtu)
{
    negotiated_mtu_ = mtu;
    ESP_LOGI(TAG, "MTU updated: %d (effective chunk: %d bytes)", 
             mtu, GetEffectiveChunkSize());
}

std::vector<uint8_t> BLE::CreateChunkHeader(uint16_t sequence, uint16_t totalChunks)
{
    static_assert(chunkHeaderSize == 4, "Chunk header size must be 4 bytes");
    std::vector<uint8_t> header(chunkHeaderSize);
    
    // Simple header format: [sequence(2)][totalChunks(2)]
    header[0] = sequence & 0xFF;
    header[1] = (sequence >> 8) & 0xFF;
    header[2] = totalChunks & 0xFF;
    header[3] = (totalChunks >> 8) & 0xFF;
    
    return header;
}

uint16_t BLE::GetEffectiveChunkSize() const
{
    // MTU - 3 (ATT header) - chunk header size
    const uint16_t att_overhead = 3;
    
    if (negotiated_mtu_ <= (att_overhead + chunkHeaderSize)) {
        return 0;
    }
    
    return negotiated_mtu_ - att_overhead - chunkHeaderSize;
}

bool BLE::ReassembleData(uint16_t sequence, uint16_t totalChunks, 
                          std::span<const uint8_t> chunkData)
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
    return false;
}

void BLE::CleanupStaleReassemblyBuffers()
{
    ESP_LOG_NOTIMPLEMENTED(TAG);
}

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_BLE