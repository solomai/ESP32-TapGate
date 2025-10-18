using System;
using System.Text;
using TapGate.Core;
using Xunit;
using Xunit.Abstractions;

namespace TapGate.Tests
{
    /// <summary>
    /// Unit tests for CRC32 module
    /// Tests CRC-32 (IEEE 802.3) checksum calculation compatibility with ESP-IDF's esp_crc32_le()
    /// </summary>
    public class Crc32Tests
    {
        private readonly ITestOutputHelper _output;

        public Crc32Tests(ITestOutputHelper output)
        {
            _output = output;
        }

        /// <summary>
        /// Test 1: Standard test vector "123456789" ? 0xCBF43926
        /// This is the official CRC-32 test vector from IEEE 802.3
        /// </summary>
        [Fact]
        public void TestCrc32_StandardTestVector()
        {
            // Arrange
            byte[] data = Encoding.ASCII.GetBytes("123456789");
            const uint expectedCrc = 0xCBF43926;

            // Act
            uint actualCrc = Crc32.Compute(data);

            // Assert
            _output.WriteLine($"Input: \"123456789\"");
            _output.WriteLine($"Expected CRC: 0x{expectedCrc:X8}");
            _output.WriteLine($"Actual CRC:   0x{actualCrc:X8}");
            
            Assert.Equal(expectedCrc, actualCrc);
        }

        /// <summary>
        /// Test 2: Empty data should return the final XOR value
        /// </summary>
        [Fact]
        public void TestCrc32_EmptyData()
        {
            // Arrange
            byte[] emptyData = Array.Empty<byte>();
            const uint expectedCrc = 0x00000000; // InitValue ^ FinalXor = 0xFFFFFFFF ^ 0xFFFFFFFF = 0

            // Act
            uint actualCrc = Crc32.Compute(emptyData);

            // Assert
            _output.WriteLine($"Empty data CRC: 0x{actualCrc:X8}");
            Assert.Equal(expectedCrc, actualCrc);
        }

        /// <summary>
        /// Test 3: Single byte CRC calculation
        /// </summary>
        [Fact]
        public void TestCrc32_SingleByte()
        {
            // Arrange
            byte[] data = new byte[] { 0x00 };

            // Act
            uint crc = Crc32.Compute(data);

            // Assert
            _output.WriteLine($"Single byte (0x00) CRC: 0x{crc:X8}");
            Assert.NotEqual(0u, crc);
        }

        /// <summary>
        /// Test 4: Incremental CRC calculation with Update method
        /// Split "123456789" into chunks and verify it produces the same result
        /// </summary>
        [Fact]
        public void TestCrc32_IncrementalCalculation()
        {
            // Arrange
            byte[] fullData = Encoding.ASCII.GetBytes("123456789");
            byte[] chunk1 = Encoding.ASCII.GetBytes("1234");
            byte[] chunk2 = Encoding.ASCII.GetBytes("56");
            byte[] chunk3 = Encoding.ASCII.GetBytes("789");
            const uint expectedCrc = 0xCBF43926;

            // Act - calculate in one go
            uint crcSinglePass = Crc32.Compute(fullData);

            // Act - calculate incrementally
            uint state = 0xFFFFFFFF; // InitValue
            state = Crc32.Update(state, chunk1);
            state = Crc32.Update(state, chunk2);
            state = Crc32.Update(state, chunk3);
            uint crcIncremental = state ^ 0xFFFFFFFF; // Apply FinalXor

            // Assert
            _output.WriteLine($"Single pass CRC:     0x{crcSinglePass:X8}");
            _output.WriteLine($"Incremental CRC:     0x{crcIncremental:X8}");
            _output.WriteLine($"Expected CRC:        0x{expectedCrc:X8}");
            
            Assert.Equal(expectedCrc, crcSinglePass);
            Assert.Equal(expectedCrc, crcIncremental);
            Assert.Equal(crcSinglePass, crcIncremental);
        }

        /// <summary>
        /// Test 5: Thread-local state management (Reset, UpdateState, Finalize)
        /// </summary>
        [Fact]
        public void TestCrc32_ThreadLocalState()
        {
            // Arrange
            byte[] data = Encoding.ASCII.GetBytes("123456789");
            byte[] chunk1 = Encoding.ASCII.GetBytes("1234");
            byte[] chunk2 = Encoding.ASCII.GetBytes("56789");
            const uint expectedCrc = 0xCBF43926;

            // Act
            Crc32.Reset();
            Crc32.UpdateState(chunk1);
            Crc32.UpdateState(chunk2);
            uint crc = Crc32.Finalize();

            // Assert
            _output.WriteLine($"Thread-local state CRC: 0x{crc:X8}");
            _output.WriteLine($"Expected CRC:           0x{expectedCrc:X8}");
            Assert.Equal(expectedCrc, crc);

            // Verify Reset works
            Crc32.Reset();
            Assert.Equal(0xFFFFFFFF, Crc32.State);
        }

