[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# topological-sort

Zero-allocation deterministic Kahn topological sort, cycle detection, and parallel DAG level stratification (Rust topological-sort / petgraph / npm toposort parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:topological-sort` ([https://github.com/gifnksm/topological-sort-rs](https://github.com/gifnksm/topological-sort-rs))（原库名 `topological-sort` → CP 包名 `topological-sort`）

| Metadata | Value |
| --- | --- |
| Package | `topological-sort` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add topological-sort
```

```cp
import { TopoResult, TopoGraph, clampNodes } from "topological-sort";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TopoResult` | `class` | Fields: `ok: bool, count: usize, order: [MAX_NODES]u16, levels: [MAX_NODES]u16` · Methods: `empty` |
| `TopoGraph` | `class` | Fields: `node_count: usize, edge_count: usize, head: [MAX_NODES]i16, edge_to: [MAX_EDGES]u16, edge_next: [MAX_EDGES]i16, in_deg: [MAX_NODES]u16, +1 more` · Methods: `init, reset, add_edge, in_degree, out_degree, sort, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `clampNodes` | `(n: usize): usize` | Invokes `clampNodes` and returns `usize`. |

## Minimal Example

```cp
import { clampNodes } from "topological-sort";

pub fn main() {
    let result = clampNodes(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
