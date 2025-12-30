/**
 * Interface for channels implementation
 */

#pragma once

#include <span>
#include <utility>
#include <functional>
#include <cstdint>

#include "device_err.h"
#include "status.h"

namespace channels {

// Channel types enumeration. Used to initialize and access channels.
enum ChannelType {
#ifdef CONFIG_TAPGATE_CHANNEL_BLE
    BLEChannel,
#endif
    // add new channel types here
    // ...

#ifdef CONFIG_TAPGATE_CHANNEL_DUMMY
    DUMMYChannel,
#endif    
    // Must be last
    _Count
};

// Map ChannelType to corresponding channel type
template<ChannelType T> struct MapType;
// forward declaration of toString function
constexpr std::string_view toString(ChannelType type);

// Channel Config interface
class IChannelConfig
{
    public:
        virtual ~IChannelConfig() = default;

        virtual esp_err_t LoadFromNVS() = 0;
};

// Channel interface
class IChannel
{
    public:
        IChannel(ChannelType type) : type_(type) {}
        virtual ~IChannel() = default;
    
        // Delete default constructor
        IChannel() = delete;

        // Delete copy constructor and assignment operator
        IChannel(const IChannel&) = delete;
        IChannel& operator=(const IChannel&) = delete;

        // Delete move constructor and assignment operator
        IChannel(IChannel&&) = delete;
        IChannel& operator=(IChannel&&) = delete;

    public:
        // callbacks
        using OnReceiveCallback = std::function<void(std::span<const std::uint8_t> data)>;
        using OnStatusCallback  = std::function<void(Status status)>;

        // callback setters
        void SetOnReceiveCallback(OnReceiveCallback callback) {
            onReceiveCallback_ = std::move(callback);
        }

        void SetOnStatusCallback(OnStatusCallback callback) {
            onStatusCallback_ = std::move(callback);
        }

        // type getter
        inline ChannelType GetType() const {
            return type_;
        }

        // status getter
        inline Status GetStatus() const {
            return status_;
        }

        // set last error code
        inline void SetLastError(esp_err_t err) {
            lastError_ = err;
        }

        // reset last error code
        inline void ResetLastError() {
            lastError_ = ESP_OK;
        }

        // get last error code
        inline esp_err_t GetLastError() const {
            return lastError_;
        }

      public:
        // lifecycle methods
        virtual bool Start() = 0;

        virtual void Stop() = 0;

        // data sending method
        virtual bool Send(std::span<const std::uint8_t> data) = 0;

        // configuration methods
        virtual const IChannelConfig& GetConfig() const = 0;
        virtual void SetConfig(const IChannelConfig& config) = 0;
        virtual esp_err_t RestoreConfig() = 0;

    protected:
        // callbacks
        OnReceiveCallback onReceiveCallback_ = nullptr;
        OnStatusCallback  onStatusCallback_  = nullptr;

        // Safe callbacks invoker
        void InvokeOnReceiveCallback(std::span<const std::uint8_t> data) {
            if (onReceiveCallback_) {
                onReceiveCallback_(data);
            }
        }

        void InvokeOnStatusCallback(Status status) {
            if (onStatusCallback_) {
                onStatusCallback_(status);
            }
        }

    protected:
        void SetStatus(Status status) {
            if (status_ != status) {
                status_ = status;
                InvokeOnStatusCallback(status_);
            }
        }

    private:
        esp_err_t lastError_ = ESP_OK;
        ChannelType type_;
        Status status_ = Status::Uninitialized;
}; // class IChannel

// Base class template for channels with specific configuration types
template<typename TConfig>
class ChannelBase : public IChannel
{
    public:
        ChannelBase(ChannelType type)
            : IChannel(type) {
        }

        const IChannelConfig& GetConfig() const override { 
            return config_;
        }
        
        void SetConfig(const IChannelConfig& config) override {
            const TConfig& cfg = static_cast<const TConfig&>(config);
            // Validate configuration before applying
            if (OnConfigValidate(cfg)) {
                config_ = cfg;
                // Notify a new configuration has been set
                OnSetConfig(cfg);
            }
        }

        esp_err_t RestoreConfig() override {
            Stop();
            return config_.LoadFromNVS();
        }

    protected:
        virtual bool OnConfigValidate(const TConfig& config) = 0;
        virtual void OnSetConfig(const TConfig& config) = 0;
        TConfig config_{};
};

} // namespace channels