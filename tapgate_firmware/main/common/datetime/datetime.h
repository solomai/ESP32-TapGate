//
// DateTimeWrapper - System date/time management
//
// Wraps the ESP-IDF POSIX time API (gettimeofday / settimeofday / strftime)
// into a singleton with timezone support and strftime-based format helpers.
//
// Format strings use POSIX strftime specifiers (NOT .NET/C# format tokens).
// Predefined DT_FMT_* macros are provided for the most common patterns.
//
// Future extension point: SNTP synchronization will be integrated here
// (see Init() and the IsTimeSynchronized() hook).
//

#pragma once

#include "device_err.h"

#include <array>
#include <cstdint>
#include <ctime>
#include <string_view>
#include <sys/time.h>

// ---------------------------------------------------------------------------
// Predefined strftime format patterns
//
// All tokens follow POSIX strftime(3) conventions — the same subset that
// ESP-IDF's newlib libc exposes.  These are intentionally NOT .NET/C# format
// strings (which use yyyy/MM/HH/fff tokens and require a separate parser).
//
// Quick reference for the tokens used below:
//   %Y  - 4-digit year                 e.g. 2025
//   %m  - month 01..12
//   %d  - day 01..31
//   %H  - hour 00..23 (24-h)
//   %I  - hour 01..12 (12-h)
//   %M  - minute 00..59
//   %S  - second 00..59
//   %p  - AM / PM
//   %z  - UTC offset +HHMM / -HHMM    e.g. +0200
//   %Z  - timezone abbreviation        e.g. EET
//   %e  - day without leading zero     e.g.  5
//   %B  - full month name              e.g. May
//   %b  - abbreviated month name       e.g. May
//   %A  - full weekday name            e.g. Wednesday
//   %a  - abbreviated weekday name     e.g. Wed
//   %j  - day of year 001..366
//
// Note: sub-second precision (%3N / %f) is NOT supported by ESP-IDF newlib
// strftime.  Use FormatMs() / GetTimeVal() to obtain tv_usec and format
// milliseconds manually when needed.
// ---------------------------------------------------------------------------

// ISO-8601 date+time with UTC offset   "2025-05-14T13:45:00+0200"
#define DT_FMT_ISO8601         "%Y-%m-%dT%H:%M:%S%z"

// ISO-8601 UTC (append literal 'Z')    "2025-05-14T13:45:00Z"
#define DT_FMT_ISO8601_UTC     "%Y-%m-%dT%H:%M:%SZ"

// Log-friendly compact timestamp       "2025-05-14 13:45:00"
#define DT_FMT_LOG_TIMESTAMP   "%Y-%m-%d %H:%M:%S"

// Date only                            "2025-05-14"
#define DT_FMT_DATE_ONLY       "%Y-%m-%d"

// Time only (24-h)                     "13:45:00"
#define DT_FMT_TIME_ONLY       "%H:%M:%S"

// Time only (12-h with AM/PM)          "01:45:00 PM"
#define DT_FMT_TIME_12H        "%I:%M:%S %p"

// Human-readable date                  "14 May 2025"
#define DT_FMT_HUMAN_DATE      "%e %B %Y"

// Human-readable full                  "Wednesday, 14 May 2025 13:45"
#define DT_FMT_HUMAN_FULL      "%A, %e %B %Y %H:%M"

// Short date with abbreviated month    "14 May 2025"  -> "14 May 2025"
#define DT_FMT_SHORT_DATE      "%d %b %Y"

// Filesystem-safe sortable timestamp   "20250514_134500"
#define DT_FMT_FILENAME_TS     "%Y%m%d_%H%M%S"

// ---------------------------------------------------------------------------
// DateTimeWrapper
// ---------------------------------------------------------------------------

class DateTimeWrapper
{
public:
    // ------------------------------------------------------------------
    // Singleton access
    // ------------------------------------------------------------------
    static DateTimeWrapper& getInstance() noexcept;

    // Delete copy / move to enforce singleton semantics
    DateTimeWrapper(const DateTimeWrapper&)            = delete;
    DateTimeWrapper& operator=(const DateTimeWrapper&) = delete;
    DateTimeWrapper(DateTimeWrapper&&)                 = delete;
    DateTimeWrapper& operator=(DateTimeWrapper&&)      = delete;

public:
    // ------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------

