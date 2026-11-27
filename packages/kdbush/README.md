[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# kdbush

Pure CP zero-allocation static 2D K-D Tree with balanced median-split build, hyperplane-pruned nearest neighbor, rectangle and radius queries (kdbush / kiddo parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kdbush` ([https://github.com/mourner/kdbush](https://github.com/mourner/kdbush))（原库名 `kdbush` → CP 包名 `kdbush`）

| Metadata | Value |
| --- | --- |
| Package | `kdbush` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add kdbush
```

```cp
import {
    KdPt,
    KdNode,
    NearestResult,
    KdBuildFrame,
    KdSearchFrame,
    kdCoord,
    kdSwap,
    kdInsertionSort,
} from "kdbush";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `KdPt` | `class` | Fields: `x: f64, y: f64, id: u32` |
| `KdNode` | `class` | Fields: `pt: KdPt, left: i32, right: i32, axis: u8` |
| `NearestResult` | `class` | Fields: `found: bool, id: u32, dist_sq: f64` |
| `KdBuildFrame` | `class` | Fields: `lo: usize, hi: usize, axis: u8, parent: i32, is_left: bool` |
| `KdSearchFrame` | `class` | Fields: `idx: i32, min_dist_sq: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `kdCoord` | `(pt: KdPt, axis: u8): f64` | Invokes `kdCoord` and returns `f64`. |
| `kdSwap` | `(pts: []KdPt, i: usize, j: usize)` | Executes `kdSwap` in-place. |
| `kdInsertionSort` | `(pts: []KdPt, lo: usize, hi: usize, axis: u8)` | Executes `kdInsertionSort` in-place. |
| `kdSelectMedian` | `(pts: []KdPt, lo_in: usize, hi_in: usize, target: usize, axis: u8)` | Executes `kdSelectMedian` in-place. |
| `buildKdTree` | `(pts: []KdPt, nodes: []KdNode): i32` | Invokes `buildKdTree` and returns `i32`. |
| `nearestNeighbor` | `(nodes: []KdNode, root: i32, qx: f64, qy: f64): NearestResult` | Invokes `nearestNeighbor` and returns `NearestResult`. |
| `rangeQueryRect` | `(nodes: []KdNode, root: i32, min_x: f64, min_y: f64, max_x: f64, max_y: f64, out_ids: []u32): usize` | Invokes `rangeQueryRect` and returns `usize`. |
| `radiusQuery` | `(nodes: []KdNode, root: i32, cx: f64, cy: f64, radius: f64, out_ids: []u32): usize` | Invokes `radiusQuery` and returns `usize`. |

## Minimal Example

```cp
import { kdCoord } from "kdbush";

pub fn main() {
    let result = kdCoord(/* pt: KdPt */, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
