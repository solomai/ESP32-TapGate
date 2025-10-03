#include "admin_portal_device.h"
#include "wifi_manager.h"
#include "nvm/nvm.h"
#include "logs.h"
#include <string.h>

// NVM Partition to store admin portal data
#define NVM_ADMIN_PORTAL_PARTITION   NVM_PARTITION_DEFAULT

// Admin portal NVM namespace
#define ADMIN_PORTAL_NAMESPACE       "admin_portal"

// HTTP Session Idle timeout before auto logoff
#define ADMIN_PORTAL_KEY_IDLE_MIN    "admin-timeout"

// Default session Idle timeout in minutes
#define DEFAULT_IDLE_TIMEOUT_MINUTES 15U

// Maximum number of concurrent WiFi page sessions
#define MAX_WIFI_PAGE_SESSIONS 4

static const char TAG[] = "HTTP Server";

// Structure to track active WiFi page sessions
typedef struct {
    char session_token[33]; // 32 chars + null terminator
    bool active;
} wifi_page_session_t;

// Array to track active WiFi page sessions
static wifi_page_session_t wifi_page_sessions[MAX_WIFI_PAGE_SESSIONS];
static bool wifi_callback_registered = false;

static size_t strnlen_safe(const char *str, size_t max_len)
{
    size_t len = 0;
    if (!str)
        return 0;
    while (len < max_len && str[len] != '\0')
        ++len;
    return len;
}

bool wifi_get_ap_ssid(char *ssid, size_t size)
{
    if (!ssid || size == 0) {
        LOGE(TAG, "Invalid argument wifi_get_ap_ssid");
        return false;
    }

    const char *source = (const char *)wifi_settings.ap_ssid;
    size_t length = strnlen_safe(source, sizeof(wifi_settings.ap_ssid));
    if (length >= size)
        length = size - 1;

    memcpy(ssid, source, length);
    ssid[length] = '\0';
    return true;
}

void wifi_set_ap_ssid(const char *ssid)
{
    if (!ssid){
        LOGE(TAG, "Invalid argument wifi_set_ap_ssid");
        return;
    }

    size_t length = strnlen_safe(ssid, sizeof(wifi_settings.ap_ssid));
    memcpy(wifi_settings.ap_ssid, ssid, length);
    if (length < sizeof(wifi_settings.ap_ssid))
        wifi_settings.ap_ssid[length] = '\0';

    // Store to NVM
    wifi_manager_save_config();
}

bool wifi_get_ap_password(char *password, size_t size)
{
    if (!password || size == 0) {
        LOGE(TAG, "Invalid argument wifi_get_ap_password");
        return false;
    }

    const char *source = (const char *)wifi_settings.ap_pwd;
    size_t length = strnlen_safe(source, sizeof(wifi_settings.ap_pwd));
    if (length >= size)
        length = size - 1;

    memcpy(password, source, length);
    password[length] = '\0';
    return false;
}

void wifi_set_ap_password(const char *password)
{
    if (!password) {
        LOGE(TAG, "Invalid argument wifi_set_ap_password");
        return;
    }

    size_t length = strnlen_safe(password, sizeof(wifi_settings.ap_pwd));
    memcpy(wifi_settings.ap_pwd, password, length);
    if (length < sizeof(wifi_settings.ap_pwd))
        wifi_settings.ap_pwd[length] = '\0';

    // Store to NVM
    wifi_manager_save_config();
}

uint32_t session_get_idle_timeout()
{
    uint32_t stored_minutes = DEFAULT_IDLE_TIMEOUT_MINUTES;
    esp_err_t err = nvm_read_u32(NVM_ADMIN_PORTAL_PARTITION, ADMIN_PORTAL_NAMESPACE, ADMIN_PORTAL_KEY_IDLE_MIN, &stored_minutes);
    if (err != ESP_OK)
    {
        stored_minutes = DEFAULT_IDLE_TIMEOUT_MINUTES;
        LOGW(TAG,"Failed to load idle timeout value. Used default value: %u minutes", stored_minutes);
    }
    return stored_minutes;
}

void session_set_idle_timeout(uint32_t timeout_value)
{
    esp_err_t err = nvm_write_u32(NVM_ADMIN_PORTAL_PARTITION, ADMIN_PORTAL_NAMESPACE, ADMIN_PORTAL_KEY_IDLE_MIN, timeout_value);
    if(err!=ESP_OK) {
        LOGE(TAG, "Failed saving %s to NVM \"%s\":\"%s\" error: %s",
             ADMIN_PORTAL_KEY_IDLE_MIN, NVM_ADMIN_PORTAL_PARTITION, ADMIN_PORTAL_NAMESPACE);
    }
}

void trigger_scan_wifi()
{
    wifi_manager_scan_async();
}

// WiFi scan completion callback
static void wifi_scan_done_callback(void* param)
{
    LOGI(TAG, "WiFi scan completed, checking for active WiFi page sessions");
    
    // Check if any WiFi page sessions are active
    bool has_active_sessions = false;
    for (int i = 0; i < MAX_WIFI_PAGE_SESSIONS; i++) {
        if (wifi_page_sessions[i].active) {
            has_active_sessions = true;
            LOGD(TAG, "Active WiFi session found: %.8s...", wifi_page_sessions[i].session_token);
        }
    }
    
    if (has_active_sessions) {
        admin_portal_notify_wifi_scan_complete();
    } else {
        LOGD(TAG, "No active WiFi page sessions, skipping notification");
    }
}

void admin_portal_register_wifi_page_session(const char* session_token)
{
    if (!session_token) {
        LOGE(TAG, "Invalid session token for WiFi page registration");
        return;
    }
    
    // Register callback if not already done
    if (!wifi_callback_registered) {
        esp_err_t err = wifi_manager_set_callback(WM_EVENT_SCAN_DONE, wifi_scan_done_callback);
        if (err == ESP_OK) {
            wifi_callback_registered = true;
            LOGI(TAG, "WiFi scan callback registered successfully");
        } else {
            LOGE(TAG, "Failed to register WiFi scan callback: %s", esp_err_to_name(err));
            return;
        }
    }
    
    // Find an empty slot or replace existing session
    for (int i = 0; i < MAX_WIFI_PAGE_SESSIONS; i++) {
        if (!wifi_page_sessions[i].active || 
            strcmp(wifi_page_sessions[i].session_token, session_token) == 0) {
            strncpy(wifi_page_sessions[i].session_token, session_token, sizeof(wifi_page_sessions[i].session_token) - 1);
            wifi_page_sessions[i].session_token[sizeof(wifi_page_sessions[i].session_token) - 1] = '\0';
            wifi_page_sessions[i].active = true;
            LOGD(TAG, "Registered WiFi page session: %.8s...", session_token);
            return;
        }
    }
    
    LOGW(TAG, "No available slots for WiFi page session registration");
}

void admin_portal_unregister_wifi_page_session(const char* session_token)
{
    if (!session_token) {
        return;
    }
    
    for (int i = 0; i < MAX_WIFI_PAGE_SESSIONS; i++) {
        if (wifi_page_sessions[i].active && 
            strcmp(wifi_page_sessions[i].session_token, session_token) == 0) {
            wifi_page_sessions[i].active = false;
            wifi_page_sessions[i].session_token[0] = '\0';
            LOGD(TAG, "Unregistered WiFi page session: %.8s...", session_token);
            return;
        }
    }
}

void admin_portal_notify_wifi_scan_complete()
{
    // This function will be called by the scan callback
    // For now, it just logs that the scan is complete
    // The web interface will use polling to get updated data
    LOGI(TAG, "WiFi scan completed - web clients should refresh network list");
}