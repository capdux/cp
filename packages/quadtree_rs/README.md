[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# quadtree_rs

Zero-allocation pooled 2D QuadTree spatial index supporting point insertion, automatic subdivision, rectangular range queries, and radius queries for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:quadtree_rs` ([https://github.com/ambuc/quadtree_rs](https://github.com/ambuc/quadtree_rs)) / `npm:d3-quadtree` ([https://github.com/d3/d3-quadtree](https://github.com/d3/d3-quadtree))（原库名 `quadtree_rs` → CP 包名 `quadtree_rs`）

| Metadata | Value |
| --- | --- |
| Package | `quadtree_rs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add quadtree_rs
```

```cp
import {
    Rect2,
    QuadItem,
    QuadNode,
    QuadTree,
    qtMakeLeaf,
    qtChildIndex,
} from "quadtree_rs";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Rect2` | `class` | Fields: `min_x: f64, min_y: f64, max_x: f64, max_y: f64` · Methods: `new, contains, intersects` |
| `QuadItem` | `class` | Fields: `x: f64, y: f64, id: u32, next_item: i32` |
| `QuadNode` | `class` | Fields: `bounds: Rect2, first_item: i32, count: u32, children: [4]i32, is_leaf: bool` |
| `QuadTree` | `class` | Fields: `node_count: usize, item_count: usize, max_leaf_items: u32, max_depth: u32` · Methods: `empty, init, insert, queryRect, queryRadius` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `qtMakeLeaf` | `(bounds: Rect2): QuadNode` | Invokes `qtMakeLeaf` and returns `QuadNode`. |
| `qtChildIndex` | `(b: Rect2, x: f64, y: f64): usize` | Invokes `qtChildIndex` and returns `usize`. |

## Minimal Example

```cp
import { qtMakeLeaf } from "quadtree_rs";

pub fn main() {
    let result = qtMakeLeaf(/* bounds: Rect2 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
