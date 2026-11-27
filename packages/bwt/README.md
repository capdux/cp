[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bwt

Zero-allocation Burrows-Wheeler Transform (BWT) and Move-To-Front (MTF) reversible compression preprocessing in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bwt` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `bwt` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bwt
```

```cp
import {
    BwtResult,
    cmpRotations,
    bwtForward,
    bwtInverse,
    mtfEncode,
    mtfDecode,
} from "bwt";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BwtResult` | `class` | Fields: `primary_index: usize, len: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cmpRotations` | `(src: &[]u8, a: usize, b: usize): i32` | Invokes `cmpRotations` and returns `i32`. |
| `bwtForward` | `(src: &[]u8, dst: []u8): BwtResult` | Invokes `bwtForward` and returns `BwtResult`. |
| `bwtInverse` | `(src: &[]u8, primary_index: usize, dst: []u8): bool` | Invokes `bwtInverse` and returns `bool`. |
| `mtfEncode` | `(src: &[]u8, dst: []u8): bool` | Invokes `mtfEncode` and returns `bool`. |
| `mtfDecode` | `(src: &[]u8, dst: []u8): bool` | Invokes `mtfDecode` and returns `bool`. |

## Minimal Example

```cp
import { cmpRotations } from "bwt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = cmpRotations(out_buf[0..256], 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
