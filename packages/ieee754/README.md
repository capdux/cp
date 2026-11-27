[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ieee754

Zero-allocation IEEE-754 32-bit and 64-bit floating-point binary encoder/decoder (little-endian & big-endian) with NaN, Infinity, and subnormal fidelity (clean-room MIT implementation of npm ieee754, #293 / 173M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ieee754` ([https://github.com/feross/ieee754](https://github.com/feross/ieee754)) | 许可：`BSD-3-Clause`

| Metadata | Value |
| --- | --- |
| Package | `ieee754` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ieee754
```

```cp
import {
    readF32Le,
    readF32Be,
    writeF32Le,
    writeF32Be,
    readF64Le,
    readF64Be,
    writeF64Le,
    writeF64Be,
} from "ieee754";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `readF32Le` | `(buf: &[]u8, offset: usize): f32` | Invokes `readF32Le` and returns `f32`. |
| `readF32Be` | `(buf: &[]u8, offset: usize): f32` | Invokes `readF32Be` and returns `f32`. |
| `writeF32Le` | `(buf: []u8, offset: usize, value: f32): bool` | Invokes `writeF32Le` and returns `bool`. |
| `writeF32Be` | `(buf: []u8, offset: usize, value: f32): bool` | Invokes `writeF32Be` and returns `bool`. |
| `readF64Le` | `(buf: &[]u8, offset: usize): f64` | Invokes `readF64Le` and returns `f64`. |
| `readF64Be` | `(buf: &[]u8, offset: usize): f64` | Invokes `readF64Be` and returns `f64`. |
| `writeF64Le` | `(buf: []u8, offset: usize, value: f64): bool` | Invokes `writeF64Le` and returns `bool`. |
| `writeF64Be` | `(buf: []u8, offset: usize, value: f64): bool` | Invokes `writeF64Be` and returns `bool`. |
| `readIeee754` | `(buf: &[]u8, offset: usize, is_le: bool, n_bytes: usize): f64` | Invokes `readIeee754` and returns `f64`. |
| `writeIeee754` | `(buf: []u8, value: f64, offset: usize, is_le: bool, n_bytes: usize): bool` | Invokes `writeIeee754` and returns `bool`. |

## Minimal Example

```cp
import { readF32Le } from "ieee754";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = readF32Le(out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
