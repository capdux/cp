[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# num-conv

Zero-heap lossless widening, explicit truncating, checked, and saturating integer conversions matching Rust num-conv in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-conv` ([https://github.com/jhpratt/num-conv](https://github.com/jhpratt/num-conv))

| Metadata | Value |
| --- | --- |
| Package | `num-conv` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add num-conv
```

```cp
import {
    CheckedU8,
    CheckedU16,
    CheckedU32,
    CheckedI8,
    CheckedI16,
    CheckedI32,
    extendU8ToU64,
    extendU16ToU64,
} from "num-conv";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CheckedU8` | `class` | Fields: `val: u8, ok: bool` |
| `CheckedU16` | `class` | Fields: `val: u16, ok: bool` |
| `CheckedU32` | `class` | Fields: `val: u32, ok: bool` |
| `CheckedI8` | `class` | Fields: `val: i8, ok: bool` |
| `CheckedI16` | `class` | Fields: `val: i16, ok: bool` |
| `CheckedI32` | `class` | Fields: `val: i32, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `extendU8ToU64` | `(v: u8): u64` | Invokes `extendU8ToU64` and returns `u64`. |
| `extendU16ToU64` | `(v: u16): u64` | Invokes `extendU16ToU64` and returns `u64`. |
| `extendU32ToU64` | `(v: u32): u64` | Invokes `extendU32ToU64` and returns `u64`. |
| `extendI8ToI64` | `(v: i8): i64` | Invokes `extendI8ToI64` and returns `i64`. |
| `extendI16ToI64` | `(v: i16): i64` | Invokes `extendI16ToI64` and returns `i64`. |
| `extendI32ToI64` | `(v: i32): i64` | Invokes `extendI32ToI64` and returns `i64`. |
| `truncateU64ToU8` | `(v: u64): u8` | Invokes `truncateU64ToU8` and returns `u8`. |
| `truncateU64ToU16` | `(v: u64): u16` | Invokes `truncateU64ToU16` and returns `u16`. |
| `truncateU64ToU32` | `(v: u64): u32` | Invokes `truncateU64ToU32` and returns `u32`. |
| `tryU64ToU8` | `(v: u64): CheckedU8` | Invokes `tryU64ToU8` and returns `CheckedU8`. |
| `tryU64ToU16` | `(v: u64): CheckedU16` | Invokes `tryU64ToU16` and returns `CheckedU16`. |
| `tryU64ToU32` | `(v: u64): CheckedU32` | Invokes `tryU64ToU32` and returns `CheckedU32`. |
| `tryI64ToI8` | `(v: i64): CheckedI8` | Invokes `tryI64ToI8` and returns `CheckedI8`. |
| `tryI64ToI16` | `(v: i64): CheckedI16` | Invokes `tryI64ToI16` and returns `CheckedI16`. |
| `tryI64ToI32` | `(v: i64): CheckedI32` | Invokes `tryI64ToI32` and returns `CheckedI32`. |
| `tryI64ToU8` | `(v: i64): CheckedU8` | Invokes `tryI64ToU8` and returns `CheckedU8`. |
| `tryI64ToU32` | `(v: i64): CheckedU32` | Invokes `tryI64ToU32` and returns `CheckedU32`. |
| `saturatingI64ToU8` | `(v: i64): u8` | Invokes `saturatingI64ToU8` and returns `u8`. |
| `saturatingI64ToI8` | `(v: i64): i8` | Invokes `saturatingI64ToI8` and returns `i8`. |
| `saturatingI64ToI16` | `(v: i64): i16` | Invokes `saturatingI64ToI16` and returns `i16`. |
| `saturatingU64ToU32` | `(v: u64): u32` | Invokes `saturatingU64ToU32` and returns `u32`. |

## Minimal Example

```cp
import { extendU8ToU64 } from "num-conv";

pub fn main() {
    let result = extendU8ToU64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
