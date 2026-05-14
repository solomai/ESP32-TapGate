//
// DateTimeWrapper - System date/time management
//

#include "datetime.h"

#include "esp_log.h"

#include <cstring>
#include <cerrno>

static const char* TAG_DATETIME = "DateTime";

// ---------------------------------------------------------------------------
// Singleton + global alias
// ---------------------------------------------------------------------------

DateTimeWrapper& DateTimeWrapper::getInstance() noexcept
{
    static DateTimeWrapper instance;
    return instance;
}

DateTimeWrapper& DateTime = DateTimeWrapper::getInstance();

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

esp_err_t DateTimeWrapper::Init() noexcept
{
    ESP_LOGI(TAG_DATETIME, "Initializing with timezone \"%s\"", m_tz_string);

    const esp_err_t err = ApplyTimezone();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_DATETIME, "Failed to apply timezone \"%s\"", m_tz_string);
        return err;
    }

    // Extension point: SNTP driver init will be invoked here.

    ESP_LOGI(TAG_DATETIME, "Initialized (sync=%s)", m_time_synchronized ? "yes" : "no");
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Set
// ---------------------------------------------------------------------------

esp_err_t DateTimeWrapper::SetTime(time_t epoch_sec, suseconds_t epoch_usec) noexcept
{
    const struct timeval tv { .tv_sec = epoch_sec, .tv_usec = epoch_usec };

    if (settimeofday(&tv, nullptr) != 0)
    {
        ESP_LOGE(TAG_DATETIME, "settimeofday failed: errno=%d", errno);
        return ESP_FAIL;
    }

    // Extension point: if SNTP is running, notify the sync driver that the
    // time has been manually overridden (e.g., cancel a pending correction).
    m_time_synchronized = true;

    ESP_LOGI(TAG_DATETIME, "Time set to epoch %lld.%06ld", static_cast<long long>(epoch_sec),
             static_cast<long>(epoch_usec));
    return ESP_OK;
}

esp_err_t DateTimeWrapper::SetTime(const struct tm& tm) noexcept
{
    // mktime() interprets the struct in the local timezone currently active.
    struct tm mutable_tm = tm;
    const time_t epoch = mktime(&mutable_tm);

    if (epoch == static_cast<time_t>(-1))
    {
        ESP_LOGE(TAG_DATETIME, "mktime failed: invalid struct tm");
        return ESP_FAIL;
    }

    return SetTime(epoch);
}

esp_err_t DateTimeWrapper::SetTimezone(const char* tz_posix) noexcept
{
    if (!tz_posix)
    {
        ESP_LOGE(TAG_DATETIME, "SetTimezone: null TZ string");
        return ESP_ERR_INVALID_ARG;
    }

    const size_t len = std::strlen(tz_posix);
    if (len >= TZ_STRING_MAX_LEN)
    {
        ESP_LOGE(TAG_DATETIME, "SetTimezone: TZ string too long (%zu >= %zu)", len, TZ_STRING_MAX_LEN);
        return ESP_ERR_INVALID_ARG;
    }

    std::memcpy(m_tz_string, tz_posix, len + 1u); // +1 for null terminator

    const esp_err_t err = ApplyTimezone();
    if (err != ESP_OK)
    {
        return err;
    }

    ESP_LOGI(TAG_DATETIME, "Timezone set to \"%s\"", m_tz_string);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Get
// ---------------------------------------------------------------------------

esp_err_t DateTimeWrapper::GetTimeVal(struct timeval& tv) const noexcept
{
    if (gettimeofday(&tv, nullptr) != 0)
    {
        ESP_LOGE(TAG_DATETIME, "gettimeofday failed: errno=%d", errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t DateTimeWrapper::GetLocalTime(struct tm& out_tm) const noexcept
{
    struct timeval tv{};
    const esp_err_t err = GetTimeVal(tv);
    if (err != ESP_OK)
    {
        return err;
    }

    if (localtime_r(&tv.tv_sec, &out_tm) == nullptr)
    {
        ESP_LOGE(TAG_DATETIME, "localtime_r failed: errno=%d", errno);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t DateTimeWrapper::GetUtcTime(struct tm& out_tm) const noexcept
{
    struct timeval tv{};
    const esp_err_t err = GetTimeVal(tv);
    if (err != ESP_OK)
    {
        return err;
    }

    if (gmtime_r(&tv.tv_sec, &out_tm) == nullptr)
    {
        ESP_LOGE(TAG_DATETIME, "gmtime_r failed: errno=%d", errno);
        return ESP_FAIL;
    }

    return ESP_OK;
}

std::string_view DateTimeWrapper::GetTimezone() const noexcept
{
    return std::string_view{ m_tz_string };
}

// ---------------------------------------------------------------------------
// Formatting
// ---------------------------------------------------------------------------

size_t DateTimeWrapper::Format(char* buffer, size_t buffer_size, const char* fmt) const noexcept
{
    if (!buffer || buffer_size == 0u || !fmt)
    {
        return 0u;
    }

    buffer[0] = '\0';

    struct tm local_tm{};
    if (GetLocalTime(local_tm) != ESP_OK)
    {
        return 0u;
    }

    return Format(buffer, buffer_size, fmt, local_tm);
}

/* static */
size_t DateTimeWrapper::Format(char* buffer,
                               size_t buffer_size,
                               const char* fmt,
                               const struct tm& tm) noexcept
{
    if (!buffer || buffer_size == 0u || !fmt)
    {
        return 0u;
    }

    buffer[0] = '\0';

    // strftime returns 0 both on error and when the output is empty.
    // An empty format string is a valid (if unusual) call.
    const size_t written = strftime(buffer, buffer_size, fmt, &tm);
    return written;
}

// ---------------------------------------------------------------------------
// Sync state
// ---------------------------------------------------------------------------

bool DateTimeWrapper::IsTimeSynchronized() const noexcept
{
    return m_time_synchronized;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

esp_err_t DateTimeWrapper::ApplyTimezone() noexcept
{
    // setenv + tzset is the official ESP-IDF mechanism for setting the
    // timezone at runtime (mirrors the POSIX / Linux approach documented
    // in the ESP-IDF System Time reference).
    if (setenv("TZ", m_tz_string, 1 /* overwrite */) != 0)
    {
        ESP_LOGE(TAG_DATETIME, "setenv(\"TZ\") failed: errno=%d", errno);
        return ESP_FAIL;
    }

    tzset(); // Applies the TZ change to the C runtime immediately.
    return ESP_OK;
}