[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# google-protobuf

Zero-allocation Protocol Buffers Wire Format encoder/decoder reusing varint (ULEB128 + ZigZag) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:google-protobuf` ([https://github.com/protocolbuffers/protobuf-javascript](https://github.com/protocolbuffers/protobuf-javascript)) / `crates.io:prost` ([https://github.com/tokio-rs/prost](https://github.com/tokio-rs/prost)) / `npm:pbf` ([https://github.com/mapbox/pbf](https://github.com/mapbox/pbf))（原库名 `google-protobuf` → CP 包名 `google-protobuf`）

| Metadata | Value |
| --- | --- |
| Package | `google-protobuf` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add google-protobuf
```

```cp
import {
    FieldTag,
    BytesField,
    Sint64Decode,
    isValidWireType,
    encodeTag,
    decodeTag,
    encodeVarintField,
    encodeSint64Field,
} from "google-protobuf";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FieldTag` | `class` | Fields: `field_number: u32, wire_type: u8, bytes_read: usize, ok: bool` |
| `BytesField` | `class` | Fields: `start: usize, len: usize, next_pos: usize, ok: bool` |
| `Sint64Decode` | `class` | Fields: `value: i64, bytes_read: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isValidWireType` | `(wt: u8): bool` | Checks predicate `isValidWireType` and returns `bool`. |
| `encodeTag` | `(field_number: u32, wire_type: u8, out: []u8): usize` | Serializes/formats data via `encodeTag` returning `usize`. |
| `decodeTag` | `(buf: &[]u8): FieldTag` | Parses input via `decodeTag` and returns `FieldTag`. |
| `encodeVarintField` | `(field_number: u32, val: u64, out: []u8): usize` | Serializes/formats data via `encodeVarintField` returning `usize`. |
| `encodeSint64Field` | `(field_number: u32, val: i64, out: []u8): usize` | Serializes/formats data via `encodeSint64Field` returning `usize`. |
| `decodeVarintValue` | `(buf: &[]u8, pos: usize): U64Decode` | Parses input via `decodeVarintValue` and returns `U64Decode`. |
| `decodeSint64Value` | `(buf: &[]u8, pos: usize): Sint64Decode` | Parses input via `decodeSint64Value` and returns `Sint64Decode`. |
| `encodeFixed32Field` | `(field_number: u32, val: u32, out: []u8): usize` | Serializes/formats data via `encodeFixed32Field` returning `usize`. |
| `decodeFixed32` | `(buf: &[]u8, pos: usize): u32` | Parses input via `decodeFixed32` and returns `u32`. |
| `encodeFixed64Field` | `(field_number: u32, val: u64, out: []u8): usize` | Serializes/formats data via `encodeFixed64Field` returning `usize`. |
| `decodeFixed64` | `(buf: &[]u8, pos: usize): u64` | Parses input via `decodeFixed64` and returns `u64`. |
| `encodeBytesField` | `(field_number: u32, payload: string, out: []u8): usize` | Serializes/formats data via `encodeBytesField` returning `usize`. |
| `decodeBytesField` | `(buf: &[]u8, pos: usize): BytesField` | Parses input via `decodeBytesField` and returns `BytesField`. |
| `skipField` | `(buf: &[]u8, pos: usize, wire_type: u8): i64` | Invokes `skipField` and returns `i64`. |

## Minimal Example

```cp
import { encodeBytesField } from "google-protobuf";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeBytesField(0, "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
