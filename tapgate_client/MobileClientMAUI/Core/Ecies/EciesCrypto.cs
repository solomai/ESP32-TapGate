using System;
using System.Security.Cryptography;
using NSec.Cryptography;

namespace TapGate.Core.Ecies
{
    /// <summary>
    /// ECIES (Elliptic Curve Integrated Encryption Scheme) implementation for .NET
    /// 
    /// Implements ECIES-style encryption using:
    /// - X25519 ECDH key exchange (RFC 7748) via NSec
    /// - HKDF-SHA256 key derivation (RFC 5869)
    /// - AES-256-GCM AEAD encryption (RFC 5116, NIST SP 800-38D)
    /// 
    /// Thread-safe, stateless operations compatible with ESP32 mbedTLS implementation.
    /// </summary>
    public static class EciesCrypto
    {
        /* Key and algorithm sizes (bytes) */
        public const int X25519_KEY_SIZE = 32;        // X25519 private/public key size
        public const int AES_KEY_SIZE = 32;           // AES-256 key size
        public const int GCM_IV_SIZE = 12;            // GCM nonce/IV size (96 bits recommended)
        public const int GCM_TAG_SIZE = 16;           // GCM authentication tag size (128 bits)
        
        /// <summary>
        /// Overhead size added to plaintext during encryption
        /// Encrypted packet structure: [Ephemeral Public Key (32)] [IV/Nonce (12)] [Ciphertext (N)] [Auth Tag (16)]
        /// </summary>
        public const int ENCRYPTION_OVERHEAD = X25519_KEY_SIZE + GCM_IV_SIZE + GCM_TAG_SIZE;
        
        // HKDF info string for domain separation (matches ESP32 implementation)
        private static readonly byte[] HKDF_INFO = System.Text.Encoding.ASCII.GetBytes("ECIES-AES256-GCM");
        
        // X25519 algorithm from NSec
        private static readonly KeyAgreementAlgorithm X25519Algorithm = KeyAgreementAlgorithm.X25519;
        
        /// <summary>
        /// Generate a new X25519 key pair
        /// Uses cryptographically secure RNG for key generation.
        /// Thread-safe, no internal state.
        /// </summary>
        /// <param name="privateKey">Buffer for private key (32 bytes)</param>
        /// <param name="publicKey">Buffer for public key (32 bytes)</param>
        /// <returns>True on success, false on failure</returns>
        public static bool GenerateKeyPair(byte[] privateKey, byte[] publicKey)
        {
            if (privateKey == null || publicKey == null)
                return false;
            
            if (privateKey.Length != X25519_KEY_SIZE || publicKey.Length != X25519_KEY_SIZE)
                return false;
            
            try
            {
                using var key = Key.Create(X25519Algorithm, new KeyCreationParameters { ExportPolicy = KeyExportPolicies.AllowPlaintextExport });
                
                // Export private key
                var privKeyBytes = key.Export(KeyBlobFormat.RawPrivateKey);
                Array.Copy(privKeyBytes, privateKey, X25519_KEY_SIZE);
                
                // Export public key
                var pubKeyBytes = key.PublicKey.Export(KeyBlobFormat.RawPublicKey);
                Array.Copy(pubKeyBytes, publicKey, X25519_KEY_SIZE);
                
                // Secure cleanup
                CryptographicOperations.ZeroMemory(privKeyBytes);
                
                return true;
            }
            catch
            {
                return false;
            }
        }
        
        /// <summary>
        /// Compute X25519 public key from private key
        /// Useful for deriving public key from stored private key.
        /// Thread-safe, stateless.
        /// </summary>
        /// <param name="privateKey">X25519 private key (32 bytes)</param>
        /// <param name="publicKey">X25519 public key (32 bytes)</param>
        /// <returns>True on success, false on failure</returns>
        public static bool ComputePublicKey(byte[] privateKey, byte[] publicKey)
        {
            if (privateKey == null || publicKey == null)
                return false;
            
            if (privateKey.Length != X25519_KEY_SIZE || publicKey.Length != X25519_KEY_SIZE)
                return false;
            
            try
            {
                using var key = Key.Import(X25519Algorithm, privateKey, KeyBlobFormat.RawPrivateKey, 
                    new KeyCreationParameters { ExportPolicy = KeyExportPolicies.AllowPlaintextExport });
                
                var pubKeyBytes = key.PublicKey.Export(KeyBlobFormat.RawPublicKey);
                Array.Copy(pubKeyBytes, publicKey, X25519_KEY_SIZE);
                
                return true;
            }
            catch
            {
                return false;
            }
        }
        
