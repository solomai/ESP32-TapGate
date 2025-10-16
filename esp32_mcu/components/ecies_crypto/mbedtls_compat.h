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

/* Check mbedTLS version */
#if MBEDTLS_VERSION_MAJOR >= 3
    /* mbedTLS 3.x - Define MBEDTLS_PRIVATE macro if not already defined */
    #ifndef MBEDTLS_PRIVATE
        #define MBEDTLS_PRIVATE(member) member
    #endif
#else
    /* mbedTLS 2.x - Define MBEDTLS_PRIVATE as direct access */
    #ifndef MBEDTLS_PRIVATE
        #define MBEDTLS_PRIVATE(member) member
    #endif
#endif
