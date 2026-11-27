# mqttbytes — AI Agent Quick Ref

Zero-heap OASIS MQTT v3.1.1 binary control packet encoder/decoder with 1..4-byte Remaining Length, PUBLISH/PUBACK/PINGREQ/PINGRESP/DISCONNECT, and partial-frame detection. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MqttHeader,
    MqttPublish,
    makeHeaderStatus,
    makePublishStatus,
    encodeRemainingLen,
    decodeHeader,
    encodePublish,
    decodePublish,
} from "mqttbytes";
```

## Core Types & Constants

```cp
class MqttHeader { status: i32, pkt_type: u8, flags: u8, remaining_len: u32, /* +2 fields */ }
class MqttPublish { status: i32, qos: u8, retain: bool, dup: bool, /* +6 fields */ }
```

## API Signatures

```cp
fn makeHeaderStatus(status: i32): MqttHeader;
fn makePublishStatus(status: i32): MqttPublish;
fn encodeRemainingLen(len: u32, dst: []u8): i64;
fn decodeHeader(buf: &[]u8): MqttHeader;
fn encodePublish(topic: &[]u8, payload: &[]u8, qos: u8, retain: bool, packet_id: u16, dst: []u8): i64;
fn decodePublish(buf: &[]u8): MqttPublish;
fn encodeSimple(pkt_type: u8, dst: []u8): i64;
fn encodePuback(packet_id: u16, dst: []u8): i64;
```

## Minimal Usage

```cp
import { encodeRemainingLen } from "mqttbytes";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeRemainingLen(0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mqttbytes"`; install via `cap add mqttbytes`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
