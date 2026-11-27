[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# constant_time_eq

Zero-allocation constant-time cryptographic comparison, branchless selection/swap, and secure memory zeroization primitives (Rust subtle + zeroize parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:constant_time_eq` ([https://github.com/cesarb/constant_time_eq](https://github.com/cesarb/constant_time_eq))（原库名 `constant_time_eq` → CP 包名 `constant_time_eq`）

| Metadata | Value |
| --- | --- |
| Package | `constant_time_eq` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add constant_time_eq
```

```cp
import {
    Choice,
    maskU8,
    maskU32,
    maskU64,
    ctIsZeroU8,
    ctEqU8,
    ctEqU32,
    ctEqU64,
} from "constant_time_eq";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Choice` | `class` | Fields: `val: u8` · Methods: `fromU8, fromBool, toBool, ctNot, ctAnd, ctOr, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `maskU8` | `(c: Choice): u8` | Invokes `maskU8` and returns `u8`. |
| `maskU32` | `(c: Choice): u32` | Invokes `maskU32` and returns `u32`. |
| `maskU64` | `(c: Choice): u64` | Invokes `maskU64` and returns `u64`. |
| `ctIsZeroU8` | `(x: u8): Choice` | Invokes `ctIsZeroU8` and returns `Choice`. |
| `ctEqU8` | `(a: u8, b: u8): Choice` | Invokes `ctEqU8` and returns `Choice`. |
| `ctEqU32` | `(a: u32, b: u32): Choice` | Invokes `ctEqU32` and returns `Choice`. |
| `ctEqU64` | `(a: u64, b: u64): Choice` | Invokes `ctEqU64` and returns `Choice`. |
| `ctLtU64` | `(a: u64, b: u64): Choice` | Invokes `ctLtU64` and returns `Choice`. |
| `ctGtU64` | `(a: u64, b: u64): Choice` | Invokes `ctGtU64` and returns `Choice`. |
| `ctSelectU8` | `(c: Choice, when_true: u8, when_false: u8): u8` | Invokes `ctSelectU8` and returns `u8`. |
| `ctSelectU32` | `(c: Choice, when_true: u32, when_false: u32): u32` | Invokes `ctSelectU32` and returns `u32`. |
| `ctSelectU64` | `(c: Choice, when_true: u64, when_false: u64): u64` | Invokes `ctSelectU64` and returns `u64`. |
| `ctSwapU64` | `(c: Choice, a: &mut u64, b: &mut u64)` | Executes `ctSwapU64` in-place. |
| `ctEqBytes` | `(a: string, b: string): Choice` | Invokes `ctEqBytes` and returns `Choice`. |
| `ctEqFixed32` | `(a: [32]u8, b: [32]u8): Choice` | Invokes `ctEqFixed32` and returns `Choice`. |
| `ctCopyIf` | `(c: Choice, dst: []u8, src: string)` | Executes `ctCopyIf` in-place. |
| `zeroizeBytes` | `(buf: []u8)` | Executes `zeroizeBytes` in-place. |
| `isAllZero` | `(buf: string): Choice` | Checks predicate `isAllZero` and returns `bool`. |

## Minimal Example

```cp
import { isAllZero } from "constant_time_eq";

pub fn main() {
    let result = isAllZero("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
