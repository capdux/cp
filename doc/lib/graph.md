# CP 标准库：图算法与有向无环图 (DAG) 引擎手册 (`cap:graph` / `cap.graph`)

## 一、 架构总览与设计准则

`cap:graph` 是 CP 语言官方内置的纯原生图论数据结构与算法库，同时提供：
1. **稠密顶点索引有向加权图 (`DirectedGraph`)**：面向编译器依赖分析、任务调度 DAG、Tarjan 强连通分量、Dijkstra / Bellman-Ford 最短路径与传递闭包计算；
2. **字符串实体邻接图 (`Graph` / `GraphAlgo`)**：面向 AI Agent **GraphRAG** 知识图谱实体检索、k-跳子图召回、Kruskal MST 与 PageRank 中心性排序。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:graph"` 与 `import cap from "cap"; cap.graph.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/graph/graph.cp` | `// Source: cp/lib/graph/graph.cp (Weighted Directed/Undirected Adjacency Graph)` | `GraphError`、`GraphEdge`、`Graph` |
| `lib/graph/directed_graph.cp` | `// Source: cp/lib/graph/directed_graph.cp (Indexed Weighted Directed Graph & Adjacency List)` | `WeightedEdge`、`DirectedGraph` |
| `lib/graph/dag_algorithms.cp` | `// Source: cp/lib/graph/dag_algorithms.cp (DAG Topological Sort, Cycle Detection, Tarjan SCC, Shortest Paths)` | `topologicalSort`、`findCycle`、`reachableFrom`、`transitiveClosure`、`tarjanScc`、`dijkstra`、`bellmanFord`、`reconstructPath` |
| `lib/graph/algorithms.cp` | `// Source: cp/lib/graph/algorithms.cp (Dijkstra, Tarjan SCC, Topological Sort, Kruskal MST & PageRank)` | `ShortestPathResult`、`SccResult`、`MstEdge`、`MstResult`、`PageRankEntry`、`GraphAlgo` |
| `lib/graph/index.cp` | `// Source: cp/lib/graph/index.cp (Unified Exports for cap:graph)` | `cap:graph` 与 `cap.graph` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与稠密索引有向图（`graph.cp` / `directed_graph.cp`）

- **`GraphError`**：`NodeNotFound`、`InvalidVertex`、`NegativeWeight`、`CycleDetected`、`Disconnected`、`DuplicateEdge`、`OutOfMemory`
- **`WeightedEdge`**：`to: usize`、`weight: f64`
- **`DirectedGraph`**：
  - `init(alloc, vertex_count): DirectedGraph!` / `deinit(*_)`
  - `addVertex(*_): usize!`：新增孤立顶点并返回其索引
  - `addEdge(*_, u, v, weight): void!`：添加有向边 `u -> v`；越界返回 `InvalidVertex`，重复边返回 `DuplicateEdge`
  - `neighbors(u): ([]const WeightedEdge)!`
  - `inDegree(v): usize!`、`outDegree(u): usize!`
  - `vertexCount(): usize`、`edgeCount(): usize`
  - `transpose(alloc): DirectedGraph!`：构造所有边反转后的反向图

### 2. DAG 拓扑排序、环检测与可达性（`dag_algorithms.cp`）

- **`topologicalSort(alloc, graph): ([]usize)!`**：Kahn 算法；若存在环路返回 `GraphError.CycleDetected`。
- **`findCycle(alloc, graph): ([]usize)?!`**：非递归三色 DFS 环检测；若存在环路返回闭合顶点序列 `[v0, ..., v0]`，无环返回 `null`。
- **`reachableFrom(alloc, graph, start): ([]usize)!`**：BFS 计算从 `start` 出发的可达顶点集合。
- **`transitiveClosure(alloc, graph): ReachabilityMatrix!`**：计算全图传递闭包；提供 `canReach(u, v): bool` 与 `deinit(*_)`。

### 3. Tarjan 强连通分量与最短路径（`dag_algorithms.cp`）

- **`tarjanScc(alloc, graph): TarjanSccResult!`**：显式栈迭代式 Tarjan 算法（$O(V+E)$，免疫深图栈溢出），按凝聚图逆拓扑序返回 SCC 分组；提供 `count(): usize` 与 `deinit(*_)`。
- **`dijkstra(alloc, graph, source): PathResult!`**：二叉最小堆单源最短路径，遇到负权或 NaN 边返回 `GraphError.NegativeWeight`。
- **`bellmanFord(alloc, graph, source): PathResult!`**：支持负权边的单源最短路径，检测到负权环返回 `GraphError.CycleDetected`。
- **`reconstructPath(alloc, prev, source, target): ([]usize)!`**：根据 `PathResult.prev` 还原 `source -> target` 最短路径；不可达返回 `GraphError.Disconnected`。

### 4. GraphRAG 字符串邻接图与高级算法（`graph.cp` / `algorithms.cp`）

- **`Graph`**：支持字符串节点 ID 的有向/无向图，提供 `addNode`、`addEdge`、`bfs`、`dfs`、`kHopSubgraph`。
- **`GraphAlgo`**：提供 `dijkstra`、`topologicalSort`、`tarjanScc`、`kruskalMst`、`pageRank`。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import {
    DirectedGraph,
    topologicalSort,
    findCycle,
    tarjanScc,
    dijkstra,
    bellmanFord,
    reconstructPath,
} from "cap:graph";

pub fn demoDagAndShortestPath(alloc: std.mem.Allocator): !void {
    let g = DirectedGraph.init(alloc, 4)?;
    defer g.deinit();

    g.addEdge(0, 1, 2.0)?;
    g.addEdge(0, 2, 5.0)?;
    g.addEdge(1, 2, 1.0)?;
    g.addEdge(2, 3, 3.0)?;

    const order = topologicalSort(alloc, g)?;
    defer alloc.free(order);

    let sp = dijkstra(alloc, g, 0)?;
    defer sp.deinit();

    const path = reconstructPath(alloc, sp.prev, 0, 3)?;
    defer alloc.free(path);

    // 等价动态挂载用法：cap.graph.tarjanScc(...)
    let scc = cap.graph.tarjanScc(alloc, g)?;
    defer scc.deinit();
}
```
