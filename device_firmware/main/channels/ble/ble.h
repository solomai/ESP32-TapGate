/**
 * BLE channel implementation.
 */

#pragma once
#include "sdkconfig.h"
// Enable or Disable BLE channel implementation based on configuration
#ifdef CONFIG_TAPGATE_CHANNEL_BLE

#include <string_view>

#include "../channels.h"
#include "ble_config.h"

#include "host/ble_uuid.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"

namespace channels 
{
class BLE; // Forward declaration to have template specialization before class definition
// Map ChannelType::BLEChannel to BLE class. Used to automatically instantiate channels in ChannelRouter.
template<> struct MapType<ChannelType::BLEChannel> { 
    using type = BLE; 
    static constexpr std::string_view name = "BLE";
};
class BLE : public ChannelBase<BLEConfig>
{
    public:
        BLE();
        ~BLE();

    public:
        // lifecycle methods
        bool Start() override;

        void Stop() override;

        // data sending method
        bool Send(std::span<const std::uint8_t> data) override;

    protected:
        // Validation before applying configuration. Return true if valid.
        bool OnConfigValidate(const BLEConfig& config) override;
        // Triggered when configuration is set by SetConfig and Validation passed
        void OnSetConfig(const BLEConfig& config) override;

    private:
        esp_err_t InitializeNimBLE();
        void DeinitNimBLE();
        int  RegisterGATTService();
        int  HandleGAPEvent(struct ble_gap_event* event);
        bool StartAdvertising();
        void StopAdvertising();

        // Connection Management
        void HandleConnect(uint16_t conn_handle);
        void HandleDisconnect();
        void HandleMTUUpdate(uint16_t mtu);

    private:
        // Static callbacks for NimBLE (C-style)
        static int GATTAccessCallback(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt* ctxt, void* arg);
        static int HandleGATTAccess(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt* ctxt);
        static int GAPEventCallback(struct ble_gap_event* event, void* arg);

        // NimBLE Callbacks
        static void OnSyncCallback();
        static void OnResetCallback(int reason);
        static void HostTaskCallback(void* param);

    private:
        // Static instance pointer for callbacks
        static BLE* instance_;

        // State management
        bool advertising_;
        uint16_t conn_handle_;
        uint16_t tx_val_handle_;
        uint16_t rx_val_handle_;        

        // GATT Service definition
        struct ble_gatt_svc_def gatt_service_[2];
        struct ble_gatt_chr_def gatt_characteristics_[3];        
        // GATT service and characteristic UUIDs
        ble_uuid128_t service_uuid_;
        ble_uuid128_t rx_char_uuid_;
        ble_uuid128_t tx_char_uuid_;
};

} // namespace channels

#endif // CONFIG_TAPGATE_CHANNEL_BLE