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

---

## 四、 零堆分配 DAG 拓扑波次规划与关键路径分析器（`workflow_dag.cp`）

面向构建系统、数据流水线与多智能体任务编排的 100% 纯 CP 零堆分配 DAG 规划原语：

- **导出类型**：`DagNodeCost`、`DagEdgeSpec`、`DagWaveAssignment`、`CriticalPathMetrics`、`WorkflowPlanSummary`
- **核心算法**：
  - `planParallelWaves(node_count, edges, out_order, out_waves, scratch_in_deg, scratch_queue): WorkflowPlanSummary?`：Kahn 拓扑排序 + 最早并发波次（`wave_index`）划分与最大并发度统计，遇环返回 `has_cycle = true` 及首个成环节点；
  - `analyzeCriticalPath(node_count, durations_ms, edges, topo_order, out_metrics): u64?`：正反两遍 CPM 动态规划精确计算每个节点的 `earliest_start`、`earliest_finish`、`latest_start`、`latest_finish`、`total_slack`、`is_critical` 与最短总工期（makespan）；
  - `findCycleNodes(node_count, edges, out_cycle_nodes, scratch_color, scratch_parent): usize`：三色 DFS 提取真实成环节点序列，秒级定位循环依赖。

### QuickStart 示例（菱形并行波次 + CPM 关键路径）

```cp
import {
    DagEdgeSpec,
    DagWaveAssignment,
    CriticalPathMetrics,
    planParallelWaves,
    analyzeCriticalPath,
} from "cap:graph";

pub fn demoWorkflowDagCpm() {
    // 0 (10ms) -> 1 (50ms) -> 3 (20ms)  [关键路径 = 80ms]
    // 0 (10ms) -> 2 (15ms) -> 3 (20ms)  [松弛时间 = 35ms]
    const edges: [4]DagEdgeSpec = [
        DagEdgeSpec{ from: 0, to: 1 },
        DagEdgeSpec{ from: 0, to: 2 },
        DagEdgeSpec{ from: 1, to: 3 },
        DagEdgeSpec{ from: 2, to: 3 },
    ];
    const durs: [4]u64 = [10, 50, 15, 20];
    let order: [4]u32 = [0; 4];
    let waves: [4]DagWaveAssignment = [DagWaveAssignment{}; 4];
    let in_deg: [4]u32 = [0; 4];
    let queue: [4]u32 = [0; 4];

    if (plan := planParallelWaves(4, edges[0..], order[0..], waves[0..], in_deg[0..], queue[0..])) {
        if (!plan.has_cycle) {
            let metrics: [4]CriticalPathMetrics = [CriticalPathMetrics{}; 4];
            if (makespan := analyzeCriticalPath(4, durs[0..], edges[0..], order[0..], metrics[0..])) {
                * = makespan; // 80ms; metrics[0], metrics[1], metrics[3] are critical; metrics[2].total_slack == 35
            }
        }
    }
}
```

---

## 五、 零分配 Dinic 最大流 / 最小割与二分图最大匹配求解器（`max_flow.cp`）

面向网络带宽分配、多 Agent 任务-算力二分图最优匹配与最小割图划分的 100% 纯 CP 零动态堆分配 Dinic 最大流/最小割内核（带当前弧优化 Current-Arc Optimization）：

- **常量与数据模型**：
  - `FLOW_MAX_NODES = 64`、`FLOW_MAX_EDGES = 512`（含反向残量边，最多 256 条有向用户边）、`FLOW_NO_EDGE = 0xFFFF`。
  - `DinicFlowEdge`（模块内名为 `FlowEdge`，为避免与 `maxflow.cp` 冲突在 `cap:graph` 具名导出为 `DinicFlowEdge`）：`{ to, rev, next, cap, flow }`。
  - `MaxFlowResult`：`{ valid, max_flow, bfs_phases, source_cut_mask }`，提供 `isInSourceCut(node: usize): bool` 精确判定节点是否位于最小 s-t 割的源侧集合 `S`。
  - `BipartiteMatchPair`：`{ left_idx: u16, right_idx: u16 }`。
- **核心求解器**：
  - `DinicFlowNetwork`：`init(node_count)`、`add_edge(*_, u, v, cap)`（自动写入正/反残量边对）、`solve(*_, source, sink): MaxFlowResult`（BFS 分层图 + 当前弧优化有界 DFS 阻塞流 + 残量图源可达集最小割导出）、`edge_flow(edge_pair_idx): i64`。
  - `solveBipartiteMatching(left_count, right_count, edges_left, edges_right, out_pairs): usize`：基于单位容量 Dinic 与 O(1) 64-bit 边去重求解最大基数二分图匹配。

### QuickStart 示例

```cp
import {
    BipartiteMatchPair,
    DinicFlowNetwork,
    solveBipartiteMatching,
} from "cap:graph";

pub fn demoDinicMaxFlowAndMatching() {
    // 1. 经典 CLRS 6 节点网络最大流 (max_flow = 23) 与最小割
    let net = DinicFlowNetwork.init(6) ?? return;
    * = net.add_edge(0, 1, 16);
    * = net.add_edge(0, 2, 13);
    * = net.add_edge(1, 2, 10);
    * = net.add_edge(2, 1, 4);
    * = net.add_edge(1, 3, 12);
    * = net.add_edge(3, 2, 9);
    * = net.add_edge(2, 4, 14);
    * = net.add_edge(4, 3, 7);
    * = net.add_edge(3, 5, 20);
    * = net.add_edge(4, 5, 4);
    const res = net.solve(0, 5); // res.max_flow == 23, res.isInSourceCut(0) == true, res.isInSourceCut(5) == false

    // 2. 多 Agent 任务-Worker 二分图最大匹配
    const el: [4]u16 = [0, 0, 1, 2];
    const er: [4]u16 = [0, 1, 0, 2];
    let pairs: [3]BipartiteMatchPair = [BipartiteMatchPair{}; 3];
    let pairs_s: []BipartiteMatchPair = pairs[0..];
    const matched = solveBipartiteMatching(3, 3, el[0..], er[0..], pairs_s); // matched == 3
    * = res;
    * = matched;
}
```

---

## 收敛子模块深度文档

- [bipartite.md](graph/bipartite.md)
- [delaunay.md](graph/delaunay.md)
- [dijkstra.md](graph/dijkstra.md)
- [hungarian.md](graph/hungarian.md)
- [maxflow.md](graph/maxflow.md)
- [mst.md](graph/mst.md)
- [octree.md](graph/octree.md)
- [rrt.md](graph/rrt.md)
- [scc.md](graph/scc.md)
- [voronoi.md](graph/voronoi.md)
