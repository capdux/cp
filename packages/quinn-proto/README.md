[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# quinn-proto

Pure CP zero-allocation RFC 9000 QUIC variable-length integer codec and Long Header parser (quinn-proto parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:quinn-proto` ([https://github.com/quinn-rs/quinn](https://github.com/quinn-rs/quinn)) / `crates.io:quinn` ([https://github.com/quinn-rs/quinn](https://github.com/quinn-rs/quinn))（原库名 `quinn-proto` → CP 包名 `quinn-proto`）

| Metadata | Value |
| --- | --- |
| Package | `quinn-proto` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add quinn-proto
```

```cp
import {
    QuicVarint,
    QuicLongHeader,
    encodedLen,
    encodeVarint,
    decodeVarint,
    qvInvalidHeader,
    parseLongHeader,
} from "quinn-proto";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `QuicVarint` | `class` | Fields: `val: u64, bytes_read: usize, ok: bool` |
| `QuicLongHeader` | `class` | Fields: `is_long: bool, packet_type: u8, version: u32, dcid_len: u8, scid_len: u8, header_len: usize, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `encodedLen` | `(val: u64): usize` | Serializes/formats data via `encodedLen` returning `usize`. |
| `encodeVarint` | `(val: u64, out: []u8): usize` | Serializes/formats data via `encodeVarint` returning `usize`. |
| `decodeVarint` | `(buf: []u8): QuicVarint` | Parses input via `decodeVarint` and returns `QuicVarint`. |
| `qvInvalidHeader` | `(): QuicLongHeader` | Invokes `qvInvalidHeader` and returns `QuicLongHeader`. |
| `parseLongHeader` | `(pkt: []u8): QuicLongHeader` | Parses input via `parseLongHeader` and returns `QuicLongHeader`. |

## Minimal Example

```cp
import { encodedLen } from "quinn-proto";

pub fn main() {
    let result = encodedLen(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
