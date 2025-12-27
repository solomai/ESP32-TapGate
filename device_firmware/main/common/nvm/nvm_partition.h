#pragma once

// keep in sync with partitions.csv

// NVM partition labels
constexpr const char* NVM_PARTITION_DEFAULT = "nvs";
constexpr const char* NVM_PARTITION_CLIENTS = "nvs_entity";
constexpr const char* NVM_PARTITION_NONCE   = "nvs_nonce";

// Table of all NVM partition labels
constexpr const char* NVM_PARTITION_LABELS[] = {
    NVM_PARTITION_DEFAULT,
    NVM_PARTITION_CLIENTS,
    NVM_PARTITION_NONCE
};