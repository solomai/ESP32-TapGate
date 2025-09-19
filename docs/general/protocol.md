### Security policy
All device communication is encrypted and authenticated using ESP-IDF native cryptography: ephemeral X25519 + HKDF-SHA256 + AES-GCM, with long-term private keys protected by ESP flash encryption and secure boot.

### Cryptography
The device relies on elliptic-curve public-key encryption combined with authenticated symmetric encryption, fully implemented using ESP-IDF 5.5 native cryptography (mbedTLS). For each message, an ephemeral X25519 ECDH key exchange (RFC 7748) is performed with the device’s long-term static key to derive a shared secret. This secret is expanded into a symmetric session key using HKDF with SHA-256 (RFC 5869). Payloads are then encrypted and authenticated using AES-GCM (NIST SP 800-38D, AEAD usage per RFC 5116). This ECIES-style construction (EC key agreement + HKDF + AEAD) ensures confidentiality, integrity, and forward secrecy at the message level, since every packet includes a fresh ephemeral public key. Long-term private keys are stored in encrypted flash partitions with ESP-IDF flash encryption and secure boot enabled, while random values (ephemeral keys and nonces) are generated using the platform CSPRNG seeded from hardware entropy (esp_random).

### Privacy Strategy
If a message cannot be decoded for any reason, it is marked as invalid. For any invalid requests, the device provides no response and does not store any data. The cause of the silence is irrelevant from the client's perspective. Diagnostics, if required, can be conducted directly through the device logs. A missing ACK indicates that the message has failed.


### Transmission

All data is transmitted on-wire as a Base64-encoded string.

Structure of the message transmitted between device and client:

on-wire package:
```text

struct Packet {
    // HEADER
    uint8_t  client_id[15];        // fixed, 15 bytes (UID)
    uint8_t  eph_pub[32];          // fixed, 32 bytes ECDH (X25519)
    // AEAD (AES-GCM)
    uint8_t  nonce[12];            // fixed, 12 bytes (AES-GCM IV)
    uint8_t  encrypted_payload[];
    // CRC
    uint32_t crc-32;               // CRC checksum
}

encrypted_payload:
struct Plaintext {
    uint32_t client_nonce;  // anti-replay
    uint8_t  msg_code;      // message code
    uint8_t  payload[];     // other fields depend on type of message
}

```

ACK structure:
```text

struct Packet {
    uint8_t  client_id[15];  // fixed, 15 bytes (UID)
    uint8_t  nonce[12];      // replayed from received Packet
    uint32_t crc-32;         // CRC checksum
}

```

### Client ↔ Device Communication

Basic communication scenarios:

Scenario 1: enrollment of a new client

    precondition: The device is switched to new client enrollment mode. The operation expires after the time specified in the options and/or once a client has been enrolled.
    In the case of remote client enrollment, a SECRET CODE is generated.

    client --> device: MessageClientEnrollment ( SECRET CODE, PUB KEY )
    device --> client: ACK
    device --> client: MessageClientEnrollmentGranted ( CLIENT ID, PUB KEY )
    client --> device: ACK

Scenario 2: Get Client Info

    device --> client: MessageGetClientInfo
    client --> device: ACK
    client --> device: MessageClientInfo
    device --> client: ACK

Scenario 3: Get device status

    client --> device: MessageGetStatus
    device --> client: ACK
    device --> client: MessageStatus
    client --> device: ACK

Scenario 4: DoAction

    client --> device: MessageWillDoAction ( to take command nonce )
    device --> client: ACK
    device --> client: MessageDoActionNonce ( wait nonce)
    client --> device: ACK
    client --> device: MessageDoAction
    device --> client: ACK


Extended communication scenarios:

Extended scenarios can be used for additional remote device control or for access to the admin panel.

---

[← Back to ESP32 MCU Documentation](../../esp32_mcu/README.md)  
[← Back to Mobile Client Documentation](../../mobile_client_MAUI/README.md)  
[← Back to main README](../../README.md)