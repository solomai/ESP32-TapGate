using System;

namespace TapGate.Core
{
    /// <summary>
    /// CRC-32 (IEEE 802.3 standard) checksum calculation for binary data.
    /// Uses the reflected (LE/LSB-first) variant with polynomial 0x04C11DB7 (reflected form 0xEDB88320).
    /// 
    /// Compatible with ESP-IDF's esp_crc32_le() function for cross-platform consistency.
    /// Test vector: "123456789" ? CRC = 0xCBF43926
    /// 
    /// Algorithm parameters:
    /// - Polynomial: 0x04C11DB7 (reflected form 0xEDB88320)
    /// - Init value: 0xFFFFFFFF
    /// - Final XOR value: 0xFFFFFFFF
    /// - refin = true, refout = true
    /// </summary>
    public static class Crc32
    {
        /// <summary>
        /// Reflected CRC-32 polynomial (0xEDB88320)
        /// </summary>
        private const uint Polynomial = 0xEDB88320;

        /// <summary>
        /// Initial CRC value
        /// </summary>
        private const uint InitValue = 0xFFFFFFFF;

        /// <summary>
        /// Final XOR value applied to result
        /// </summary>
        private const uint FinalXor = 0xFFFFFFFF;

        /// <summary>
        /// Precomputed 256-element lookup table for CRC-32 calculation
        /// </summary>
        private static readonly uint[] Table = GenerateTable();

        /// <summary>
        /// Thread-local state for incremental CRC calculation
        /// </summary>
        [ThreadStatic]
        private static uint _state = InitValue;

        /// <summary>
        /// Generates the CRC-32 lookup table using the reflected polynomial
        /// </summary>
        private static uint[] GenerateTable()
        {
            uint[] table = new uint[256];
            
            for (uint i = 0; i < 256; i++)
            {
                uint crc = i;
                for (int j = 0; j < 8; j++)
                {
                    if ((crc & 1) != 0)
                    {
                        crc = (crc >> 1) ^ Polynomial;
                    }
                    else
                    {
                        crc >>= 1;
                    }
                }
                table[i] = crc;
            }
            
            return table;
        }

        /// <summary>
        /// Computes CRC-32 checksum for the entire data block
        /// </summary>
        /// <param name="data">Input data to calculate CRC for</param>
        /// <returns>CRC-32 checksum value</returns>
        public static uint Compute(ReadOnlySpan<byte> data)
        {
            return Update(InitValue, data) ^ FinalXor;
        }

        /// <summary>
        /// Updates CRC-32 state with a new data chunk (for incremental calculation)
        /// </summary>
        /// <param name="state">Current CRC state (use InitValue for first chunk)</param>
        /// <param name="chunk">Data chunk to process</param>
        /// <returns>Updated CRC state (apply FinalXor only at the end)</returns>
        public static uint Update(uint state, ReadOnlySpan<byte> chunk)
        {
            uint crc = state;
            
            for (int i = 0; i < chunk.Length; i++)
            {
                byte index = (byte)((crc ^ chunk[i]) & 0xFF);
                crc = (crc >> 8) ^ Table[index];
            }
            
            return crc;
        }

        /// <summary>
        /// Resets the thread-local CRC state to initial value
        /// </summary>
        public static void Reset()
        {
            _state = InitValue;
        }

        /// <summary>
        /// Gets the current thread-local CRC state
        /// </summary>
        public static uint State => _state;

        /// <summary>
        /// Updates the thread-local CRC state with new data
        /// </summary>
        /// <param name="chunk">Data chunk to process</param>
        public static void UpdateState(ReadOnlySpan<byte> chunk)
        {
            _state = Update(_state, chunk);
        }

        /// <summary>
        /// Finalizes the thread-local CRC calculation and returns the result
        /// </summary>
        /// <returns>Final CRC-32 checksum value</returns>
        public static uint Finalize()
        {
            return _state ^ FinalXor;
        }
    }
}
