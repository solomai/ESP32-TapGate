# ecies_crypto

ECIES-based cryptography component for ESP-IDF.

Implements secure message encryption and decryption using:
- X25519 elliptic-curve key agreement (RFC 7748)
- HKDF with SHA-256 for key derivation (RFC 5869)
- AES-GCM for authenticated encryption (NIST SP 800-38D, RFC 5116)

Provides:
- Static key pair generation and storage (device identity)
- Ephemeral key handling per message
- Payload encryption/decryption with integrity protection
- Anti-replay sequence number validation

---

[← Back to ESP32 MCU Documentation](../../esp32_mcu/README.md)  
[← Back to main README](../../README.md)