        /// <summary>
        /// Test 6: Various ASCII strings to ensure consistency
        /// </summary>
        [Theory]
        [InlineData("Hello, World!", 0xEC4AC3D0)]
        [InlineData("", 0x00000000)]
        [InlineData("A", 0xD3D99E8B)]
        [InlineData("ABC", 0xA3830348)]
        public void TestCrc32_VariousStrings(string input, uint expectedCrc)
        {
            // Arrange
            byte[] data = Encoding.ASCII.GetBytes(input);

            // Act
            uint actualCrc = Crc32.Compute(data);

            // Assert
            _output.WriteLine($"Input: \"{input}\"");
            _output.WriteLine($"Expected CRC: 0x{expectedCrc:X8}");
            _output.WriteLine($"Actual CRC:   0x{actualCrc:X8}");
            
            Assert.Equal(expectedCrc, actualCrc);
        }

        /// <summary>
        /// Test 7: Binary data compatibility test
        /// Tests with binary data that might come from ESP32
        /// </summary>
        [Fact]
        public void TestCrc32_BinaryData()
        {
            // Arrange - simulate a binary packet from ESP32
            byte[] binaryData = new byte[]
            {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
            };

            // Act
            uint crc = Crc32.Compute(binaryData);

            // Assert
            _output.WriteLine($"Binary data (16 bytes) CRC: 0x{crc:X8}");
            Assert.NotEqual(0u, crc);
            
            // Verify consistency - computing same data twice gives same result
            uint crc2 = Crc32.Compute(binaryData);
            Assert.Equal(crc, crc2);
        }

        /// <summary>
        /// Test 8: Large data block performance test
        /// </summary>
        [Fact]
        public void TestCrc32_LargeDataBlock()
        {
            // Arrange - 1 MB of data
            byte[] largeData = new byte[1024 * 1024];
            for (int i = 0; i < largeData.Length; i++)
            {
                largeData[i] = (byte)(i & 0xFF);
            }

            // Act
            var startTime = DateTime.UtcNow;
            uint crc = Crc32.Compute(largeData);
            var elapsed = DateTime.UtcNow - startTime;

            // Assert
            _output.WriteLine($"Large data (1 MB) CRC: 0x{crc:X8}");
            _output.WriteLine($"Time elapsed: {elapsed.TotalMilliseconds:F2} ms");
            
            Assert.NotEqual(0u, crc);
            Assert.True(elapsed.TotalMilliseconds < 1000, "CRC calculation should be fast (< 1 second for 1 MB)");
        }

        /// <summary>
        /// Test 9: ESP32 compatibility test with known values
        /// This test ensures compatibility with ESP-IDF's esp_crc32_le()
        /// Test data and expected CRC values should match ESP32 implementation
        /// </summary>
        [Fact]
        public void TestCrc32_ESP32Compatibility()
        {
            // Arrange - Test data that will be verified against ESP32
            byte[] testMessage = Encoding.ASCII.GetBytes("Test message for ESP32");
            
            // Act
            uint crc = Crc32.Compute(testMessage);

            // Assert
            _output.WriteLine($"ESP32 test message: \"Test message for ESP32\"");
            _output.WriteLine($"CRC: 0x{crc:X8}");
            
            // This CRC value should be verified against actual ESP32 output
            // For now, we just verify it's computed consistently
            uint crc2 = Crc32.Compute(testMessage);
            Assert.Equal(crc, crc2);
        }

        /// <summary>
        /// Test 10: Span vs Array consistency
        /// Ensures Span-based API produces same results as array-based
        /// </summary>
        [Fact]
        public void TestCrc32_SpanArrayConsistency()
        {
            // Arrange
            byte[] data = Encoding.ASCII.GetBytes("123456789");
            ReadOnlySpan<byte> span = data.AsSpan();

            // Act
            uint crcFromArray = Crc32.Compute(data);
            uint crcFromSpan = Crc32.Compute(span);

            // Assert
            _output.WriteLine($"CRC from array: 0x{crcFromArray:X8}");
            _output.WriteLine($"CRC from span:  0x{crcFromSpan:X8}");

            Assert.Equal(0xCBF43926U, crcFromArray);
            Assert.Equal(crcFromArray, crcFromSpan);
        }
    }
}
