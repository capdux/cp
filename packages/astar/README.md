[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# astar

Zero-heap 2D grid A* shortest pathfinding engine with 4-way Manhattan & 8-way Octile diagonal search and path reconstruction.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:astar` ([https://github.com/evenfurther/pathfinding](https://github.com/evenfurther/pathfinding))

| Metadata | Value |
| --- | --- |
| Package | `astar` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add astar
```

```cp
import {
    GridPos,
    AStarResult,
    GridMap,
    absDiffUsize,
    heuristic,
} from "astar";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GridPos` | `class` | Fields: `x: u16, y: u16` |
| `AStarResult` | `class` | Fields: `found: bool, cost: i64, path_len: usize` |
| `GridMap` | `class` | Fields: `width: usize, height: usize, blocked: [1024]bool` · Methods: `init, setBlocked, isBlocked, findPath` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absDiffUsize` | `(a: usize, b: usize): i64` | Invokes `absDiffUsize` and returns `i64`. |
| `heuristic` | `(x1: usize, y1: usize, x2: usize, y2: usize, allow_diagonal: bool): i64` | Invokes `heuristic` and returns `i64`. |

## Minimal Example

```cp
import { absDiffUsize } from "astar";

pub fn main() {
    let result = absDiffUsize(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
