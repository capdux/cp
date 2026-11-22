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
| `lib/graph/dijkstra.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc CSR Shortest Paths)` | `WeightedCsrGraph`、`csrDijkstra`、`reconstructCsrPath`、`csrBellmanFord`、`floydWarshall`、`reconstructFloydPath` |
| `lib/graph/mst.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc Kruskal/Prim MST & DisjointSet)` | `MstWeightedEdge`、`PrimEdge`、`DenseMstResult`、`DisjointSet`、`kruskalMst`、`maxSpanningTree`、`primDenseMst`、`isValidSpanningTree` |
| `lib/graph/maxflow.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc Dinic Max-Flow & Min-Cost Max-Flow)` | `FlowEdge`、`FlowGraph`、`MinCostResult`、`dinicMaxFlow`、`minCutReachable`、`minCostMaxFlow` |
| `lib/graph/scc.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc CSR Tarjan SCC, Kahn TopoSort, DAG Longest Path & 2-SAT)` | `CsrGraph`、`TarjanWorkspace`、`TwoSatClause`、`csrTarjanScc`、`topoSort`、`isDag`、`dagLongestPath`、`solve2Sat` |
| `lib/graph/bipartite.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc Hopcroft-Karp Matching & Konig Vertex Cover)` | `UNMATCHED`、`BipartiteGraph`、`isBipartite`、`hopcroftKarp`、`konigMinVertexCover` |
| `lib/graph/hungarian.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc O(n^3) Hungarian Linear Assignment Solver)` | `AssignmentResultF64`/`I64`、`HungarianWorkspaceF64`/`I64`、`solveMinF64`/`solveMaxF64`/`solveMinI64`/`solveMaxI64` |
| `lib/graph/delaunay.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc 2D Bowyer-Watson Delaunay Triangulation)` | `Point2`、`Triangle`、`Edge2`、`Circumcircle`、`DelaunayWorkspace`、`triangulate`、`locateTriangle` |
| `lib/graph/voronoi.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc 2D Voronoi Cell Clipping & Lloyd Relaxation)` | `VoronoiPoint2`、`VoronoiBounds2`、`CellMetrics`、`computeVoronoiCell`、`lloydRelaxationStep`、`nearestSite` |
| `lib/graph/octree.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc 3D Octree Spatial Index & Ray-AABB Queries)` | `Vec3`、`Aabb3`、`Ray3`、`rayIntersectsAabb`、`Octree`、`NearestResult` |
| `lib/graph/rrt.cp` | `// Origin: Pure-CP Stdlib (Zero-Alloc 2D RRT Motion Planner & Path Smoothing)` | `RrtPoint2`、`RrtBounds2`、`CircleObstacle`、`RectObstacle`、`planRrt2D`、`smoothPath`、`pathLength` |
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

### 5. 零堆分配图论扩展算法集（已收敛至 `cap:graph`）

- **CSR 最短路径（`dijkstra.cp`）**：`WeightedCsrGraph`、`DijkstraWorkspace`、`csrDijkstra`、`reconstructCsrPath`、`csrBellmanFord`、`floydWarshall`、`reconstructFloydPath`（为避免与 `dag_algorithms.cp` 的 `DirectedGraph` 版本重名，CSR 版本使用 `csr*` 别名）。
- **最小/最大生成树与并查集（`mst.cp`）**：`MstWeightedEdge`、`PrimEdge`、`DenseMstResult`、`DisjointSet`、`kruskalMst`、`maxSpanningTree`、`primDenseMst`、`isValidSpanningTree`。
- **网络流与最小费用最大流（`maxflow.cp`）**：`FlowGraph`、`FlowEdge`、`dinicMaxFlow`、`minCutReachable`、`minCostMaxFlow`。
- **CSR 强连通分量与 2-SAT（`scc.cp`）**：`CsrGraph`、`TarjanWorkspace`、`TwoSatClause`、`csrTarjanScc`、`topoSort`、`isDag`、`dagLongestPath`、`solve2Sat`。
- **二分图最大匹配与最小点覆盖（`bipartite.cp`）**：`BipartiteGraph`、`isBipartite`、`hopcroftKarp`、`konigMinVertexCover`。
- **线性指派匈牙利算法（`hungarian.cp`）**：`HungarianWorkspaceF64`/`I64`、`solveMinF64`/`solveMaxF64`/`solveMinI64`/`solveMaxI64`。

### 6. 零堆分配计算几何与空间规划（已收敛至 `cap:graph`）

- **2D Delaunay 三角剖分（`delaunay.cp`）**：`Point2`、`Triangle`、`Edge2`、`Circumcircle`、`DelaunayWorkspace`、`orient2d`、`circumcircle`、`inCircumcircle`、`pointInTriangle`、`triangulate`、`verifyDelaunayEmptyCircle`、`locateTriangle`。
- **2D Voronoi 单元裁剪与 Lloyd 松弛（`voronoi.cp`）**：`VoronoiPoint2`、`VoronoiBounds2`、`CellMetrics`、`clipPolygonHalfPlane`、`polygonArea`、`polygonCentroid`、`computeVoronoiCell`、`lloydRelaxationStep`、`nearestSite`。
- **3D Octree 空间索引（`octree.cp`）**：`Vec3`、`Aabb3`、`Ray3`、`rayIntersectsAabb`、`Octree`、`OctreeNode`、`NearestResult`。
- **2D RRT 运动规划与路径平滑（`rrt.cp`）**：`RrtPoint2`、`RrtBounds2`、`CircleObstacle`、`RectObstacle`、`RrtNode`、`RrtConfig`、`RrtResult`、`planRrt2D`、`smoothPath`、`pathLength`。

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
