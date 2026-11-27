[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# pathfinding

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pathfinding` ([https://github.com/evenfurther/pathfinding](https://github.com/evenfurther/pathfinding))（原库名 `pathfinding` → CP 包名 `pathfinding`）

Zero-allocation weighted shortest-path engine (Dijkstra + Bellman-Ford with negative-cycle detection + path reconstruction) for CP — clean-room MIT parity with Rust `pathfinding` and `petgraph::algo::dijkstra`.

纯 CP 零堆分配静态图加权最短路径引擎（压缩前向星邻接表，最多 64 节点 / 256 边），支持非负权 Dijkstra、负权边 Bellman-Ford 与可达负环检测及前驱路径回溯。

## Install / 安装

```bash
cap add pathfinding
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `ShortestPaths` | `distanceTo(target) -> i64`, `isReachable(target) -> bool`, `reconstructPath(target, out) -> usize` | Shortest-path result & forward path reconstruction |
| `WeightedGraph` | `init(n_nodes)`, `addEdge(*_, src, dst, w)`, `addUndirectedEdge(*_, u, v, w)`, `nodeCount()`, `edgeCount()`, `reset(*_, n_nodes)` | Fixed-capacity forward-star weighted graph |
| `dijkstra` | `(source: usize) -> ShortestPaths` | Non-negative weighted shortest paths |
| `bellmanFord` | `(source: usize) -> ShortestPaths` | Handles negative weights and sets `ok = false` on reachable negative cycles |

## Usage / 使用示例

```cp
import { WeightedGraph } from "pathfinding";

let g = WeightedGraph.init(4 as usize);
* = g.addEdge(0 as usize, 1 as usize, 4 as i64);
* = g.addEdge(0 as usize, 2 as usize, 1 as i64);
* = g.addEdge(2 as usize, 1 as usize, 2 as i64);
* = g.addEdge(1 as usize, 3 as usize, 1 as i64);

const sp = g.dijkstra(0 as usize);
const d = sp.distanceTo(3 as usize); // 4
```
