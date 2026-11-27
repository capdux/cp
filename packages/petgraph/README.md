[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# petgraph

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:petgraph` ([https://github.com/petgraph/petgraph](https://github.com/petgraph/petgraph))（原库名 `petgraph` → CP 包名 `petgraph`）

Zero-allocation directed/undirected graph (`DiGraph`) and classic graph algorithms (BFS, iterative DFS, Kahn topological sort, cycle detection, Dijkstra shortest paths, reachability) for CP — clean-room MIT parity with Rust `petgraph` v0.6 (950M+ downloads).

纯 CP 零堆分配固定容量图引擎（64 节点 / 256 加权边），适用于依赖解析、任务 DAG 调度、路由最短路径与编译器控制流图分析。

## Install / 安装

```bash
cap add petgraph
```

## API

### Constants

| Symbol | Value | Description |
| --- | --- | --- |
| `PG_MAX_NODES` | `64` | Maximum nodes in a `DiGraph` |
| `PG_MAX_EDGES` | `256` | Maximum directed edges in a `DiGraph` |
| `PG_INF` | `0x3fffffffffffffff` | Unreachable distance sentinel for Dijkstra |

### `DiGraph`

| Method | Signature | Description |
| --- | --- | --- |
| `new` | `static fn new() -> DiGraph` | Creates an empty graph |
| `nodeCount` / `edgeCount` | `() -> usize` | Returns current node / edge counts |
| `addNode` | `(*_) -> usize?` | Adds a node and returns its ID (`null` when full) |
| `addEdge` | `(*_, from_node, to_node, weight) -> bool` | Adds a weighted directed edge |
| `addUndirectedEdge` | `(*_, a, b, weight) -> bool` | Atomically adds both `(a -> b)` and `(b -> a)` |
| `hasEdge` / `outDegree` / `inDegree` | `(u[, v]) -> bool / usize` | Edge lookup and degree queries |
| `bfs` | `(start: usize, order_out: []usize) -> usize` | Breadth-first traversal |
| `dfs` | `(start: usize, order_out: []usize) -> usize` | Iterative depth-first traversal (no recursion) |
| `topoSort` | `(order_out: []usize) -> usize?` | Kahn topological sort (`null` if cyclic) |
| `isCyclic` | `() -> bool` | Detects whether any directed cycle exists |
| `dijkstra` | `(start: usize, dist_out: []u64) -> bool` | Non-negative single-source shortest paths |
| `hasPath` | `(src: usize, dst: usize) -> bool` | Reachability check from `src` to `dst` |

## Usage / 使用示例

```cp
import { DiGraph, PG_INF } from "petgraph";

let g = DiGraph.new();
let a = g.addNode() ?? (0 as usize);
let b = g.addNode() ?? (0 as usize);
* = g.addEdge(a, b, 5 as u64);

let dist: [2]u64 = undefined;
* = g.dijkstra(a, dist[0..2]); // dist[1] == 5
```
