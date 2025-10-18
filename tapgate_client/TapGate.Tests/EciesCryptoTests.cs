using System;
using System.Linq;
using TapGate.Core.Ecies;
using Xunit;

namespace TapGate.Tests
{
    /// <summary>
    /// Unit tests for EciesCrypto module
    /// Tests ECIES encryption/decryption functionality
    /// </summary>
    public class EciesCryptoTests
    {
        /// <summary>
        /// Test 1: Encryption–Decryption Between Two Parties
        /// 
        /// This test verifies that a message encrypted by the client can be 
        /// successfully decrypted by the host. The host generates a key pair,
        /// the client encrypts a plaintext message using the host's public key,
        /// and the host decrypts the ciphertext using its private key.
        /// The decrypted plaintext must match the original message.
        /// </summary>
        [Fact]
        public void EncryptDecrypt_TextMessage_ShouldSucceed()
        {
            // Arrange: Host generates key pair
            byte[] hostPrivateKey = new byte[EciesCrypto.X25519_KEY_SIZE];
            byte[] hostPublicKey = new byte[EciesCrypto.X25519_KEY_SIZE];
            
            bool keyGenSuccess = EciesCrypto.GenerateKeyPair(hostPrivateKey, hostPublicKey);
            Assert.True(keyGenSuccess, "Host key pair generation should succeed");
            
            // Client prepares a plaintext message
            string originalMessage = "Hello, ECIES encryption!";
            byte[] plaintext = System.Text.Encoding.UTF8.GetBytes(originalMessage);
            
            // Act: Client encrypts the message using host's public key
            byte[] ciphertext = new byte[plaintext.Length + EciesCrypto.ENCRYPTION_OVERHEAD];
            bool encryptSuccess = EciesCrypto.Encrypt(plaintext.AsSpan(), hostPublicKey.AsSpan(), ciphertext.AsSpan());
            Assert.True(encryptSuccess, "Encryption should succeed");
            Assert.NotNull(ciphertext);
            Assert.Equal(plaintext.Length + EciesCrypto.ENCRYPTION_OVERHEAD, ciphertext.Length);
            
            // Host decrypts the ciphertext using its private key
            byte[] decryptedPlaintext = new byte[plaintext.Length];
            bool decryptSuccess = EciesCrypto.Decrypt(ciphertext.AsSpan(), hostPrivateKey.AsSpan(), decryptedPlaintext.AsSpan());
            Assert.True(decryptSuccess, "Decryption should succeed");
            Assert.NotNull(decryptedPlaintext);
            
            // Assert: Decrypted plaintext must match the original message
            string decryptedMessage = System.Text.Encoding.UTF8.GetString(decryptedPlaintext);
            Assert.Equal(originalMessage, decryptedMessage);
            Assert.True(plaintext.SequenceEqual(decryptedPlaintext), 
                "Decrypted data must match original plaintext");
        }

        /// <summary>
        /// Test 2: Encryption of Binary Data
        /// 
        /// This test verifies that binary data can be encrypted and decrypted correctly.
        /// The host generates a key pair, the client generates a 512-byte binary buffer,
        /// encrypts it using the host's public key, and the host decrypts it using its
        /// private key. The decrypted buffer must match the original binary data.
        /// </summary>
        [Fact]
        public void EncryptDecrypt_BinaryData_ShouldSucceed()
        {
            // Arrange: Host generates key pair
            byte[] hostPrivateKey = new byte[EciesCrypto.X25519_KEY_SIZE];
            byte[] hostPublicKey = new byte[EciesCrypto.X25519_KEY_SIZE];
            
            bool keyGenSuccess = EciesCrypto.GenerateKeyPair(hostPrivateKey, hostPublicKey);
            Assert.True(keyGenSuccess, "Host key pair generation should succeed");
            
            // Client generates a 512-byte binary buffer with pseudo-random data
            const int bufferSize = 512;
            byte[] originalBinaryData = new byte[bufferSize];
            Random random = new Random(42); // Fixed seed for reproducibility
            random.NextBytes(originalBinaryData);
            
            // Act: Client encrypts the binary data using host's public key
            byte[] ciphertext = new byte[bufferSize + EciesCrypto.ENCRYPTION_OVERHEAD];
            bool encryptSuccess = EciesCrypto.Encrypt(originalBinaryData.AsSpan(), hostPublicKey.AsSpan(), ciphertext.AsSpan());
            Assert.True(encryptSuccess, "Encryption should succeed");
            Assert.NotNull(ciphertext);
            Assert.Equal(bufferSize + EciesCrypto.ENCRYPTION_OVERHEAD, ciphertext.Length);
            
            // Host decrypts the ciphertext using its private key
            byte[] decryptedBinaryData = new byte[bufferSize];
            bool decryptSuccess = EciesCrypto.Decrypt(ciphertext.AsSpan(), hostPrivateKey.AsSpan(), decryptedBinaryData.AsSpan());
            Assert.True(decryptSuccess, "Decryption should succeed");
            Assert.NotNull(decryptedBinaryData);
            Assert.Equal(bufferSize, decryptedBinaryData.Length);
            
            // Assert: Decrypted buffer must match the original binary data
            Assert.True(originalBinaryData.SequenceEqual(decryptedBinaryData),
                "Decrypted binary data must match original binary data");
            
            // Verify all bytes match
            for (int i = 0; i < bufferSize; i++)
            {
                Assert.Equal(originalBinaryData[i], decryptedBinaryData[i]);
            }
        }
    }
}
