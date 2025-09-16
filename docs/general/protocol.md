

All data is transmitted as a Base64-encoded string.

Structure of the message transmitted between device and client:

| Field        | Type         | Description                         |
|--------------|--------------|-------------------------------------|
| client id    | UID          | unique client id assigned by device |
| Payload      | Base64String | message peyload                     |
| CRC-32       | u32          | CRC checksum                        |

ACK structure:

| Field        | Type         | Description                         |
|--------------|--------------|-------------------------------------|
| client id    | UID          | unique client id assigned by device |
| CRC-32       | u32          | CRC checksum                        |

Payload structure:
1. Message for adding a new client (certificate exchange):<br>
```text
preliminary size : u8 - preliminary data size
preliminary data : string
pub key size     : u16 - public key data size 
pub key          : public key data
```

2. Other messages:<br>
The payload is always encrypted with the sender's private key. Decryption must be performed using the public key obtained during the certificate exchange.
If you’d like, I can also help rephrase this for different technical audiences or embed it into a protocol spec or documentation template.<br>
```text
nonce : number;     // Used to filter out duplicates like message id and for Anti-replay guard
opt   : number;     // Operation code to be executed or message id
data  : any or non; // Message type dictates the structure and content of the transmitted data.
```


Privacy Strategy:
For any unclear or invalid requests, the device provides no response and does not store any data. The cause of the silence is irrelevant from the client's perspective. Diagnostics, if required, can be conducted directly through the device logs. A missing ACK indicates that the message has failed.

Sequences:

    %% 0. Sequence behavior in the fail case
    [NOK]
    client --> device: any wrong message
    device --> client: silent

    %% 1. Add me message
    [OK]
    device --> device: set allow adding mode and generate SECRET CODE
    client --> device: MessageAuthorizeMe ( SECRET CODE, PUB KEY )
    device --> client: ACK
    device --> client: MessageAuthorizeMeGranted ( CLIENT ID, PUB KEY )
    client --> device: ACK
    client --> device: MessageClientInfo
    device --> client: ACK

    %% 2. Reset nonce
    [OK]
    device --> client: MessageResetNonce
    client --> device: ACK

    %% 3. DoAction
    [OK]
    client --> device: MessageDoAction
    device --> client: ACK

    %% 4. Get device status
    [OK]
    client --> device: MessageGetStatus
    device --> client: ACK
    device --> client: MessageStatus
    client --> device: ACK

---

[← Back to ESP32 MCU Documentation](../../esp32_mcu/README.md)  
[← Back to Mobile Client Documentation](../../mobile_client_MAUI/README.md)  
[← Back to main README](../../README.md)