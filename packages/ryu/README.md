[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ryu

Pure CP zero-allocation fast float-to-string formatter (Rust ryu v1 parity, reuses itoa)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ryu` ([https://github.com/dtolnay/ryu](https://github.com/dtolnay/ryu))（原库名 `ryu` → CP 包名 `ryu`）

| Metadata | Value |
| --- | --- |
| Package | `ryu` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ryu
```

```cp
import {
    RyuDecimal,
    ryuPow10U64,
    ryuScalePow10,
    ryuStripTrailingZeros,
    ryuRoundSig,
    ryuExtractShortestF64,
    ryuExtractShortestF32,
    ryuCopySlice,
} from "ryu";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RyuDecimal` | `class` | Fields: `sig: u64, exp10_msb: i32` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ryuPow10U64` | `(n: u8): u64` | Invokes `ryuPow10U64` and returns `u64`. |
| `ryuScalePow10` | `(x: f64, p_in: i32): f64` | Invokes `ryuScalePow10` and returns `f64`. |
| `ryuStripTrailingZeros` | `(sig_in: u64): u64` | Invokes `ryuStripTrailingZeros` and returns `u64`. |
| `ryuRoundSig` | `(m17: u64, drop_digits: u8): u64` | Invokes `ryuRoundSig` and returns `u64`. |
| `ryuExtractShortestF64` | `(abs_val: f64): RyuDecimal` | Invokes `ryuExtractShortestF64` and returns `RyuDecimal`. |
| `ryuExtractShortestF32` | `(abs_f32: f32): RyuDecimal` | Invokes `ryuExtractShortestF32` and returns `RyuDecimal`. |
| `ryuCopySlice` | `(dst: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `ryuCopySlice` and returns `bool`. |
| `ryuPushByte` | `(dst: []u8, pos: &mut usize, b: u8): bool` | Invokes `ryuPushByte` and returns `bool`. |
| `ryuRenderDecimal` | `(neg: bool, dec_val: RyuDecimal, out: []u8): usize` | Invokes `ryuRenderDecimal` and returns `usize`. |
| `ryuRoundAsciiDigits` | `(digits: &[]u8, keep: usize, rounded: []u8): bool` | Invokes `ryuRoundAsciiDigits` and returns `bool`. |
| `isNanF64` | `(val: f64): bool` | Checks predicate `isNanF64` and returns `bool`. |
| `isInfF64` | `(val: f64): bool` | Checks predicate `isInfF64` and returns `bool`. |
| `isFiniteF64` | `(val: f64): bool` | Checks predicate `isFiniteF64` and returns `bool`. |
| `formatF64` | `(val: f64, out: []u8): usize` | Serializes/formats data via `formatF64` returning `usize`. |
| `formatF64Finite` | `(val: f64, out: []u8): usize` | Serializes/formats data via `formatF64Finite` returning `usize`. |
| `formatF32` | `(val: f32, out: []u8): usize` | Serializes/formats data via `formatF32` returning `usize`. |
| `formatFixedF64` | `(val: f64, decimals: u8, out: []u8): usize` | Serializes/formats data via `formatFixedF64` returning `usize`. |
| `formatExpF64` | `(val: f64, precision: u8, out: []u8): usize` | Serializes/formats data via `formatExpF64` returning `usize`. |

## Minimal Example

```cp
import { isNanF64 } from "ryu";

pub fn main() {
    let result = isNanF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
