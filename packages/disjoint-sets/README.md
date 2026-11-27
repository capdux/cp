[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# disjoint-sets

Zero-heap Disjoint Set Union (Union-Find) with path halving and union by size/rank for pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:disjoint-sets` ([https://github.com/tov/disjoint-sets-rs](https://github.com/tov/disjoint-sets-rs))（原库名 `disjoint-sets` → CP 包名 `disjoint-sets`）

| Metadata | Value |
| --- | --- |
| Package | `disjoint-sets` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add disjoint-sets
```

```cp
import { DisjointSet } from "disjoint-sets";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DisjointSet` | `class` | Fields: `parent: [256]u16, sizes: [256]u16, count: usize, components: usize` · Methods: `init, len, componentCount, find, findReadOnly, connected, +3 more` |

## Minimal Example

```cp
import { DisjointSet } from "disjoint-sets";

pub fn main() {
    // Refer to src/index.d.cp for DisjointSet fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
