[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# kiddo

Zero-allocation static 2D KD-Tree spatial index with exact nearest-neighbor and radius range queries in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:kiddo` ([https://github.com/sdd/kiddo](https://github.com/sdd/kiddo))（原库名 `kiddo` → CP 包名 `kiddo`）

| Metadata | Value |
| --- | --- |
| Package | `kiddo` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add kiddo
```

```cp
import {
    KdPoint2D,
    NearestResult,
    KdNode,
    BuildTask,
    SearchFrame,
    KdTree2D,
    coordAt,
    sortRangeByAxis,
} from "kiddo";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `KdPoint2D` | `class` | Fields: `x: f64, y: f64, id: u32` |
| `NearestResult` | `class` | Fields: `id: u32, dist_sq: f64, found: bool` |
| `KdNode` | `class` | Fields: `x: f64, y: f64, id: u32, axis: u8, left: i32, right: i32` |
| `BuildTask` | `class` | Fields: `lo: usize, hi: usize, depth: usize, slot: usize` |
| `SearchFrame` | `class` | Fields: `idx: i32, plane_dist_sq: f64` |
| `KdTree2D` | `class` | Fields: `nodes: [256]KdNode, node_count: usize` · Methods: `build, len, isEmpty, nearest, withinRadius` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `coordAt` | `(p: KdPoint2D, axis: u8): f64` | Invokes `coordAt` and returns `f64`. |
| `sortRangeByAxis` | `(pts: []KdPoint2D, lo: usize, hi: usize, axis: u8)` | Executes `sortRangeByAxis` in-place. |

## Minimal Example

```cp
import { coordAt } from "kiddo";

pub fn main() {
    let result = coordAt(/* p: KdPoint2D */, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
