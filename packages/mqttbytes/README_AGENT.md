# mqttbytes@1.0.0 (Agent Quickref)

> Zero-heap OASIS MQTT v3.1.1 binary control packet encoder/decoder with 1..4-byte Remaining Length, PUBLISH/PUBACK/PINGREQ/PINGRESP/DISCONNECT, and partial-frame detection. | License: MIT
> Import: `import { ... } from "mqttbytes";` | Full types: `src/index.d.cp`

## Functions
- `export fn makeHeaderStatus(status: i32): MqttHeader;`
- `export fn makePublishStatus(status: i32): MqttPublish;`
- `export fn encodeRemainingLen(len: u32, dst: []u8): i64;`
- `export fn decodeHeader(buf: &[]u8): MqttHeader;`
- `export fn encodePublish(topic: &[]u8, payload: &[]u8, qos: u8, retain: bool, packet_id: u16, dst: []u8): i64;`
- `export fn decodePublish(buf: &[]u8): MqttPublish;`
- `export fn encodeSimple(pkt_type: u8, dst: []u8): i64;`
- `export fn encodePuback(packet_id: u16, dst: []u8): i64;`
