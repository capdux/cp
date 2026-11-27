[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cast

Zero-allocation checked primitive numeric casts preventing silent overflow, underflow, and NaN/Inf truncation (clean-room MIT implementation of crates.io cast, #443 / 335M+)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cast` ([https://github.com/japaric/cast.rs](https://github.com/japaric/cast.rs)) | 许可：`MIT OR Apache-2.0`

| Metadata | Value |
| --- | --- |
| Package | `cast` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cast
```

```cp
import {
    CastError,
    u8FromI64,
    u8FromU64,
    u16FromI64,
    u32FromI64,
    u32FromU64,
    i32FromU64,
    i32FromI64,
} from "cast";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CastError` | `Overflow, Underflow, Infinite, NaN` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `u8FromI64` | `(v: i64): u8?` | Invokes `u8FromI64` and returns `u8?`. |
| `u8FromU64` | `(v: u64): u8?` | Invokes `u8FromU64` and returns `u8?`. |
| `u16FromI64` | `(v: i64): u16?` | Invokes `u16FromI64` and returns `u16?`. |
| `u32FromI64` | `(v: i64): u32?` | Invokes `u32FromI64` and returns `u32?`. |
| `u32FromU64` | `(v: u64): u32?` | Invokes `u32FromU64` and returns `u32?`. |
| `i32FromU64` | `(v: u64): i32?` | Invokes `i32FromU64` and returns `i32?`. |
| `i32FromI64` | `(v: i64): i32?` | Invokes `i32FromI64` and returns `i32?`. |
| `u64FromI64` | `(v: i64): u64?` | Invokes `u64FromI64` and returns `u64?`. |
| `i64FromU64` | `(v: u64): i64?` | Invokes `i64FromU64` and returns `i64?`. |
| `usizeFromI64` | `(v: i64): usize?` | Invokes `usizeFromI64` and returns `usize?`. |
| `u32FromF64` | `(v: f64): u32?` | Invokes `u32FromF64` and returns `u32?`. |
| `i32FromF64` | `(v: f64): i32?` | Invokes `i32FromF64` and returns `i32?`. |
| `u8FromF64` | `(v: f64): u8?` | Invokes `u8FromF64` and returns `u8?`. |
| `checkF64ToI32` | `(v: f64): CastError?` | Validates input with `checkF64ToI32` returning `CastError?`. |

## Minimal Example

```cp
import { u8FromI64 } from "cast";

pub fn main() {
    let result = u8FromI64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
