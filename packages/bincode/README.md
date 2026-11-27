[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bincode

Zero-allocation bincode v2 compact binary serializer & zero-copy decoder (LE varint, ZigZag signed ints, length-prefixed strings, Option, fixed LE integers — Rust bincode parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bincode` ([https://github.com/bincode-org/bincode](https://github.com/bincode-org/bincode))

| Metadata | Value |
| --- | --- |
| Package | `bincode` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bincode
```

```cp
import {
    OptStrResult,
    BincodeEncoder,
    BincodeDecoder,
    zigzagEncode,
    zigzagDecode,
} from "bincode";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OptStrResult` | `class` | Fields: `ok: bool, has_value: bool, value: string` |
| `BincodeEncoder` | `class` | Fields: `out: []u8, pos: usize, overflow: bool` · Methods: `init, asSlice, writeByte, encodeBool, encodeFixedU32, encodeFixedU64, +4 more` |
| `BincodeDecoder` | `class` | Fields: `data: string, pos: usize` · Methods: `init, remaining, atEnd, decodeBool, decodeFixedU32, decodeFixedU64, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `zigzagEncode` | `(v: i64): u64` | Invokes `zigzagEncode` and returns `u64`. |
| `zigzagDecode` | `(u: u64): i64` | Invokes `zigzagDecode` and returns `i64`. |

## Minimal Example

```cp
import { zigzagEncode } from "bincode";

pub fn main() {
    let result = zigzagEncode(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
