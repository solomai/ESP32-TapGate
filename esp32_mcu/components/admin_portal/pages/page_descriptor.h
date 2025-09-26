#pragma once

#include "admin_portal_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *uri;
    admin_portal_page_t page;
    const uint8_t *data;
    size_t size;
    const char *content_type;
    bool compressed;
} admin_portal_page_descriptor_t;

const admin_portal_page_descriptor_t *admin_portal_page_auth_descriptor(void);
const admin_portal_page_descriptor_t *admin_portal_page_busy_descriptor(void);
const admin_portal_page_descriptor_t *admin_portal_page_change_psw_descriptor(void);
const admin_portal_page_descriptor_t *admin_portal_page_clients_descriptor(void);
const admin_portal_page_descriptor_t *admin_portal_page_device_descriptor(void);
const admin_portal_page_descriptor_t *admin_portal_page_enroll_descriptor(void);
const admin_portal_page_descriptor_t *admin_portal_page_events_descriptor(void);
const admin_portal_page_descriptor_t *admin_portal_page_main_descriptor(void);
const admin_portal_page_descriptor_t *admin_portal_page_off_descriptor(void);
const admin_portal_page_descriptor_t *admin_portal_page_wifi_descriptor(void);
