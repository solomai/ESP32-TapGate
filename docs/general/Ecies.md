# 🔐 ECIES - Elliptic Curve Integrated Encryption Scheme

## Overview

ECIES is a hybrid public-key encryption scheme that combines elliptic curve cryptography for key exchange with symmetric encryption for data protection.  
It provides confidentiality, integrity, and authenticity by generating an ephemeral key pair for each encryption operation, deriving a shared secret through Elliptic Curve Diffie–Hellman (ECDH), and then using that secret as input to a Key Derivation Function (KDF) to produce symmetric keys.  
These keys are typically used with AES in GCM mode, ensuring both encryption and authentication of the data.  
ECIES is widely used in constrained environments and modern security frameworks due to its compact key size, strong security guarantees, and efficiency on embedded platforms like ESP32.

## Encrypted Packet Structure

ECIES combines elliptic-curve key exchange with symmetric authenticated encryption.
Each encryption operation generates a new ephemeral key pair, derives a shared secret via X25519 (ECDH), and then uses it to create a 256-bit AES key for symmetric encryption in GCM mode.
GCM provides both confidentiality and authentication using a 96-bit IV (nonce) and a 128-bit authentication tag.

Structure:
```text
[Ephemeral Public Key (32)] [IV/Nonce (12)] [Ciphertext (N)] [Auth Tag (16)]
```
- Ephemeral Public Key – temporary X25519 public key, unique per message
- IV / Nonce – 96-bit initialization vector used by AES-GCM
- Ciphertext – encrypted payload of variable length
- Auth Tag – 128-bit authentication tag ensuring integrity and authenticity

This layout adds a fixed 60-byte overhead to the plaintext size while maintaining high security and compactness, suitable for constrained devices such as the ESP32.


## Implementation Comparison: ESP32 ( IDF 5.5) vs .NET MAUI (.NET 9)

### ESP32 Implementation (C/mbedTLS)

| Component | Technology | Library |
|-----------|-----------|---------|
| X25519 ECDH | mbedTLS ECP API | `mbedtls/ecp.h` |
| HKDF-SHA256 | mbedTLS HKDF | `mbedtls/hkdf.h` |
| AES-256-GCM | mbedTLS GCM | `mbedtls/gcm.h` |
| RNG | ESP32 Hardware RNG | `esp_random()` |
| Memory Cleanup | mbedTLS Platform Utils | `mbedtls_platform_zeroize()` |

### .NET MAUI Implementation (C#)

| Component | Technology | Library |
|-----------|-----------|---------|
| X25519 ECDH | NSec/libsodium | `NSec.Cryptography` |
| HKDF-SHA256 | .NET BCL | `System.Security.Cryptography.HKDF` |
| AES-256-GCM | .NET BCL | `System.Security.Cryptography.AesGcm` |
| RNG | Platform CSPRNG | `RandomNumberGenerator` |
| Memory Cleanup | .NET BCL | `CryptographicOperations.ZeroMemory()` |

## API Surface Comparison - Function Signatures

### ESP32 (C)

```c
bool ecies_generate_keypair(uint8_t private_key[32], uint8_t public_key[32]);
bool ecies_compute_public_key(const uint8_t private_key[32], uint8_t public_key[32]);
bool ecies_encrypt(const uint8_t *plaintext, size_t plaintext_len,
                   const uint8_t recipient_pubkey[32],
                   uint8_t *ciphertext, size_t *ciphertext_len);
bool ecies_decrypt(const uint8_t *ciphertext, size_t ciphertext_len,
                   const uint8_t recipient_privkey[32],
                   uint8_t *plaintext, size_t *plaintext_len);
void ecies_secure_zero(void *buffer, size_t size);
```

### .NET MAUI (C#)

```csharp
bool GenerateKeyPair(byte[] privateKey, byte[] publicKey);
bool ComputePublicKey(byte[] privateKey, byte[] publicKey);
bool Encrypt(ReadOnlySpan<byte> plaintext, ReadOnlySpan<byte> recipientPublicKey, 
             Span<byte> ciphertext);
bool Encrypt(byte[] plaintext, byte[] recipientPublicKey, out byte[] ciphertext);
bool Decrypt(ReadOnlySpan<byte> ciphertext, ReadOnlySpan<byte> recipientPrivateKey, 
             Span<byte> plaintext);
bool Decrypt(byte[] ciphertext, byte[] recipientPrivateKey, out byte[] plaintext);
void SecureZero(Span<byte> buffer);
```

## Performance Characteristics

### ESP32 (ESP-IDF 5.5, ESP32-S3 @ 240MHz)

| Operation | Typical Time |
|-----------|--------------|
| Key Generation | ~5-10 ms |
| ECDH | ~5-8 ms |
| HKDF | ~0.5 ms |
| AES-GCM Encrypt (512 bytes) | ~1-2 ms |
| Total Encrypt | ~12-20 ms |
| Total Decrypt | ~10-15 ms |

### .NET MAUI (Release, .NET 9)

| Platform | Key Gen | Encrypt (512B) | Decrypt (512B) |
|----------|---------|----------------|----------------|
| Android (Snapdragon 8 Gen 2) | < 1 ms | < 2 ms | < 2 ms |
| iOS (A16 Bionic) | < 1 ms | < 1.5 ms | < 1.5 ms |
| Windows (Intel i7-12700) | < 0.5 ms | < 1 ms | < 1 ms |
| macOS (M2 Pro) | < 0.5 ms | < 1 ms | < 1 ms |

*Note: .NET implementations benefit from hardware AES acceleration and optimized native libraries*

## Platform Support Matrix

### ESP32

| Platform | Support |
|----------|---------|
| ESP32    | ✅ |
| ESP32-S2 | ✅ |
| ESP32-S3 | ✅ |
| ESP32-C3 | ✅ |
| ESP32-C6 | ✅ |
| ESP32-H2 | ✅ |

### .NET MAUI

| Platform       | Architecture    | Support         |
|----------------|-----------------|-----------------|
| Android        | ARM64, ARM, x64 | ✅ API 26+      |
| iOS            | ARM64           | ✅ iOS 15.0+    |
| macOS Catalyst | ARM64, x64      | ✅ macOS 12.0+  |
| Windows        | x64, ARM64      | ✅ Win10 19041+ |

## Dependencies

### ESP32 Build Dependencies

```
esp-idf (5.5+)
├── mbedtls (3.6.0)
├── esp_random
├── esp_log
└── freertos
```

### .NET MAUI Build Dependencies

```
.NET 9 SDK
├── System.Security.Cryptography (built-in)
├── NSec.Cryptography (25.4.0+)
│   └── libsodium (1.0.20.1)
└── Microsoft.Maui.Controls (9.0+)
```

## References

- [RFC 7748 - Elliptic Curves for Security (X25519)](https://datatracker.ietf.org/doc/html/rfc7748)
- [RFC 5869 - HMAC-based Extract-and-Expand Key Derivation Function (HKDF)](https://datatracker.ietf.org/doc/html/rfc5869)
- [RFC 5116 - An Interface and Algorithms for Authenticated Encryption](https://datatracker.ietf.org/doc/html/rfc5116)
- [NIST SP 800-38D - Galois/Counter Mode (GCM)](https://csrc.nist.gov/pubs/sp/800/38/d/final)
- [NSec Documentation](https://nsec.rocks/)
- [ESP-IDF mbedTLS Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mbedtls.html)

---

[← Back to ESP32 MCU Documentation](../../esp32_mcu/README.md)  
[← Back to Mobile Client Documentation](../../mobile_client_MAUI/README.md)  
[← Back to main README](../../README.md)
