/**
 * Interface for channels implementation
 */

#pragma once

#include <span>
#include <utility>
#include <functional>
#include <cstdint>

#include "status.h"

namespace channels {

// Channel types enumeration. Used to initialize and access channels.
enum ChannelType {
    BLEChannel = 0,

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
        ChannelType GetType() const {
            return type_;
        }

        // status getter
        Status GetStatus() const {
            return status_;
        }

        // config getter
        IChannelConfig* GetConfig() const {
            return config_;
        }

        // config setter
        void SetConfig(IChannelConfig* config) {
            config_ = config;
            OnSetConfig(config_);
        }

    public:
        // lifecycle methods
        virtual bool Start() = 0;

        virtual void Stop() = 0;

        // data sending method
        virtual bool Send(std::span<const std::uint8_t> data) = 0;

    protected:
        // Triggered when configuration is set by SetConfig
        virtual void OnSetConfig(IChannelConfig* config) = 0;

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
        ChannelType type_;
        Status status_ = Status::Uninitialized;
        IChannelConfig* config_ = nullptr;
};
} // namespace channels