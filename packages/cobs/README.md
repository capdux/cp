[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cobs

Zero-allocation Consistent Overhead Byte Stuffing (COBS), custom-sentinel COBS/XOR, framed codec, and streaming CobsDecoder for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cobs` ([https://github.com/jamesmunns/cobs.rs](https://github.com/jamesmunns/cobs.rs))

| Metadata | Value |
| --- | --- |
| Package | `cobs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cobs
```

```cp
import {
    CobsDecoder,
    max_encoded_len,
    encode,
    decode,
    encode_with_sentinel,
    decode_with_sentinel,
    encode_framed,
    decode_framed,
} from "cobs";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CobsDecoder` | `class` | Fields: `buf: [512]u8, write_len: usize, block_rem: u8, cur_code: u8, has_data: bool, failed: bool, +1 more` · Methods: `init, reset, frame, push` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `max_encoded_len` | `(source_len: usize): usize` | Invokes `max_encoded_len` and returns `usize`. |
| `encode` | `(src: &[]u8, dst: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decode` | `(src: &[]u8, dst: []u8): i64` | Parses input via `decode` and returns `i64`. |
| `encode_with_sentinel` | `(src: &[]u8, dst: []u8, sentinel: u8): usize` | Serializes/formats data via `encode_with_sentinel` returning `usize`. |
| `decode_with_sentinel` | `(src: &[]u8, dst: []u8, sentinel: u8): i64` | Parses input via `decode_with_sentinel` and returns `i64`. |
| `encode_framed` | `(src: &[]u8, dst: []u8): usize` | Serializes/formats data via `encode_framed` returning `usize`. |
| `decode_framed` | `(src: &[]u8, dst: []u8): i64` | Parses input via `decode_framed` and returns `i64`. |

## Minimal Example

```cp
import { encode } from "cobs";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = encode(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
