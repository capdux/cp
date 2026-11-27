[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ciborium-ll

Zero-heap RFC 8949 low-level streaming CBOR encoder and decoder primitives in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ciborium-ll` ([https://github.com/enarx/ciborium](https://github.com/enarx/ciborium)) — `Apache-2.0`

| Metadata | Value |
| --- | --- |
| Package | `ciborium-ll` |
| Version | `1.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ciborium-ll
```

```cp
import {
    CborHeaderKind,
    CborHeader,
    CborDecoder,
    CborEncoder,
    cblMaxI64AsU64,
    cblReadBe16,
    cblReadBe32,
    cblReadBe64,
} from "ciborium-ll";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CborHeaderKind` | `Positive, Negative, Bytes, Text, Array, Map, Tag, Bool ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CborHeader` | `class` | Fields: `kind: CborHeaderKind, u64_val: u64, i64_val: i64, f64_val: f64, bool_val: bool, len: i64` · Methods: `invalid` |
| `CborDecoder` | `class` | Fields: `data: &[]u8, pos: usize` · Methods: `new, remaining, isEof, readBytes, pull` |
| `CborEncoder` | `class` | Fields: `buf: []u8, pos: usize` · Methods: `new, written, remaining, slice, pushPositive, pushNegativeRaw, +13 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cblMaxI64AsU64` | `(): u64` | Invokes `cblMaxI64AsU64` and returns `u64`. |
| `cblReadBe16` | `(data: &[]u8, pos: usize): u16` | Invokes `cblReadBe16` and returns `u16`. |
| `cblReadBe32` | `(data: &[]u8, pos: usize): u32` | Invokes `cblReadBe32` and returns `u32`. |
| `cblReadBe64` | `(data: &[]u8, pos: usize): u64` | Invokes `cblReadBe64` and returns `u64`. |
| `cblDecodeHalf` | `(bits: u16): f64` | Invokes `cblDecodeHalf` and returns `f64`. |
| `cblArgSize` | `(val: u64): usize` | Invokes `cblArgSize` and returns `usize`. |
| `cblWriteHead` | `(buf: []u8, pos: usize, major: u8, val: u64): usize` | Invokes `cblWriteHead` and returns `usize`. |

## Minimal Example

```cp
import { cblMaxI64AsU64 } from "ciborium-ll";

pub fn main() {
    let result = cblMaxI64AsU64();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
