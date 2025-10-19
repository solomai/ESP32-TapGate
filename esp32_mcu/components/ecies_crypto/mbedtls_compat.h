#pragma once

/**
 * @file mbedtls_compat.h
 * @brief Compatibility layer for mbedTLS 2.x vs 3.x API differences
 * 
 * ESP-IDF 5.5 uses mbedTLS 3.x which uses MBEDTLS_PRIVATE() macro to access
 * structure members. The host system may use mbedTLS 2.x which accesses
 * members directly. This header provides compatibility.
 */

#include <mbedtls/version.h>

/* 
 * Only define MBEDTLS_PRIVATE if not already defined by mbedtls headers.
 * In mbedTLS 3.x, private_access.h defines this macro.
 * In mbedTLS 2.x, we need to define it for compatibility.
 */
#ifndef MBEDTLS_PRIVATE
    #define MBEDTLS_PRIVATE(member) member
#endif
