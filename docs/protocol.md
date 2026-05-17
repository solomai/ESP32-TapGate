### Security policy
All device communication is encrypted and authenticated using ESP-IDF native cryptography: ephemeral X25519 + HKDF-SHA256 + AES-GCM, with long-term private keys protected by ESP flash encryption and secure boot. An explicit exception is made for EnrollMessage, as the client's cryptographic parameters are not available during user enrollment.

### Cryptography
The device relies on elliptic-curve public-key encryption combined with authenticated symmetric encryption, fully implemented using ESP-IDF 5.5 native cryptography (mbedTLS). Details in [ECIES document](Ecies.md)

### Privacy Strategy
If a message cannot be decoded for any reason, it is marked as invalid. For any invalid requests, the device provides no response and does not store any data. The cause of the silence is irrelevant from the client's perspective. Diagnostics, if required, can be conducted directly through the device logs.

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

- **Users**  
  Non-admin clients are limited to the capabilities explicitly enabled by an administrator. Within the MAUI app, the UI exposes only the functional modules granted to that user, ordered by the same priority categories defined by the administrator.

### Message Structure on-wire:
The message can be presented as one of the following:
1. EnrollMessage

```text
secret_code : max size 15 bytes
pub_key     : 32 bytes
ep_crc32    : CRC 32
```

2. RegularMessage

```text
Host/Client ID       : UID
Ephemeral Public Key : 32 bytes
IV/Nonce             : 12 bytes
Payload              : n bytes
Auth Tag             : 16 bytes
ep_crc32             : CRC 32
```
Minimum message size: UID + empty payload + 60 bytes for ECIES + CRC 32<BR>
where:<BR>
ECIES Encryption overhead = 60 bytes included: [Ephemeral Public Key (32)] [IV/Nonce (12)] [Auth Tag (16)]

### Basic communication scenarios:
1. Enroll a new user to device
2. Admin configured device
3. Request device status by client
4. Request doAction by client

### Client ↔ Device Communication

Messages that cannot be properly decoded are not considered valid requests and must be ignored.

The message lifecycle is always defined as:
Client Request → Device Response.
The client initiates a request to the device, which generates a response for the client.
After the response is sent, the operation is considered completed, and all resources allocated for processing the request must be released on the device.

### Client ↔ Device Communication: 1. Enroll a new user to device

**Scenario 1:** enrollment first user
    
    device precondition: no users have been enrolled on the device (new device or after hard reset).
    The device automatically enters the "enrollment of first user" state.
    available channels: BLE and SoftAP (STA WiFi is disabled).

    client --> device: MsgReqClientEnrollment ( EMPTY SECRET CODE, CLIENT PUB KEY )
    device --> client: MsgRspClientEnrollmentGranted ( ASSIGNED UNIQUE CLIENT ID, DEVICE PUB KEY )

    note: device store a new client after response sent

**Scenario 2:** enrollment other users

    device precondition: first user enrolled ( admin role ).
    The administrator has enabled the mode that allows adding new users. A SECRET CODE is required.
    All available channels are enabled (BLE, SoftAP, STA).

    client --> device: MsgReqClientEnrollment ( SECRET CODE, CLIENT PUB KEY )
    device --> client: MsgRspClientEnrollmentGranted ( ASSIGNED UNIQUE CLIENT ID, DEVICE PUB KEY )

    note: device store a new client after response sent

Fields of the MsgReqClientEnrollment:
```text
{
    DateTime      : Client DateTime
    Secret code   : SECRETE CODE TYPE
    Client PubKey : PUBLIC KEY TYPE
}
```

Fields of the MsgRspClientEnrollmentGranted:
```text
{
    DateTime          : Device DateTime
    Assigned ClientId : CLIENT ID TYPE
    Device PubKey     : PUBLIC KEY TYPE
}
```
More info: [datetime document](../device/datetime.md)

### Client ↔ Device Communication: 2. Admin configured device

**Scenario 1:** setup STA, get current configuration and available AP list

    client --> device: MsgReqStaSetup
    device --> client: MsgRspStaCurrentConfig // Include STA config and list of available AP

The request MsgReqStaSetup does not require extra fields.

Fields of the MsgRspStaCurrentConfig:
```text
{
    Current STA // Name last active STA
    ARRAY OF
    {
        SSID // STA name ( size 32 symbols )
        RSSI // Signal strength
        Channel
        Auth
    } MAX 15 Elements
}
```

