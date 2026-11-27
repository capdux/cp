[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bvh

Zero-allocation static 3D AABB Bounding Volume Hierarchy (BVH) with overlap queries and slab raycasting in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bvh` ([https://github.com/svenstaro/bvh](https://github.com/svenstaro/bvh))

| Metadata | Value |
| --- | --- |
| Package | `bvh` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bvh
```

```cp
import {
    Aabb,
    BvhItem,
    RayHit,
    BvhNode,
    BuildTask,
    RayFrame,
    Bvh,
    minF64,
} from "bvh";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Aabb` | `class` | Fields: `min_x: f64, min_y: f64, min_z: f64, max_x: f64, max_y: f64, max_z: f64` · Methods: `merge, overlaps, containsPoint, surfaceArea, rayHit` |
| `BvhItem` | `class` | Fields: `bounds: Aabb, id: u32` |
| `RayHit` | `class` | Fields: `id: u32, t: f64, hit: bool` |
| `BvhNode` | `class` | Fields: `bounds: Aabb, left: i32, right: i32, item_id: u32, is_leaf: bool` |
| `BuildTask` | `class` | Fields: `lo: usize, hi: usize, slot: usize` |
| `RayFrame` | `class` | Fields: `idx: i32, t_enter: f64` |
| `Bvh` | `class` | Fields: `nodes: [256]BvhNode, node_count: usize, item_count: usize` · Methods: `build, len, isEmpty, queryAabb, raycast` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `minF64` | `(a: f64, b: f64): f64` | Invokes `minF64` and returns `f64`. |
| `maxF64` | `(a: f64, b: f64): f64` | Invokes `maxF64` and returns `f64`. |
| `centroidOnAxis` | `(b: Aabb, axis: u8): f64` | Invokes `centroidOnAxis` and returns `f64`. |
| `sortItemsByAxis` | `(items: []BvhItem, lo: usize, hi: usize, axis: u8)` | Executes `sortItemsByAxis` in-place. |

## Minimal Example

```cp
import { minF64 } from "bvh";

pub fn main() {
    let result = minF64(0.0, 0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
