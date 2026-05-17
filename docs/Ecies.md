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

## Communication Overview

This ECIES communication process is based on the **ephemeral-static** encryption model.  
Each message encryption generates a new ephemeral X25519 key pair on the sender side.  
Only the **recipient's public key** is required for encryption, while the **recipient's private key** is used for decryption.  
This ensures forward secrecy — every message has a unique shared secret, even if previous keys are compromised.

---

### Key Usage Table

| Direction        | Operation        | Keys Used                                                                                         | Notes |
|------------------|------------------|---------------------------------------------------------------------------------------------------|-------|
| **Client → Host** | **Client encode** | Uses `client_ephemeral_privatekey` (generated internally) + `host_public_key` → produces `ephemeral_publickey`, `IV_Nonce`, `authtag` | The client encrypts the message for the host using the host’s static public key. The ephemeral public key is embedded in the ciphertext. |
|                  | **Host decode**   | Uses `host_private_key` + extracted `ephemeral_publickey`                                         | The host derives the same shared secret and decrypts the message using AES-GCM (`IV_Nonce`, `authtag`). |
| **Host → Client** | **Host encode**   | Uses `host_ephemeral_privatekey` (generated internally) + `client_public_key` → produces `ephemeral_publickey`, `IV_Nonce`, `authtag` | The host encrypts the message for the client using the client’s static public key. The ephemeral public key is included in the ciphertext. |
|                  | **Client decode** | Uses `client_private_key` + extracted `ephemeral_publickey`                                       | The client derives the same shared secret and decrypts the ciphertext using AES-GCM (`IV_Nonce`, `authtag`). |

---

**Summary:**  
- `Encrypt()` → uses only **recipient’s public key** and a new **ephemeral keypair**.  
- `Decrypt()` → uses **recipient’s private key** and the **ephemeral public key** from the message.  
- No sender static keys are required unless sender authentication is added later (e.g., digital signature).


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
