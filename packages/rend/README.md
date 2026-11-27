[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rend

Zero-allocation endian-aware primitive types (U16/U32/I32/U64/I64 Be/Le) and bounds-checked slice readers for zero-copy binary protocols in CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rend` ([https://github.com/rkyv/rend](https://github.com/rkyv/rend), `MIT`)

| Metadata | Value |
| --- | --- |
| Package | `rend` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rend
```

```cp
import {
    U16Be,
    U16Le,
    U32Be,
    U32Le,
    I32Be,
    I32Le,
    U64Be,
    U64Le,
} from "rend";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `U16Be` | `class` | Fields: `bytes: [2]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |
| `U16Le` | `class` | Fields: `bytes: [2]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |
| `U32Be` | `class` | Fields: `bytes: [4]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |
| `U32Le` | `class` | Fields: `bytes: [4]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |
| `I32Be` | `class` | Fields: `bytes: [4]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |
| `I32Le` | `class` | Fields: `bytes: [4]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |
| `U64Be` | `class` | Fields: `bytes: [8]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |
| `U64Le` | `class` | Fields: `bytes: [8]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |
| `I64Be` | `class` | Fields: `bytes: [8]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |
| `I64Le` | `class` | Fields: `bytes: [8]u8` · Methods: `fromNative, fromBytes, toNative, asBytes` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rndPackU16Be` | `(v: u16): [2]u8` | Invokes `rndPackU16Be` and returns `[2]u8`. |
| `rndPackU16Le` | `(v: u16): [2]u8` | Invokes `rndPackU16Le` and returns `[2]u8`. |
| `rndUnpackU16Be` | `(b: [2]u8): u16` | Invokes `rndUnpackU16Be` and returns `u16`. |
| `rndUnpackU16Le` | `(b: [2]u8): u16` | Invokes `rndUnpackU16Le` and returns `u16`. |
| `rndPackU32Be` | `(v: u32): [4]u8` | Invokes `rndPackU32Be` and returns `[4]u8`. |
| `rndPackU32Le` | `(v: u32): [4]u8` | Invokes `rndPackU32Le` and returns `[4]u8`. |
| `rndUnpackU32Be` | `(b: [4]u8): u32` | Invokes `rndUnpackU32Be` and returns `u32`. |
| `rndUnpackU32Le` | `(b: [4]u8): u32` | Invokes `rndUnpackU32Le` and returns `u32`. |
| `rndPackU64Be` | `(v: u64): [8]u8` | Invokes `rndPackU64Be` and returns `[8]u8`. |
| `rndPackU64Le` | `(v: u64): [8]u8` | Invokes `rndPackU64Le` and returns `[8]u8`. |
| `rndUnpackU64Be` | `(b: [8]u8): u64` | Invokes `rndUnpackU64Be` and returns `u64`. |
| `rndUnpackU64Le` | `(b: [8]u8): u64` | Invokes `rndUnpackU64Le` and returns `u64`. |
| `readU16Be` | `(buf: &[]u8, offset: usize): u16?` | Invokes `readU16Be` and returns `u16?`. |
| `readU16Le` | `(buf: &[]u8, offset: usize): u16?` | Invokes `readU16Le` and returns `u16?`. |
| `readU32Be` | `(buf: &[]u8, offset: usize): u32?` | Invokes `readU32Be` and returns `u32?`. |
| `readU32Le` | `(buf: &[]u8, offset: usize): u32?` | Invokes `readU32Le` and returns `u32?`. |
| `readU64Be` | `(buf: &[]u8, offset: usize): u64?` | Invokes `readU64Be` and returns `u64?`. |
| `readU64Le` | `(buf: &[]u8, offset: usize): u64?` | Invokes `readU64Le` and returns `u64?`. |

## Minimal Example

```cp
import { rndPackU16Be } from "rend";

pub fn main() {
    let result = rndPackU16Be(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