        /// <summary>
        /// Encrypt data using ECIES
        /// 
        /// Performs:
        /// 1. Generate ephemeral X25519 key pair
        /// 2. Compute shared secret via ECDH with recipient's public key
        /// 3. Derive AES-256 key using HKDF-SHA256
        /// 4. Encrypt plaintext with AES-256-GCM
        /// 5. Output: [ephemeral_pubkey || nonce || ciphertext || tag]
        /// 
        /// Thread-safe, stateless operation. Uses secure RNG for ephemeral key and nonce.
        /// </summary>
        /// <param name="plaintext">Input plaintext buffer</param>
        /// <param name="recipientPublicKey">Recipient's X25519 public key (32 bytes)</param>
        /// <param name="ciphertext">Output buffer (must be >= plaintext.Length + ENCRYPTION_OVERHEAD)</param>
        /// <returns>True on success, false on failure</returns>
        public static bool Encrypt(ReadOnlySpan<byte> plaintext, ReadOnlySpan<byte> recipientPublicKey, Span<byte> ciphertext)
        {
            if (recipientPublicKey.Length != X25519_KEY_SIZE)
                return false;
            
            if (ciphertext.Length < plaintext.Length + ENCRYPTION_OVERHEAD)
                return false;
            
            try
            {
                // Step 1: Generate ephemeral key pair
                Span<byte> ephemeralPrivateKey = stackalloc byte[X25519_KEY_SIZE];
                Span<byte> ephemeralPublicKey = stackalloc byte[X25519_KEY_SIZE];
                
                using var ephemeralKey = Key.Create(X25519Algorithm, new KeyCreationParameters { ExportPolicy = KeyExportPolicies.AllowPlaintextExport });
                var ephemPrivBytes = ephemeralKey.Export(KeyBlobFormat.RawPrivateKey);
                var ephemPubBytes = ephemeralKey.PublicKey.Export(KeyBlobFormat.RawPublicKey);
                
                ephemPrivBytes.CopyTo(ephemeralPrivateKey);
                ephemPubBytes.CopyTo(ephemeralPublicKey);
                
                // Copy ephemeral public key to output
                ephemeralPublicKey.CopyTo(ciphertext.Slice(0, X25519_KEY_SIZE));
                
                // Step 2 & 3: Perform ECDH and derive AES key
                Span<byte> aesKey = stackalloc byte[AES_KEY_SIZE];
                if (!PerformECDHandDeriveKey(ephemeralPrivateKey, recipientPublicKey, aesKey))
                {
                    CryptographicOperations.ZeroMemory(ephemeralPrivateKey);
                    CryptographicOperations.ZeroMemory(aesKey);
                    return false;
                }
                
                // Step 4: Generate random nonce
                Span<byte> nonce = ciphertext.Slice(X25519_KEY_SIZE, GCM_IV_SIZE);
                RandomNumberGenerator.Fill(nonce);
                
                // Step 5: Encrypt with AES-GCM
                var ciphertextData = ciphertext.Slice(X25519_KEY_SIZE + GCM_IV_SIZE, plaintext.Length);
                var tag = ciphertext.Slice(X25519_KEY_SIZE + GCM_IV_SIZE + plaintext.Length, GCM_TAG_SIZE);
                
                using var aesGcm = new AesGcm(aesKey, GCM_TAG_SIZE);
                aesGcm.Encrypt(nonce, plaintext, ciphertextData, tag);
                
                // Secure cleanup
                CryptographicOperations.ZeroMemory(ephemeralPrivateKey);
                CryptographicOperations.ZeroMemory(aesKey);
                CryptographicOperations.ZeroMemory(ephemPrivBytes);
                
                return true;
            }
            catch
            {
                return false;
            }
        }
        
        /// <summary>
        /// Encrypt data using ECIES (byte array version)
        /// </summary>
        public static bool Encrypt(byte[] plaintext, byte[] recipientPublicKey, out byte[] ciphertext)
        {
            ciphertext = new byte[plaintext.Length + ENCRYPTION_OVERHEAD];
            return Encrypt(plaintext.AsSpan(), recipientPublicKey.AsSpan(), ciphertext.AsSpan());
        }
        
