[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pathfinding

Zero-allocation weighted shortest-path engine with Dijkstra, Bellman-Ford negative-cycle detection, and path reconstruction (Rust pathfinding / petgraph parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pathfinding` ([https://github.com/evenfurther/pathfinding](https://github.com/evenfurther/pathfinding))（原库名 `pathfinding` → CP 包名 `pathfinding`）

| Metadata | Value |
| --- | --- |
| Package | `pathfinding` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pathfinding
```

```cp
import { ShortestPaths, WeightedGraph, emptyShortestPaths } from "pathfinding";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ShortestPaths` | `class` | Fields: `ok: bool, node_count: usize, source: usize, dist: [MAX_NODES]i64, prev: [MAX_NODES]i16` · Methods: `isReachable, distanceTo, reconstructPath` |
| `WeightedGraph` | `class` | Fields: `node_count: usize, edge_count: usize, head: [MAX_NODES]i16, edge_src: [MAX_EDGES]u16, edge_dst: [MAX_EDGES]u16, edge_weight: [MAX_EDGES]i64, +1 more` · Methods: `init, nodeCount, edgeCount, addEdge, addUndirectedEdge, reset, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `emptyShortestPaths` | `(n_nodes: usize, src: usize, ok_val: bool): ShortestPaths` | Invokes `emptyShortestPaths` and returns `ShortestPaths`. |

## Minimal Example

```cp
import { emptyShortestPaths } from "pathfinding";

pub fn main() {
    let result = emptyShortestPaths(0, 0, false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
