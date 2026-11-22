# `cap:dijkstra` — 零分配图最短路径算法库（Dijkstra / Bellman-Ford / Floyd-Warshall）

> [!NOTE]
> 本子模块已归并至统一主包 `cap:graph`，导入请使用 `import { ... } from "cap:graph"`。

零堆分配（Zero-Alloc）的纯 CP 图最短路径标准库，提供基于索引二叉最小堆（Indexed Binary Min-Heap）的 O((V + E) log V) 单源最短路 `dijkstra`、支持负权边与负权环检测的 `bellmanFord`、全源最短路与中转矩阵 `floydWarshall`，以及正序路径还原函数 `reconstructPath` / `reconstructFloydPath`。

## 核心特性

- **零堆分配（Zero-Alloc）**：所有距离数组、前驱数组、索引最小堆工作区及输出路径缓冲区均由调用方栈上或静态切片提供。
- **索引二叉最小堆（Indexed Binary Min-Heap）**：`dijkstra` 使用 `heap_pos` 反向索引实现 O(log V) 原地 `decreaseKey`，堆大小严格不超过 `num_nodes`，永不溢出。
- **负权边安全防线**：`dijkstra` 预先校验并优雅拒收任意负权边（返回 `null`）；`bellmanFord` 与 `floydWarshall` 完整支持负权边并精确报告 `has_negative_cycle`。
- **路径还原防环保护**：`reconstructPath` 与 `reconstructFloydPath` 自动拦截不可达节点、非法前驱下标、畸形环与缓冲区不足。

## 导出 API

### 常量与数据模型

- `NO_PARENT: i64 = -1`
- `isReachable(dist: f64): bool` — 判断距离值是否有限（可达）
- `Edge { from: usize, to: usize, weight: f64 }`
- `WeightedCsrGraph { num_nodes: usize, row_ptr: []const usize, col_idx: []const usize, weights: []const f64 }`
  - `validate(): bool`、`numEdges(): usize`、`outDegree(u: usize): usize`、`neighbors(u: usize): []const usize`、`edgeWeights(u: usize): []const f64`
- `DijkstraWorkspace { heap_nodes: []usize, heap_pos: []i64, visited: []u8 }`
- `DijkstraResult { settled_nodes: usize, relaxations: usize }`
- `BellmanFordResult { has_negative_cycle: bool, iterations: usize }`
- `FloydWarshallResult { has_negative_cycle: bool }`

### 核心函数

- `dijkstra(graph: WeightedCsrGraph, src: usize, dist_out: []f64, parent_out: []i64, ws: *DijkstraWorkspace): DijkstraResult?`
- `reconstructPath(parent: []const i64, src: usize, dst: usize, path_out: []usize): usize?`
- `bellmanFord(graph: WeightedCsrGraph, src: usize, dist_out: []f64, parent_out: []i64): BellmanFordResult?`
- `floydWarshall(num_nodes: usize, edges: []const Edge, dist_mat: []f64, next_mat: []i64): FloydWarshallResult?`
- `reconstructFloydPath(next_mat: []const i64, num_nodes: usize, src: usize, dst: usize, path_out: []usize): usize?`

## 使用示例

```cp
import std from "std";
import {
    WeightedCsrGraph,
    DijkstraWorkspace,
    dijkstra,
    reconstructPath,
    isReachable,
} from "cap:graph";

fn example() {
    // 4-node graph: 0->1 (2.0), 0->2 (5.0), 1->2 (1.0), 2->3 (3.0)
    const row_ptr: [5]usize = [0, 2, 3, 4, 4];
    const col_idx: [4]usize = [1, 2, 2, 3];
    const weights: [4]f64 = [2.0, 5.0, 1.0, 3.0];
    const g = WeightedCsrGraph {
        num_nodes: 4,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
        weights: weights[0..],
    };

    let dist: [4]f64 = std.mem.zeroes([4]f64);
    let parent: [4]i64 = std.mem.zeroes([4]i64);
    let hn: [4]usize = std.mem.zeroes([4]usize);
    let hp: [4]i64 = std.mem.zeroes([4]i64);
    let vis: [4]u8 = std.mem.zeroes([4]u8);
    let ws = DijkstraWorkspace { heap_nodes: hn[0..], heap_pos: hp[0..], visited: vis[0..] };

    if (res := dijkstra(g, 0, dist[0..], parent[0..], &ws)) {
        * = res.settled_nodes;
        * = isReachable(dist[3]); // true, dist[3] == 6.0
        let path: [4]usize = std.mem.zeroes([4]usize);
        * = reconstructPath(parent[0..], 0, 3, path[0..]); // [0, 1, 2, 3]
    }
}
```
