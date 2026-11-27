[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# petgraph

Zero-allocation directed/undirected graph and classic graph algorithms (BFS, iterative DFS, Kahn topoSort, cycle detection, Dijkstra) (Rust petgraph v0.6 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:petgraph` ([https://github.com/petgraph/petgraph](https://github.com/petgraph/petgraph))（原库名 `petgraph` → CP 包名 `petgraph`）

| Metadata | Value |
| --- | --- |
| Package | `petgraph` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add petgraph
```

```cp
import { DiGraph, pgcSatAdd } from "petgraph";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DiGraph` | `class` | Fields: `node_count: usize, edge_count: usize, edge_from: [256]usize, edge_to: [256]usize, edge_weight: [256]u64, head: [64]usize, +4 more` · Methods: `new, nodeCount, edgeCount, addNode, addEdge, addUndirectedEdge, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pgcSatAdd` | `(a: u64, b: u64): u64` | Invokes `pgcSatAdd` and returns `u64`. |

## Minimal Example

```cp
import { pgcSatAdd } from "petgraph";

pub fn main() {
    let result = pgcSatAdd(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
