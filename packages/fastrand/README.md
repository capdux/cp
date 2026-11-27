[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fastrand

Zero-allocation WyRand 64-bit PRNG with unbiased range sampling, 53-bit unit floats, Fisher-Yates shuffle, and alphanumeric generation for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fastrand` ([https://github.com/smol-rs/fastrand](https://github.com/smol-rs/fastrand))

| Metadata | Value |
| --- | --- |
| Package | `fastrand` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fastrand
```

```cp
import {
    Rng,
    addWrap64,
    mulWide64Xor,
    i64ToOrderedU64,
    orderedU64ToI64,
} from "fastrand";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Rng` | `class` | Fields: `state: u64` · Methods: `withSeed, nextU64, nextU32, nextBool, fork, u64Below, +8 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `addWrap64` | `(a: u64, b: u64): u64` | Mutates or appends state via `addWrap64`. |
| `mulWide64Xor` | `(a: u64, b: u64): u64` | Invokes `mulWide64Xor` and returns `u64`. |
| `i64ToOrderedU64` | `(v: i64): u64` | Invokes `i64ToOrderedU64` and returns `u64`. |
| `orderedU64ToI64` | `(u: u64): i64` | Invokes `orderedU64ToI64` and returns `i64`. |

## Minimal Example

```cp
import { addWrap64 } from "fastrand";

pub fn main() {
    let result = addWrap64(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
