[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# num-rational

Zero-allocation exact rational number (Ratio64) arithmetic library with canonical reduction via num-integer for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-rational` ([https://github.com/rust-num/num-rational](https://github.com/rust-num/num-rational))

| Metadata | Value |
| --- | --- |
| Package | `num-rational` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add num-rational
```

```cp
import { Ratio64, nratAbsU64, nratReduce } from "num-rational";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Ratio64` | `class` | Fields: `numer: i64, denom: i64` · Methods: `new, fromInt, zero, one, isInteger, isZero, +14 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nratAbsU64` | `(v: i64): u64` | Invokes `nratAbsU64` and returns `u64`. |
| `nratReduce` | `(numer: i64, denom: i64): Ratio64` | Invokes `nratReduce` and returns `Ratio64`. |

## Minimal Example

```cp
import { nratAbsU64 } from "num-rational";

pub fn main() {
    let result = nratAbsU64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
