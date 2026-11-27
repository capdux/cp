[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# smart-buffer

Zero-heap cursor-based binary reader/writer buffer with big/little-endian integer and string operations

| Metadata | Value |
| --- | --- |
| Package | `smart-buffer` |
| Version | `4.2.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add smart-buffer
```

```cp
import {
    SmartBuffer,
    sbufMin,
    sbufW16BE,
    sbufW16LE,
    sbufW32BE,
    sbufW32LE,
    sbufR16BE,
    sbufR16LE,
} from "smart-buffer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SmartBuffer` | `class` | Fields: `buf: [1024]u8, read_offset: usize, write_offset: usize` · Methods: `new, fromSlice, writeU8, writeU16BE, writeU16LE, writeU32BE, +12 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sbufMin` | `(a: usize, b: usize): usize` | Invokes `sbufMin` and returns `usize`. |
| `sbufW16BE` | `(dst: []u8, off: usize, v: u16): bool` | Invokes `sbufW16BE` and returns `bool`. |
| `sbufW16LE` | `(dst: []u8, off: usize, v: u16): bool` | Invokes `sbufW16LE` and returns `bool`. |
| `sbufW32BE` | `(dst: []u8, off: usize, v: u32): bool` | Invokes `sbufW32BE` and returns `bool`. |
| `sbufW32LE` | `(dst: []u8, off: usize, v: u32): bool` | Invokes `sbufW32LE` and returns `bool`. |
| `sbufR16BE` | `(src: &[]u8, off: usize): u16?` | Invokes `sbufR16BE` and returns `u16?`. |
| `sbufR16LE` | `(src: &[]u8, off: usize): u16?` | Invokes `sbufR16LE` and returns `u16?`. |
| `sbufR32BE` | `(src: &[]u8, off: usize): u32?` | Invokes `sbufR32BE` and returns `u32?`. |
| `sbufR32LE` | `(src: &[]u8, off: usize): u32?` | Invokes `sbufR32LE` and returns `u32?`. |

## Minimal Example

```cp
import { sbufMin } from "smart-buffer";

pub fn main() {
    let result = sbufMin(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
