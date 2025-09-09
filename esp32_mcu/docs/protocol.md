

Msg (client → device)
{
  msg_id: 128-bit (UUID/ULID),
  op: "DO_ACTION",            // command
  seq: u32,                   // command counter from this client
  ts: unix_ms,                // timestamp
  auth: HMAC/nonce,           // protection against spoofing/replay
  chan_hint: ["BLE","MQTT"]   // optional, useful for diagnostics
}
op:
DO_ACTION
GET_STATUS

ACK/status (device → client)
{
  ack: msg_id,
  result: "accepted" | "done" | "error",
  state: { gate: "closing"|"closed", version: u32 },
  ts: unix_ms
}