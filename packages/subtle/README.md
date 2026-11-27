[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# subtle

Zero-allocation branchless constant-time cryptographic primitives (Choice, ctEq, ctLt, ctSelect, ctCondSwap, zeroizeBytes) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:subtle` ([https://github.com/dalek-cryptography/subtle](https://github.com/dalek-cryptography/subtle)) / `npm:tsscmp` ([https://github.com/suryagh/tsscmp](https://github.com/suryagh/tsscmp)) / `npm:safe-compare` ([https://github.com/vadimdemedes/safe-compare](https://github.com/vadimdemedes/safe-compare))（原库名 `subtle` → CP 包名 `subtle`）

| Metadata | Value |
| --- | --- |
| Package | `subtle` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add subtle
```

```cp
import {
    Choice,
    mask8,
    mask32,
    mask64,
    ctIsZeroU8,
    ctIsZeroU32,
    ctIsZeroU64,
    ctEqU8,
} from "subtle";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Choice` | `class` | Fields: `val: u8` · Methods: `fromU8, toBool, not, and, or, xor` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mask8` | `(c: Choice): u8` | Invokes `mask8` and returns `u8`. |
| `mask32` | `(c: Choice): u32` | Invokes `mask32` and returns `u32`. |
| `mask64` | `(c: Choice): u64` | Invokes `mask64` and returns `u64`. |
| `ctIsZeroU8` | `(x: u8): Choice` | Invokes `ctIsZeroU8` and returns `Choice`. |
| `ctIsZeroU32` | `(x: u32): Choice` | Invokes `ctIsZeroU32` and returns `Choice`. |
| `ctIsZeroU64` | `(x: u64): Choice` | Invokes `ctIsZeroU64` and returns `Choice`. |
| `ctEqU8` | `(a: u8, b: u8): Choice` | Invokes `ctEqU8` and returns `Choice`. |
| `ctEqU32` | `(a: u32, b: u32): Choice` | Invokes `ctEqU32` and returns `Choice`. |
| `ctEqU64` | `(a: u64, b: u64): Choice` | Invokes `ctEqU64` and returns `Choice`. |
| `ctEqBytes` | `(a: &[]u8, b: &[]u8): Choice` | Invokes `ctEqBytes` and returns `Choice`. |
| `ctLtU64` | `(a: u64, b: u64): Choice` | Invokes `ctLtU64` and returns `Choice`. |
| `ctGtU64` | `(a: u64, b: u64): Choice` | Invokes `ctGtU64` and returns `Choice`. |
| `ctSelectU8` | `(a: u8, b: u8, c: Choice): u8` | Invokes `ctSelectU8` and returns `u8`. |
| `ctSelectU32` | `(a: u32, b: u32, c: Choice): u32` | Invokes `ctSelectU32` and returns `u32`. |
| `ctSelectU64` | `(a: u64, b: u64, c: Choice): u64` | Invokes `ctSelectU64` and returns `u64`. |
| `ctSelectBytes` | `(dst: []u8, a: &[]u8, b: &[]u8, c: Choice): usize` | Invokes `ctSelectBytes` and returns `usize`. |
| `ctCondSwapU64` | `(a: &mut u64, b: &mut u64, c: Choice)` | Executes `ctCondSwapU64` in-place. |
| `zeroizeBytes` | `(buf: []u8)` | Executes `zeroizeBytes` in-place. |

## Minimal Example

```cp
import { mask8 } from "subtle";

pub fn main() {
    let result = mask8(/* c: Choice */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