        /// <summary>
        /// Decrypt data using ECIES
        /// 
        /// Performs:
        /// 1. Extract ephemeral public key from ciphertext
        /// 2. Compute shared secret using recipient's private key
        /// 3. Derive AES-256 key using HKDF-SHA256
        /// 4. Decrypt and verify using AES-256-GCM
        /// 
        /// Thread-safe, stateless operation.
        /// </summary>
        /// <param name="ciphertext">Input ciphertext (format: [ephemeral_pubkey || nonce || encrypted || tag])</param>
        /// <param name="recipientPrivateKey">Recipient's X25519 private key (32 bytes)</param>
        /// <param name="plaintext">Output plaintext buffer (must be >= ciphertext.Length - ENCRYPTION_OVERHEAD)</param>
        /// <returns>True on success, false on authentication failure or error</returns>
        public static bool Decrypt(ReadOnlySpan<byte> ciphertext, ReadOnlySpan<byte> recipientPrivateKey, Span<byte> plaintext)
        {
            if (recipientPrivateKey.Length != X25519_KEY_SIZE)
                return false;
            
            if (ciphertext.Length < ENCRYPTION_OVERHEAD)
                return false;
            
            var plaintextLength = ciphertext.Length - ENCRYPTION_OVERHEAD;
            if (plaintext.Length < plaintextLength)
                return false;
            
            try
            {
                // Step 1: Parse input: [ephemeral_pubkey || nonce || ciphertext || tag]
                var ephemeralPublicKey = ciphertext.Slice(0, X25519_KEY_SIZE);
                var nonce = ciphertext.Slice(X25519_KEY_SIZE, GCM_IV_SIZE);
                var encryptedData = ciphertext.Slice(X25519_KEY_SIZE + GCM_IV_SIZE, plaintextLength);
                var tag = ciphertext.Slice(X25519_KEY_SIZE + GCM_IV_SIZE + plaintextLength, GCM_TAG_SIZE);
                
                // Step 2 & 3: Perform ECDH and derive AES key
                Span<byte> aesKey = stackalloc byte[AES_KEY_SIZE];
                if (!PerformECDHandDeriveKey(recipientPrivateKey, ephemeralPublicKey, aesKey))
                {
                    CryptographicOperations.ZeroMemory(aesKey);
                    return false;
                }
                
                // Step 4: Decrypt and verify with AES-GCM
                using var aesGcm = new AesGcm(aesKey, GCM_TAG_SIZE);
                aesGcm.Decrypt(nonce, encryptedData, tag, plaintext.Slice(0, plaintextLength));
                
                // Secure cleanup
                CryptographicOperations.ZeroMemory(aesKey);
                
                return true;
            }
            catch (CryptographicException)
            {
                // Authentication failed or decryption error
                return false;
            }
            catch
            {
                return false;
            }
        }
        
        /// <summary>
        /// Decrypt data using ECIES (byte array version)
        /// </summary>
        public static bool Decrypt(byte[] ciphertext, byte[] recipientPrivateKey, out byte[] plaintext)
        {
            var plaintextLength = ciphertext.Length - ENCRYPTION_OVERHEAD;
            plaintext = new byte[plaintextLength];
            
            if (Decrypt(ciphertext.AsSpan(), recipientPrivateKey.AsSpan(), plaintext.AsSpan()))
            {
                return true;
            }
            
            plaintext = Array.Empty<byte>();
            return false;
        }
        
        /// <summary>
        /// Perform X25519 ECDH and derive AES key directly from shared secret
        /// Uses NSec's built-in KDF for deriving the AES key
        /// </summary>
        private static bool PerformECDHandDeriveKey(ReadOnlySpan<byte> ourPrivateKey, ReadOnlySpan<byte> theirPublicKey, Span<byte> aesKey)
        {
            try
            {
                using var privateKey = Key.Import(X25519Algorithm, ourPrivateKey, KeyBlobFormat.RawPrivateKey);
                var publicKey = NSec.Cryptography.PublicKey.Import(X25519Algorithm, theirPublicKey, KeyBlobFormat.RawPublicKey);
                
                using var secret = X25519Algorithm.Agree(privateKey, publicKey);
                if (secret == null)
                    return false;
                
                // Use HKDF-SHA256 to derive the AES key from the shared secret
                var hkdfAlgorithm = KeyDerivationAlgorithm.HkdfSha256;
                var derivedKeyBytes = hkdfAlgorithm.DeriveBytes(secret, ReadOnlySpan<byte>.Empty, HKDF_INFO, AES_KEY_SIZE);
                
                derivedKeyBytes.CopyTo(aesKey);
                CryptographicOperations.ZeroMemory(derivedKeyBytes);
                
                return true;
            }
            catch
            {
                return false;
            }
        }
        
        /// <summary>
        /// Securely erase sensitive data from memory
        /// Prevents compiler optimization from removing the zeroing operation.
        /// </summary>
        /// <param name="buffer">Buffer to clear</param>
        public static void SecureZero(Span<byte> buffer)
        {
            CryptographicOperations.ZeroMemory(buffer);
        }
    }
}
