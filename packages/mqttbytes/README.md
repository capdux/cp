[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mqttbytes

Zero-heap OASIS MQTT v3.1.1 binary control packet encoder/decoder with 1..4-byte Remaining Length, PUBLISH/PUBACK/PINGREQ/PINGRESP/DISCONNECT, and partial-frame detection.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:mqttbytes` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `mqttbytes` → CP 包名 `mqttbytes`）

| Metadata | Value |
| --- | --- |
| Package | `mqttbytes` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mqttbytes
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MqttHeader` | `class` | Fields: `status: i32, pkt_type: u8, flags: u8, remaining_len: u32, header_len: usize, total_len: usize` |
| `MqttPublish` | `class` | Fields: `status: i32, qos: u8, retain: bool, dup: bool, packet_id: u16, topic_start: usize, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeHeaderStatus` | `(status: i32): MqttHeader` | Invokes `makeHeaderStatus` and returns `MqttHeader`. |
| `makePublishStatus` | `(status: i32): MqttPublish` | Invokes `makePublishStatus` and returns `MqttPublish`. |
| `encodeRemainingLen` | `(len: u32, dst: []u8): i64` | Serializes/formats data via `encodeRemainingLen` returning `i64`. |
| `decodeHeader` | `(buf: &[]u8): MqttHeader` | Parses input via `decodeHeader` and returns `MqttHeader`. |
| `encodePublish` | `(topic: &[]u8, payload: &[]u8, qos: u8, retain: bool, packet_id: u16, dst: []u8): i64` | Serializes/formats data via `encodePublish` returning `i64`. |
| `decodePublish` | `(buf: &[]u8): MqttPublish` | Parses input via `decodePublish` and returns `MqttPublish`. |
| `encodeSimple` | `(pkt_type: u8, dst: []u8): i64` | Serializes/formats data via `encodeSimple` returning `i64`. |
| `encodePuback` | `(packet_id: u16, dst: []u8): i64` | Serializes/formats data via `encodePuback` returning `i64`. |

## Minimal Example

```cp
import { encodeRemainingLen } from "mqttbytes";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeRemainingLen(0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
