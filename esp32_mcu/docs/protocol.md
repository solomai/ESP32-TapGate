

Message structure:

| Field        | Type         | Description                         |
|--------------|--------------|-------------------------------------|
| seq (nonce)  | u32          | command counter, also message id    |
| client id    | UID          | unique client id assigned by device |
| Payload      | Base64String | message peyload                     |
| CRC-32       | u32          | CRC checksum                        |

ACK structure:

| Field        | Type         | Description                         |
|--------------|--------------|-------------------------------------|
| seq (nonce)  | u32          | message id from received message    |
| client id    | UID          | unique client id assigned by device |
| CRC-32       | u32          | CRC checksum                        |


The seq value is always incremented by the client when sending a new message. The device expects each new message to have a seq greater than the previous one. This mechanism is used to filter out duplicates and for Anti-replay guard ( Nonce ).


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
    client --> device: MessageAuthorizeMe ( SECRET CODE, PUB KEY, INFO )
    device --> client: ACK
    device --> client: MessageAuthorizeMeGranted ( PUB KEY, CLIENT ID )
    client --> device: ACK

    %% 2. Reset sequence
    [OK]
    device --> client: MessageResetSequence
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