    /**
     * @brief Initialize the datetime subsystem.
     *
     * Must be called once before any other method.
     * Configures the POSIX timezone environment from the stored TZ string.
     * If no timezone has been set yet, UTC is used as the default.
     *
     * Extension point: SNTP initialization will be added here in the future.
     *
     * @return ESP_OK on success, or an esp_err_t error code.
     */
    esp_err_t Init() noexcept;

    // ------------------------------------------------------------------
    // Set
    // ------------------------------------------------------------------

    /**
     * @brief Set the current date and time (UTC epoch).
     *
     * Calls POSIX settimeofday().  The caller supplies seconds and
     * microseconds since the Unix epoch (1970-01-01 00:00:00 UTC).
     *
     * Extension point: when SNTP is active this method will mark the time
     * as "manually overridden" so the sync logic can handle the conflict.
     *
     * @param epoch_sec   Seconds since Unix epoch (UTC).
     * @param epoch_usec  Microseconds fraction [0, 999999].
     * @return ESP_OK on success, ESP_FAIL if settimeofday() fails.
     */
    esp_err_t SetTime(time_t epoch_sec, suseconds_t epoch_usec = 0) noexcept;

    /**
     * @brief Set the current date/time from a broken-down struct tm (local time).
     *
     * Converts the struct tm to a UTC epoch via mktime() and delegates to
     * SetTime().  The struct tm is interpreted in the currently active
     * timezone (see SetTimezone / GetTimezone).
     *
     * @param tm  Broken-down local time.  tm.tm_isdst = -1 lets mktime
     *            determine DST automatically.
     * @return ESP_OK on success.
     */
    esp_err_t SetTime(const struct tm& tm) noexcept;

    /**
     * @brief Set the POSIX TZ string and activate it.
     *
     * The string follows the POSIX extended TZ format, e.g.:
     *   "UTC0"
     *   "EET-2EEST,M3.5.0,M10.5.0/3"   (Eastern European Time)
     *   "CST-8"                          (China Standard Time)
     *
     * The string is stored internally (max TZ_STRING_MAX_LEN - 1 chars)
     * and applied immediately via setenv("TZ") + tzset().
     *
     * @param tz_posix  POSIX TZ string (null-terminated).
     * @return ESP_OK on success, ESP_ERR_INVALID_ARG if the string is null
     *         or exceeds the internal buffer.
     */
    esp_err_t SetTimezone(const char* tz_posix) noexcept;

    // ------------------------------------------------------------------
    // Get
    // ------------------------------------------------------------------

    /**
     * @brief Get the current time as a timeval (UTC epoch, microsecond resolution).
     *
     * @param[out] tv  Populated with the current UTC time.
     * @return ESP_OK on success, ESP_FAIL if gettimeofday() fails.
     */
    esp_err_t GetTimeVal(struct timeval& tv) const noexcept;

    /**
     * @brief Get the current local time as a broken-down struct tm.
     *
     * Uses the active POSIX timezone (see SetTimezone).
     *
     * @param[out] out_tm  Populated with the local broken-down time.
     * @return ESP_OK on success, ESP_FAIL if gettimeofday() / localtime_r() fails.
     */
    esp_err_t GetLocalTime(struct tm& out_tm) const noexcept;

    /**
     * @brief Get the current UTC time as a broken-down struct tm.
     *
     * @param[out] out_tm  Populated with the UTC broken-down time.
     * @return ESP_OK on success.
     */
    esp_err_t GetUtcTime(struct tm& out_tm) const noexcept;

    /**
     * @brief Get the currently active POSIX TZ string.
     *
     * Returns a string_view into the internal buffer; valid for the
     * lifetime of this singleton.
     *
     * @return POSIX TZ string (never empty; defaults to "UTC0").
     */
    [[nodiscard]] std::string_view GetTimezone() const noexcept;

    // ------------------------------------------------------------------
    // Formatting helpers  (POSIX strftime format strings)
    // ------------------------------------------------------------------

