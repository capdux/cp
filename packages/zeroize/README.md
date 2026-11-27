[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# zeroize

Zero-heap secure secret memory wiping and ZeroizingBuf32/ZeroizingBuf64 key containers (crates.io zeroize equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:zeroize` ([https://github.com/RustCrypto/utils](https://github.com/RustCrypto/utils))

| Metadata | Value |
| --- | --- |
| Package | `zeroize` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add zeroize
```

```cp
import {
    ZeroizingKey32,
    ZeroizingBuf64,
    zeroizeBytes,
    zeroizeU32,
    zeroizeU64,
    isAllZeroBytes,
} from "zeroize";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ZeroizingKey32` | `class` | Fields: `bytes: [32]u8, active: bool` · Methods: `fromSlice, zeroize, isZeroed` |
| `ZeroizingBuf64` | `class` | Fields: `bytes: [64]u8, len: usize, active: bool` · Methods: `fromSlice, zeroize, isZeroed` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `zeroizeBytes` | `(buf: []u8): usize` | Invokes `zeroizeBytes` and returns `usize`. |
| `zeroizeU32` | `(buf: []u32): usize` | Invokes `zeroizeU32` and returns `usize`. |
| `zeroizeU64` | `(buf: []u64): usize` | Invokes `zeroizeU64` and returns `usize`. |
| `isAllZeroBytes` | `(buf: []u8): bool` | Checks predicate `isAllZeroBytes` and returns `bool`. |

## Minimal Example

```cp
import { isAllZeroBytes } from "zeroize";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isAllZeroBytes(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
