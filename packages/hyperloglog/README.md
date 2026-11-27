[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hyperloglog

Zero-allocation 256-byte HyperLogLog cardinality estimator with LinearCounting small-range correction and distributed merge (Rust hyperloglog / Redis PFADD/PFCOUNT/PFMERGE parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hyperloglog` ([https://github.com/jedisct1/rust-hyperloglog](https://github.com/jedisct1/rust-hyperloglog))

| Metadata | Value |
| --- | --- |
| Package | `hyperloglog` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hyperloglog
```

```cp
import { HyperLogLog256, hashBytes64, hashU64, clz56 } from "hyperloglog";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HyperLogLog256` | `class` | Fields: `regs: [HLL_M]u8` · Methods: `init, clear, registerCount, insertHash, add, addU64, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hashBytes64` | `(data: string): u64` | Checks predicate `hashBytes64` and returns `bool`. |
| `hashU64` | `(val: u64): u64` | Checks predicate `hashU64` and returns `bool`. |
| `clz56` | `(w: u64): u8` | Invokes `clz56` and returns `u8`. |

## Minimal Example

```cp
import { hashBytes64 } from "hyperloglog";

pub fn main() {
    let result = hashBytes64("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
