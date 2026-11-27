[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unsigned-varint

Zero-allocation 3-in-1 variable-length integer codec (ULEB128, SLEB128, Protobuf ZigZag, and QUIC RFC 9000 varint) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unsigned-varint` ([https://github.com/paritytech/unsigned-varint](https://github.com/paritytech/unsigned-varint))（原库名 `unsigned-varint` → CP 包名 `unsigned-varint`）

| Metadata | Value |
| --- | --- |
| Package | `unsigned-varint` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unsigned-varint
```

```cp
import {
    U64Decode,
    I64Decode,
    i64ToBits,
    bitsToI64,
    uleb128Size,
    encodeUleb128,
    decodeUleb128,
    zigzagEncode,
} from "unsigned-varint";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `U64Decode` | `class` | Fields: `ok: bool, value: u64, bytes_read: usize` · Methods: `fail` |
| `I64Decode` | `class` | Fields: `ok: bool, value: i64, bytes_read: usize` · Methods: `fail` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `i64ToBits` | `(v: i64): u64` | Invokes `i64ToBits` and returns `u64`. |
| `bitsToI64` | `(u: u64): i64` | Invokes `bitsToI64` and returns `i64`. |
| `uleb128Size` | `(v: u64): usize` | Invokes `uleb128Size` and returns `usize`. |
| `encodeUleb128` | `(v: u64, out: []u8): usize?` | Serializes/formats data via `encodeUleb128` returning `usize?`. |
| `decodeUleb128` | `(data: string): U64Decode` | Parses input via `decodeUleb128` and returns `U64Decode`. |
| `zigzagEncode` | `(v: i64): u64` | Invokes `zigzagEncode` and returns `u64`. |
| `zigzagDecode` | `(u: u64): i64` | Invokes `zigzagDecode` and returns `i64`. |
| `encodeSleb128` | `(v: i64, out: []u8): usize?` | Serializes/formats data via `encodeSleb128` returning `usize?`. |
| `decodeSleb128` | `(data: string): I64Decode` | Parses input via `decodeSleb128` and returns `I64Decode`. |
| `encodeZigzagVarint` | `(v: i64, out: []u8): usize?` | Serializes/formats data via `encodeZigzagVarint` returning `usize?`. |
| `decodeZigzagVarint` | `(data: string): I64Decode` | Parses input via `decodeZigzagVarint` and returns `I64Decode`. |
| `encodeQuicVarint` | `(v: u64, out: []u8): usize?` | Serializes/formats data via `encodeQuicVarint` returning `usize?`. |
| `decodeQuicVarint` | `(data: string): U64Decode` | Parses input via `decodeQuicVarint` and returns `U64Decode`. |

## Minimal Example

```cp
import { decodeUleb128 } from "unsigned-varint";

pub fn main() {
    let result = decodeUleb128("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
