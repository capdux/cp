[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# postcard

Zero-heap compact binary wire protocol codecs (Varint, ZigZag, and COBS framing)

| Metadata | Value |
| --- | --- |
| Package | `postcard` |
| Version | `1.1.3` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add postcard
```

```cp
import {
    VarintU64Res,
    VarintI64Res,
    pcrdU64Max,
    encodeVarintU64,
    decodeVarintU64,
    zigzagEncode,
    zigzagDecode,
    encodeVarintI64,
} from "postcard";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `VarintU64Res` | `class` | Fields: `ok: bool, value: u64, consumed: usize` |
| `VarintI64Res` | `class` | Fields: `ok: bool, value: i64, consumed: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pcrdU64Max` | `(): u64` | Invokes `pcrdU64Max` and returns `u64`. |
| `encodeVarintU64` | `(val: u64, out: []u8): usize` | Serializes/formats data via `encodeVarintU64` returning `usize`. |
| `decodeVarintU64` | `(data: &[]u8): VarintU64Res` | Parses input via `decodeVarintU64` and returns `VarintU64Res`. |
| `zigzagEncode` | `(val: i64): u64` | Invokes `zigzagEncode` and returns `u64`. |
| `zigzagDecode` | `(val: u64): i64` | Invokes `zigzagDecode` and returns `i64`. |
| `encodeVarintI64` | `(val: i64, out: []u8): usize` | Serializes/formats data via `encodeVarintI64` returning `usize`. |
| `decodeVarintI64` | `(data: &[]u8): VarintI64Res` | Parses input via `decodeVarintI64` and returns `VarintI64Res`. |
| `cobsEncode` | `(src: &[]u8, out: []u8): usize` | Invokes `cobsEncode` and returns `usize`. |
| `cobsDecode` | `(src: &[]u8, out: []u8): i64` | Invokes `cobsDecode` and returns `i64`. |

## Minimal Example

```cp
import { encodeVarintU64 } from "postcard";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeVarintU64(0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
