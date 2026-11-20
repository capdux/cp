# CP 标准库：图论算法与知识图谱 (GraphRAG) 引擎手册 (`cap:graph` / `cap.graph`)

## 一、 架构总览与设计准则

`cap:graph` 是 CP 语言官方内置的纯原生图论数据结构与算法库，专为 AI Agent **GraphRAG** 知识图谱实体关联检索、依赖图拓扑分析、最优路径路由、强连通分量分解与节点中心性排序打造。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:graph"` 与 `import cap from "cap"; cap.graph.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/graph/graph.cp` | `// Source: cp/lib/graph/graph.cp (Weighted Directed/Undirected Adjacency Graph)` | `GraphError`、`GraphEdge`、`Graph`（邻接表建图、BFS/DFS、k-跳子图召回） |
| `lib/graph/algorithms.cp` | `// Source: cp/lib/graph/algorithms.cp (Dijkstra, Tarjan SCC, Topological Sort, Kruskal MST & PageRank)` | `ShortestPathResult`、`SccResult`、`MstEdge`、`MstResult`、`PageRankEntry`、`GraphAlgo` |
| `lib/graph/index.cp` | `// Source: cp/lib/graph/index.cp (Unified Exports for cap:graph)` | `cap:graph` 与 `cap.graph` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 加权邻接图（`graph.cp`）

- **`GraphError`**：`NodeNotFound`、`NegativeWeight`、`CycleDetected`、`OutOfMemory`
- **`GraphEdge`**：`to: usize`、`weight: f64`、`label: string`
- **`Graph`**：
  - 构造与销毁：`init(alloc, directed: bool): Graph`、`deinit(*_)`
  - 节点管理：`addNode(*_, id): usize!`（幂等返回节点稠密索引并深拷贝节点 ID）、`findNode(id): usize?`、`nodeId(idx): string?`、`nodeCount(): usize`
  - 边与邻接查询：`addEdge(*_, src, dst, weight, label = ""): !`（无向图自动添加双向对称边）、`neighbors(id): ([]const GraphEdge)!`、`neighborsAt(idx): []const GraphEdge`、`edgeCount(): usize`、`isDirected(): bool`
  - 图遍历与 GraphRAG 召回：
    - `bfs(alloc, start): ([]string)!`
    - `dfs(alloc, start): ([]string)!`
    - `kHopSubgraph(alloc, start, max_hops): ([]string)!`（按跳数逐层扩展的 k-跳实体子图召回）

### 2. 图算法引擎（`algorithms.cp`）

- **`ShortestPathResult`**：`found: bool`、`total_cost: f64`、`path: []string`、`deinit(*_)`
- **`SccResult`**：`components: [][]string`、`count(): usize`、`deinit(*_)`
- **`MstEdge` / `MstResult`**：`MstEdge{ from, to, weight, label }`；`MstResult{ total_weight, edges, deinit(*_) }`
- **`PageRankEntry`**：`id: string`、`score: f64`
- **`GraphAlgo`**：
  - `dijkstra(alloc, g, start, goal): ShortestPathResult!`：基于二叉最小堆的 Dijkstra 最短路径，显式拦截负权/NaN 边（`GraphError.NegativeWeight`）并返回完整路径与总代价。
  - `topologicalSort(alloc, g): ([]string)!`：Kahn 拓扑排序；存在环路时返回 `GraphError.CycleDetected`。
  - `tarjanScc(alloc, g): SccResult!`：迭代式 Tarjan 线性时间 $O(V+E)$ 强连通分量分解，免疫深层递归栈溢出。
  - `kruskalMst(alloc, g): MstResult!`：并查集（路径压缩 + 按秩合并）实现 Kruskal 最小生成树。
  - `pageRank(alloc, g, damping = 0.85, iterations = 20): ([]PageRankEntry)!`：计算带悬垂节点质量重分配的 PageRank 中心性得分，按得分降序排列返回，用于 GraphRAG 核心实体重要度排序。

---

## 三、 使用示例

```typescript
import cap from "cap";
import { Graph, GraphAlgo } from "cap:graph";

pub fn demoGraphRag(): !void {
    const alloc = std.heap.page_allocator;

    let kg = Graph.init(alloc, true);
    defer kg.deinit();

    * = kg.addNode("LLM")?;
    * = kg.addNode("RAG")?;
    * = kg.addNode("GraphRAG")?;
    kg.addEdge("LLM", "RAG", 1.0, "uses")?;
    kg.addEdge("RAG", "GraphRAG", 1.5, "evolves_to")?;

    const sub = kg.kHopSubgraph(alloc, "LLM", 2)?;
    defer alloc.free(sub);

    let sp = GraphAlgo.dijkstra(alloc, kg, "LLM", "GraphRAG")?;
    defer sp.deinit();

    // 等价动态挂载用法：cap.graph.GraphAlgo.pageRank(...)
    const pr = cap.graph.GraphAlgo.pageRank(alloc, kg, 0.85, 25)?;
    defer alloc.free(pr);
}
```