**Scenario 2:** setup STA, connect to WiFi with password

    client --> device: MsgReqStaConnection
    device --> client: MsgRspResult // Response OK/NOK STA connection

Fields of the MsgReqStaConnection:
```text
{
    SSID // STA name ( size 32 symbols )
    PSW  // password
}
```

**Scenario 3:** Client configuration, request user list with settings

    client --> device: MsgReqUsers
    device --> client: MsgRspUsers // List enrolled users with policy

The request MsgReqUsers does not require extra fields.

Fields of the MsgRspUsers:
```text
{
    ARRAY OF
    {
        Client Name
        Client Id
        Allow Flags
        Enroll Datetime   // Datetime when was enrolled
        Activity Datetime // Datetime of last activities
    } MAX defined by CLIENTS_DB_MAX_RECORDS
}
```

**Scenario 4:** Client configuration, configure user

   client --> device: MsgReqConfigUser
   device --> client: MsgRspResult // Response OK/NOK configuration applied

Fields of the MsgReqConfigUser:
```text
{
    Client Id   // used like key
    Client Name // updated client name
    Allow Flags // updated allows flags
}
```

**Scenario 5:** Client configuration, remove user

   client --> device: MsgReqRemoveUser
   device --> client: MsgRspResult // Response OK/NOK configuration applied

Fields of the MsgReqRemoveUser:
```text
{
    Client Id // used like key
}
```

**Scenario 6:** Edit mqtt Comunitation config

    client --> device: MsgReqMqttConfig
    device --> client: MsgRspMqttConfig

    // TO BE DEFINED

**Scenario 7:** Sync mqtt Comunitation config with Client ( send mqtt connection data )

    client --> device: MsgReqMqttConfig
    device --> client: MsgRspMqttConfig

The request MsgReqMqttConfig does not require extra fields.

Fields of the MsgRspMqttConfig:
```text
{
    // TO BE DEFINED
}
```

**Scenario 8:** Request Activity logs

    client --> device: MsgReqActivityLogs
    device --> client: MsgRspActivityLogs

The request MsgReqActivityLogs does not require extra fields.

Fields of the MsgRspActivityLogs:
```text
{
    ARRAY OF
    {
        Datetime
        Client Id
        Client Name
        Activity Id // like connected, did action, abnormal activities, ect
    } MAX defined by log system
}
```

**Scenario 9:** Request Device logs

    client --> device: MsgReqDeviceLogs
    device --> client: MsgRspDeviceLogs

The request MsgReqDeviceLogs does not require extra fields.

Fields of the MsgRspDeviceLogs:
```text
{
    ARRAY OF
    {
        Datetime
        Log level // Warnings and Errors
        Message   // max size 128 symbols
    } MAX defined by log system
}
```


### Client ↔ Device Communication: 3. Request device status

**Scenario 1:** Device status: Could be used like ping device

    client --> device: MsgReqStatus
    device --> client: MsgRspResult

Fields of the MsgReqStatus:
```text
{
     DateTime : Client DateTime
}
```
More info: [datetime document](../device/datetime.md)

### Client ↔ Device Communication: 4. Request doAction by client

**Scenario 1:** Request DoAction opt

    client --> device: MsgReqWillDoAction ( to take command nonce )
    device --> client: MsgRspDoActionNonce ( current nonce )

    client --> device: MsgReqDoAction ( check nonce and change, all dublacate will be skipped )
    device --> client: MsgRspResult // OK/NOK answer to confirm opt

The request MsgReqWillDoAction does not require extra fields.

Fields of the MsgMsgRspDoActionNonceRspStatus:
```text
{
    Device None // Next device nonce value
}
```

Fields of the MsgReqDoAction:
```text
{
    Device None // device nonce received on MsgMsgRspDoActionNonceRspStatus
    DoAction Id
}
```

### Client ↔ Device Communication: scenario where a correct message is received but the command is unsupported

This is used even when the response includes nothing but the result of execution.

    client --> device: some request message
    device --> client: MsgRspResult // OK/NOK answer with explanation

Fields of the MsgRspResult:
```text
{
    Error code // 0 - no errors
    Message    // string max size 128
}
```

---

[← Back to ESP32 MCU Documentation](../../esp32_mcu/README.md)  
[← Back to Mobile Client Documentation](../../mobile_client_MAUI/README.md)  
[← Back to main README](../../README.md)