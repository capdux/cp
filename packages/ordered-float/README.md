[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ordered-float

Zero-heap IEEE-754 total ordering, canonical hashing, and NaN-safe min/max/clamp for f64 and f32 (clean-room port of Rust ordered-float)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ordered-float` ([https://github.com/reem/rust-ordered-float](https://github.com/reem/rust-ordered-float))

| Metadata | Value |
| --- | --- |
| Package | `ordered-float` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ordered-float
```

```cp
import {
    OrderedF64,
    NotNanF64,
    isNanF64,
    isInfiniteF64,
    isFiniteF64,
    cmpOrderedF64,
    eqOrderedF64,
    hashOrderedF64,
} from "ordered-float";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OrderedF64` | `class` | Fields: `val: f64` · Methods: `new, intoInner, isNan, isFinite, cmp, eq, +4 more` |
| `NotNanF64` | `class` | Fields: `val: f64` · Methods: `new, intoInner, cmp, eq, hash` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isNanF64` | `(x: f64): bool` | Checks predicate `isNanF64` and returns `bool`. |
| `isInfiniteF64` | `(x: f64): bool` | Checks predicate `isInfiniteF64` and returns `bool`. |
| `isFiniteF64` | `(x: f64): bool` | Checks predicate `isFiniteF64` and returns `bool`. |
| `cmpOrderedF64` | `(a: f64, b: f64): i32` | Invokes `cmpOrderedF64` and returns `i32`. |
| `eqOrderedF64` | `(a: f64, b: f64): bool` | Invokes `eqOrderedF64` and returns `bool`. |
| `hashOrderedF64` | `(x: f64): u64` | Checks predicate `hashOrderedF64` and returns `bool`. |
| `sortOrderedF64` | `(arr: []f64)` | Executes `sortOrderedF64` in-place. |

## Minimal Example

```cp
import { isNanF64 } from "ordered-float";

pub fn main() {
    let result = isNanF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
