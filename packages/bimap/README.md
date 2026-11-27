[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bimap

Zero-heap bidirectional 1-to-1 map (left <-> right) in pure CP, modeled after Rust bimap

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bimap` ([https://github.com/billyrieger/bimap-rs](https://github.com/billyrieger/bimap-rs))

| Metadata | Value |
| --- | --- |
| Package | `bimap` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bimap
```

```cp
import { BiPair, BiMap } from "bimap";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BiPair` | `class` | Fields: `left: u64, right: u64` |
| `BiMap` | `class` | Fields: `pairs: [128]BiPair, count: usize` · Methods: `init, len, isEmpty, capacity, findLeftIndex, findRightIndex, +9 more` |

## Minimal Example

```cp
import { BiPair } from "bimap";

pub fn main() {
    // Refer to src/index.d.cp for BiPair fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
