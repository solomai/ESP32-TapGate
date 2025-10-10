### Security policy
All device communication is encrypted and authenticated using ESP-IDF native cryptography: ephemeral X25519 + HKDF-SHA256 + AES-GCM, with long-term private keys protected by ESP flash encryption and secure boot.

### Cryptography
The device relies on elliptic-curve public-key encryption combined with authenticated symmetric encryption, fully implemented using ESP-IDF 5.5 native cryptography (mbedTLS). For each message, an ephemeral X25519 ECDH key exchange (RFC 7748) is performed with the device’s long-term static key to derive a shared secret. This secret is expanded into a symmetric session key using HKDF with SHA-256 (RFC 5869). Payloads are then encrypted and authenticated using AES-GCM (NIST SP 800-38D, AEAD usage per RFC 5116). This ECIES-style construction (EC key agreement + HKDF + AEAD) ensures confidentiality, integrity, and forward secrecy at the message level, since every packet includes a fresh ephemeral public key. Long-term private keys are stored in encrypted flash partitions with ESP-IDF flash encryption and secure boot enabled, while random values (ephemeral keys and nonces) are generated using the platform CSPRNG seeded from hardware entropy (esp_random).

### Privacy Strategy
If a message cannot be decoded for any reason, it is marked as invalid. For any invalid requests, the device provides no response and does not store any data. The cause of the silence is irrelevant from the client's perspective. Diagnostics, if required, can be conducted directly through the device logs. A missing ACK indicates that the message has failed.

### Device Security States and Enrollment Flow

- **Unsecured device (factory-fresh / after hard reset)**  
  Bluetooth LE, Wi-Fi SoftAP, and Wi-Fi STA are enabled but the STA client is not yet associated with an access point. Only local connections are accepted. The first user who connects through the MAUI app must enroll as the primary administrator. No additional security challenges are enforced in this phase because the state is expected only during initial provisioning.

- **Secured device (administrator enrolled)**  
  The device is considered configured. Additional clients can be connected only when an administrator explicitly authorizes them. Administrators generate short-lived invitation keys (5–15 minutes) that can be redeemed over any transport channel (BLE, Wi-Fi AP, or the Internet via MQTT). Once an invitation is consumed or expires, it can no longer be used.

### Administrative Roles

- **Administrators**  
  Only enrolled administrators can access the administration panel in the MAUI application. They manage all IoT settings, including network parameters and feature toggles. Administrators are also responsible for user lifecycle tasks—inviting new clients, revoking access, or upgrading user permissions. They can review event logs to audit device activity. The admin workspace is organized by priority categories:
    1. Device configuration (IoT settings, connectivity).
    2. Client management (invites, removals, role adjustments).
    3. Event and security logging.

- **Standard users**  
  Non-admin clients are limited to the capabilities explicitly enabled by an administrator. Within the MAUI app, the UI exposes only the functional modules granted to that user, ordered by the same priority categories defined by the administrator.

### Basic communication scenarios:
1. Enroll a new user to device
2. Admin configured device
3. Request device status by client
4. Request doAction by client

### Client ↔ Device Communication: 1. Enroll a new user to device

Scenario 1: enrollment first user
    
    device precondition: no users have been enrolled on the device (new device or after hard reset).
    The device automatically enters the "enrollment of first user" state.
    available channels: BLE and SoftAP (STA WiFi is disabled).

    client --> device: MessageClientEnrollment ( EMPTY SECRET CODE, CLIENT PUB KEY )
    device --> client: MessageClientEnrollmentGranted ( ASSIGNED UNIQUE CLIENT ID, DEVICE PUB KEY )
    client --> device: ACK - after this message device store a new client

Scenario 2: enrollment user

    device precondition: first user enrolled ( admin role ).
    The administrator has enabled the mode that allows adding new users. A SECRET CODE is required.
    All available channels are enabled (BLE, SoftAP, STA).

    client --> device: MessageClientEnrollment ( SECRET CODE, CLIENT PUB KEY )
    device --> client: MessageClientEnrollmentGranted ( ASSIGNED UNIQUE CLIENT ID, DEVICE PUB KEY )
    client --> device: ACK - after this message device store a new client


### Client ↔ Device Communication: 2. Admin configured device

Scenario 1: setup STA, get current configuration and available AP list

    client --> device: MessageRequestSTASetup
    device --> client: MessageSTACurrentConfig
    device --> client: MessageAvailableAP ( first frame with list of available AP )
    ...
    device --> client: MessageAvailableAP ( last frame with list of available AP )

Scenario 2: setup STA, connect to WiFi with password

    client --> device: MessageRequestStaConnection
    device --> client: ACK - confirm that configuration applied

Scenario 3: Client configuration, request user list with settings

    client --> device: MessageRequestUsers
    device --> client: MessageUser ( first frame with added user )
    ...
    device --> client: MessageUser ( last frame with added user )

Scenario 4: Client configuration, configure user

   client --> device: MessageConfigUser
   device --> client: ACK - confirm that configuration applied

Scenario 5: Request diag info

    client --> device: MessageRequestDiag
    device --> client: MessageDiag ( first frame with diag info )
    ...
    device --> client: MessageDiag ( last frame with diag info )


### Client ↔ Device Communication: 3. Request device status and config

Scenario 1: Device status: Could be used like ping device

    client --> device: MessageRequestStatus
    device --> client: MessageStatus

Scenario 2: Sync mqtt Comunitation config ( send mqtt connection data )

    client --> device: MessageRequestMqttConfig
    device --> client: MessageMqttConfig


### Client ↔ Device Communication: 4. Request doAction by client

Scenario 1: Request DoAction opt

    client --> device: MessageWillDoAction ( to take command nonce )
    device --> client: MessageDoActionNonce ( current nonce )
    client --> device: MessageDoAction ( check nonce and change, all dublacate will be skipped )
    device --> client: ACK - OK/NOK answer to confirm opt


### Client ↔ Device Communication: scenario where a correct message is received but the command is unsupported

    client --> device: some message
    device --> client: ACK - NOK answer
---

[← Back to ESP32 MCU Documentation](../../esp32_mcu/README.md)  
[← Back to Mobile Client Documentation](../../mobile_client_MAUI/README.md)  
[← Back to main README](../../README.md)