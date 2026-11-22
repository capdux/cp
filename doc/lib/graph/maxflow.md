# `cap:maxflow` — 零堆分配网络流图算法库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:graph`，导入请使用 `import { ... } from "cap:graph"`。

`cap:maxflow` 提供纯 CP 实现、完全零堆分配（zero-alloc）的网络流算法套件，基于静态链式前向星边池（caller-owned slices）构建，适用于实时路由、任务分配、二分图最大匹配、图像分割（最小割）及最小费用流调度等高性能场景。

## 核心特性

- **零堆分配**：图邻接表、静态边池及算法工作区均由调用方以切片形式传入，无任何动态内存分配。
- **Dinic 最大流（`dinicMaxFlow`）**：BFS 分层图 + DFS 阻塞流 + 当前弧优化，在一般图上 $O(V^2 E)$、单位容量与二分图匹配上 $O(E \sqrt{V})$。
- **最小割 S-T 划分（`minCutReachable`）**：在最大流残量网络上从源点 BFS 标记 S 侧可达节点（`1`）与 T 侧节点（`0`），满足最大流-最小割定理。
- **最小费用最大流 MCMF（`minCostMaxFlow`）**：基于环形队列 SPFA 连续最短增广路，支持负费用残量退流，内置负权环防护。
- **幂等复用**：支持 `FlowGraph.resetFlows()` 将所有边流量清零，零成本重复求解。

---

## 导出清单

### 常量

| 名称 | 类型 | 说明 |
| :--- | :--- | :--- |
| `NO_EDGE` | `i32` | 链式前向星邻接链表末尾哨兵（`-1`） |
| `INF_FLOW` | `i64` | 防溢出的无穷大流量 / 费用哨兵（`0x3FFFFFFFFFFFFFFF`） |

### 数据结构

#### `FlowEdge`
```typescript
pub struct FlowEdge {
    to: usize,
    rev_next: i32,
    cap: i64,
    flow: i64,
    cost: i64,

    fn residual(): i64;
}
```
正向边与配对反向残量边成对存储在 `edges[2k]` 与 `edges[2k + 1]`，通过 `idx ^ 1` 互查。

#### `FlowGraph`
```typescript
pub struct FlowGraph {
    num_nodes: usize,
    head: []i32,
    edges: []FlowEdge,
    edge_count: usize,

    fn init(num_nodes: usize, head: []i32, edges: []FlowEdge): FlowGraph?;
    fn addEdge(*_, u: usize, v: usize, cap: i64): bool;
    fn addCostEdge(*_, u: usize, v: usize, cap: i64, cost: i64): bool;
    fn resetFlows(*_);
}
```
- `head` 长度必须 `>= num_nodes`；`edges` 长度必须 `>= 2 * max_edges`。
- `addEdge` / `addCostEdge` 自动校验顶点范围、`cap >= 0` 以及边池剩余容量，非法时返回 `false`。

#### `MinCostResult`
```typescript
pub struct MinCostResult {
    flow: i64,
    cost: i64,
}
```

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `dinicMaxFlow(g: *FlowGraph, source: usize, sink: usize, level_scratch: *[]i32, iter_scratch: *[]i32, queue_scratch: *[]usize): i64?` | 计算 `source -> sink` 最大流；参数非法或工作区不足返回 `null` |
| `minCutReachable(g: FlowGraph, source: usize, reachable_out: *[]u8, queue_scratch: *[]usize): usize?` | 最大流后在残量网络上 BFS 提取 S 侧可达节点（标记 `1`，T 侧 `0`），返回 S 侧节点数 |
| `minCostMaxFlow(g: *FlowGraph, source: usize, sink: usize, dist_scratch: *[]i64, prev_node: *[]i32, prev_edge: *[]i32, in_queue: *[]u8, queue_scratch: *[]usize): MinCostResult?` | 计算最小费用最大流（SPFA 连续最短增广路），检测到可达负权环或参数非法时返回 `null` |

---

## 使用示例

```typescript
import std from "std";
import {
    FlowEdge,
    FlowGraph,
    dinicMaxFlow,
    minCutReachable,
    minCostMaxFlow,
} from "cap:graph";

fn solveExample() {
    let head: [6]i32 = std.mem.zeroes([6]i32);
    let edges: [32]FlowEdge = std.mem.zeroes([32]FlowEdge);
    let g = FlowGraph.init(6, head[0..], edges[0..]) ?? return;

    * = g.addEdge(0, 1, 16);
    * = g.addEdge(0, 2, 13);
    * = g.addEdge(1, 2, 10);
    * = g.addEdge(2, 1, 4);
    * = g.addEdge(1, 3, 12);
    * = g.addEdge(2, 4, 14);
    * = g.addEdge(3, 2, 9);
    * = g.addEdge(4, 3, 7);
    * = g.addEdge(3, 5, 20);
    * = g.addEdge(4, 5, 4);

    let level_buf: [6]i32 = std.mem.zeroes([6]i32);
    let iter_buf: [6]i32 = std.mem.zeroes([6]i32);
    let queue_buf: [6]usize = std.mem.zeroes([6]usize);
    let level_s: []i32 = level_buf[0..];
    let iter_s: []i32 = iter_buf[0..];
    let queue_s: []usize = queue_buf[0..];

    const max_flow = dinicMaxFlow(&g, 0, 5, &level_s, &iter_s, &queue_s) ?? 0;
    // max_flow == 23

    let reach_buf: [6]u8 = std.mem.zeroes([6]u8);
    let reach_s: []u8 = reach_buf[0..];
    const s_count = minCutReachable(g, 0, &reach_s, &queue_s) ?? 0;
    // s_count == 4 (S = {0, 1, 2, 4}, T = {3, 5})
    * = max_flow;
    * = s_count;
}
```
