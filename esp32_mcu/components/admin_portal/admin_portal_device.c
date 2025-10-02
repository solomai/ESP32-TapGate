#include "admin_portal_device.h"
#include "wifi_manager.h"
#include "nvm/nvm.h"
#include "logs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_wifi.h"

// NVM Partition to store admin portal data
#define NVM_ADMIN_PORTAL_PARTITION   NVM_PARTITION_DEFAULT

// Admin portal NVM namespace
#define ADMIN_PORTAL_NAMESPACE       "admin_portal"

// HTTP Session Idle timeout before auto logoff
#define ADMIN_PORTAL_KEY_IDLE_MIN    "admin-timeout"

// Default session Idle timeout in minutes
#define DEFAULT_IDLE_TIMEOUT_MINUTES 15U

static const char TAG[] = "HTTP Server";

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

char* create_json_from_ap_records()
{
    uint16_t ap_count = 0;
    wifi_ap_record_t ap_records[MAX_AP_NUM];
    
    // Get scan results from ESP-IDF
    esp_err_t err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    if (err != ESP_OK) {
        LOGE(TAG, "Failed to get AP records: %s", esp_err_to_name(err));
        return NULL;
    }
    
    if (ap_count == 0) {
        // Return empty JSON array
        char* result = malloc(3);
        if (result) {
            strcpy(result, "[]");
        }
        return result;
    }
    
    // Calculate buffer size needed for JSON
    // Each entry needs approximately: {"ssid":"<ssid>","chan":<chan>,"rssi":<rssi>,"auth":<auth>},
    // Maximum SSID length is 32 chars, so roughly 80 bytes per entry plus array brackets
    size_t buffer_size = ap_count * 120 + 10;
    char* json_buffer = malloc(buffer_size);
    if (!json_buffer) {
        LOGE(TAG, "Failed to allocate memory for JSON buffer");
        return NULL;
    }
    
    // Track seen SSIDs to filter duplicates
    char seen_ssids[MAX_AP_NUM][33]; // 32 chars + null terminator
    uint16_t seen_count = 0;
    
    // Start building JSON array
    strcpy(json_buffer, "[");
    bool first_entry = true;
    
    for (uint16_t i = 0; i < ap_count && i < MAX_AP_NUM; i++) {
        // Filter out empty SSIDs
        if (ap_records[i].ssid[0] == '\0') {
            continue;
        }
        
        // Check for duplicate SSIDs
        bool is_duplicate = false;
        for (uint16_t j = 0; j < seen_count; j++) {
            if (strcmp((char*)ap_records[i].ssid, seen_ssids[j]) == 0) {
                is_duplicate = true;
                break;
            }
        }
        
        if (is_duplicate) {
            continue;
        }
        
        // Add SSID to seen list
        if (seen_count < MAX_AP_NUM) {
            strncpy(seen_ssids[seen_count], (char*)ap_records[i].ssid, 32);
            seen_ssids[seen_count][32] = '\0';
            seen_count++;
        }
        
        // Add comma separator for subsequent entries
        if (!first_entry) {
            strcat(json_buffer, ",");
        }
        first_entry = false;
        
        // Escape SSID for JSON (basic escaping for quotes and backslashes)
        char escaped_ssid[66]; // 32 * 2 + 2 for worst case
        size_t escaped_len = 0;
        const char* ssid_ptr = (char*)ap_records[i].ssid;
        
        while (*ssid_ptr && escaped_len < sizeof(escaped_ssid) - 3) {
            if (*ssid_ptr == '"') {
                escaped_ssid[escaped_len++] = '\\';
                escaped_ssid[escaped_len++] = '"';
            } else if (*ssid_ptr == '\\') {
                escaped_ssid[escaped_len++] = '\\';
                escaped_ssid[escaped_len++] = '\\';
            } else {
                escaped_ssid[escaped_len++] = *ssid_ptr;
            }
            ssid_ptr++;
        }
        escaped_ssid[escaped_len] = '\0';
        
        // Build JSON entry
        char entry[150];
        snprintf(entry, sizeof(entry), 
                "{\"ssid\":\"%s\",\"chan\":%d,\"rssi\":%d,\"auth\":%d}",
                escaped_ssid,
                ap_records[i].primary,
                ap_records[i].rssi,
                ap_records[i].authmode);
        
        strcat(json_buffer, entry);
    }
    
    // Close JSON array
    strcat(json_buffer, "]");
    
    LOGI(TAG, "Generated JSON for %d WiFi access points", seen_count);
    return json_buffer;
}