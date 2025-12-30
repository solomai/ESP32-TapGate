#pragma once

// keep in sync with partitions.csv

// NVM partition labels
constexpr const char* NVM_PARTITION_DEFAULT = "nvs";
constexpr const char* NVM_PARTITION_ENTITY  = "nvs_entity";
constexpr const char* NVM_PARTITION_NONCE   = "nvs_nonce";

// Aliases
constexpr const char* NVM_PARTITION_CTXDEVICE = NVM_PARTITION_ENTITY;

// Table of all NVM partition labels
constexpr const char* NVM_PARTITION_LABELS[] = {
    NVM_PARTITION_DEFAULT,
    NVM_PARTITION_ENTITY,
    NVM_PARTITION_NONCE
};