    /**
     * @brief Format the current local time into a caller-supplied buffer.
     *
     * Uses POSIX strftime() directly.  Prefer the DT_FMT_* macros for
     * common patterns:
     *
     *   DateTime.Format(buf, sizeof(buf), DT_FMT_ISO8601)
     *     -> "2025-05-14T13:45:00+0200"
     *
     *   DateTime.Format(buf, sizeof(buf), DT_FMT_LOG_TIMESTAMP)
     *     -> "2025-05-14 13:45:00"
     *
     *   DateTime.Format(buf, sizeof(buf), DT_FMT_HUMAN_FULL)
     *     -> "Wednesday, 14 May 2025 13:45"
     *
     *   DateTime.Format(buf, sizeof(buf), DT_FMT_FILENAME_TS)
     *     -> "20250514_134500"
     *
     * Custom format strings use standard strftime(3) tokens (%Y, %m, %d ...).
     *
     * @param buffer      Destination buffer.
     * @param buffer_size Size of buffer in bytes (including null terminator).
     * @param fmt         strftime format string or a DT_FMT_* macro.
     * @return Number of characters written (excluding null), or 0 on error.
     */
    size_t Format(char* buffer, size_t buffer_size, const char* fmt) const noexcept;

    /**
     * @brief Format an arbitrary broken-down time into a caller-supplied buffer.
     *
     * Same semantics as Format(char*, size_t, const char*) but operates on
     * the supplied struct tm instead of the current time.
     *
     * @param buffer      Destination buffer.
     * @param buffer_size Size of buffer in bytes.
     * @param fmt         strftime format string.
     * @param tm          Broken-down time to format.
     * @return Number of characters written (excluding null), or 0 on error.
     */
    static size_t Format(char* buffer,
                         size_t buffer_size,
                         const char* fmt,
                         const struct tm& tm) noexcept;

    /**
     * @brief Format the current local time into a fixed-size stack buffer.
     *
     * Convenience overload for callers that do not need dynamic allocation.
     * Returns an std::array<char, N> populated with the formatted string.
     *
     * Usage example:
     *   auto buf = DateTime.FormatFixed<32>("%Y-%m-%d %H:%M:%S");
     *   ESP_LOGI(TAG, "Time: %s", buf.data());
     *
     * @tparam N          Buffer size (must be > 0).
     * @param  fmt        strftime format string.
     * @return Null-terminated character array.
     */
    template <size_t N>
    [[nodiscard]] std::array<char, N> FormatFixed(const char* fmt) const noexcept
    {
        static_assert(N > 0, "FormatFixed buffer size must be > 0");
        std::array<char, N> buf{};
        buf[0] = '\0';
        if (fmt)
        {
            Format(buf.data(), N, fmt);
        }
        return buf;
    }

    // ------------------------------------------------------------------
    // Sync state query (extension point for future SNTP integration)
    // ------------------------------------------------------------------

    /**
     * @brief Indicates whether the system clock has been set from an
     *        authoritative source (SNTP or manual SetTime call).
     *
     * Currently returns true only after a successful SetTime() call.
     * Future SNTP integration will also set this flag upon successful sync.
     */
    [[nodiscard]] bool IsTimeSynchronized() const noexcept;

private:
    DateTimeWrapper() = default;
    ~DateTimeWrapper()  = default;

    // Maximum length of the POSIX TZ string (including null terminator).
    // POSIX allows up to ~60 chars; 64 bytes gives comfortable headroom.
    static constexpr size_t TZ_STRING_MAX_LEN = 64u;

    // Internal TZ string storage (avoids heap allocation).
    char m_tz_string[TZ_STRING_MAX_LEN] = "UTC0";

    // Set to true after the first successful SetTime() call.
    // Acts as the hook for SNTP sync-state tracking in the future.
    bool m_time_synchronized = false;

    // Applies m_tz_string to the C runtime via setenv + tzset.
    esp_err_t ApplyTimezone() noexcept;

}; // class DateTimeWrapper

// ---------------------------------------------------------------------------
// Global instance alias  (mirrors the NVM / NVMWrapper convention)
// ---------------------------------------------------------------------------
extern DateTimeWrapper& DateTime;