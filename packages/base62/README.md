[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# base62

Zero-heap URL-safe Base62 (0-9A-Za-z) u64 and arbitrary byte-slice encoder/decoder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:base62` ([https://github.com/fbernier/base62](https://github.com/fbernier/base62))

| Metadata | Value |
| --- | --- |
| Package | `base62` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add base62
```

```cp
import {
    DecodeU64Result,
    charToVal,
    isValidBase62,
    encodeU64,
    encodeU64Padded,
    decodeU64,
    encodeBytes,
    decodeBytes,
} from "base62";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DecodeU64Result` | `class` | Fields: `value: u64, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `charToVal` | `(c: u8): i32` | Invokes `charToVal` and returns `i32`. |
| `isValidBase62` | `(src: &[]u8): bool` | Checks predicate `isValidBase62` and returns `bool`. |
| `encodeU64` | `(value: u64, dst: []u8): usize` | Serializes/formats data via `encodeU64` returning `usize`. |
| `encodeU64Padded` | `(value: u64, width: usize, dst: []u8): usize` | Serializes/formats data via `encodeU64Padded` returning `usize`. |
| `decodeU64` | `(src: &[]u8): DecodeU64Result` | Parses input via `decodeU64` and returns `DecodeU64Result`. |
| `encodeBytes` | `(src: &[]u8, dst: []u8): i64` | Serializes/formats data via `encodeBytes` returning `i64`. |
| `decodeBytes` | `(src: &[]u8, dst: []u8): i64` | Parses input via `decodeBytes` and returns `i64`. |

## Minimal Example

```cp
import { isValidBase62 } from "base62";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValidBase62(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
