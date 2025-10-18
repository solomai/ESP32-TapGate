/**
 * @file crc32.h
 * @brief CRC-32 (IEEE 802.3) checksum calculation module
 * 
 * This module implements CRC-32 checksum calculation using the IEEE 802.3 standard
 * polynomial 0x04C11DB7 (reflected form 0xEDB88320).
 * 
 * The implementation uses reflected bit order (LSB-first) as per the standard.
 * - Polynomial: 0x04C11DB7 (reflected: 0xEDB88320)
 * - Init value: 0xFFFFFFFF
 * - Final XOR: 0xFFFFFFFF
 * - Reflects input bytes and final CRC
 * 
 * Reference test: CRC32("123456789") = 0xCBF43926
 */

#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize CRC32 calculation with a seed value
 * 
 * @param seed Initial CRC value (typically 0xFFFFFFFF for standard CRC32)
 * @return Initialized CRC value
 */
uint32_t crc32_init(uint32_t seed);

/**
 * @brief Update CRC32 with new data
 * 
 * @param crc Current CRC value (from crc32_init or previous crc32_update)
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return Updated CRC value
 */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len);

/**
 * @brief Finalize CRC32 calculation
 * 
 * @param crc Current CRC value
 * @return Final CRC32 checksum
 */
uint32_t crc32_finalize(uint32_t crc);

/**
 * @brief Calculate CRC32 for a complete data buffer (convenience function)
 * 
 * This is a convenience wrapper that calls init, update, and finalize.
 * 
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return CRC32 checksum
 */
uint32_t crc32_calculate(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC32_H */