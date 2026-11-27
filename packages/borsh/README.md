[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# borsh

Zero-allocation canonical little-endian Borsh binary serializer and deserializer for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:borsh` ([https://github.com/near/borsh-rs](https://github.com/near/borsh-rs))

| Metadata | Value |
| --- | --- |
| Package | `borsh` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add borsh
```

```cp
import {
    BytesSpan,
    BorshWriter,
    BorshReader,
    i32ToBits,
    bitsToI32,
    i64ToBits,
    bitsToI64,
} from "borsh";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BytesSpan` | `class` | Fields: `start: usize, len: usize, ok: bool` |
| `BorshWriter` | `class` | Fields: `pos: usize` · Methods: `new, writeU8, writeBool, writeOptionTag, writeU16Le, writeU32Le, +5 more` |
| `BorshReader` | `class` | Fields: `pos: usize, ok: bool` · Methods: `new, remaining, readU8, readBool, readOptionTag, readU16Le, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `i32ToBits` | `(v: i32): u32` | Invokes `i32ToBits` and returns `u32`. |
| `bitsToI32` | `(u: u32): i32` | Invokes `bitsToI32` and returns `i32`. |
| `i64ToBits` | `(v: i64): u64` | Invokes `i64ToBits` and returns `u64`. |
| `bitsToI64` | `(u: u64): i64` | Invokes `bitsToI64` and returns `i64`. |

## Minimal Example

```cp
import { i32ToBits } from "borsh";

pub fn main() {
    let result = i32ToBits(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
