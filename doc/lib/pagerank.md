# `cap:pagerank` — CSR 稀疏有向图 PageRank、个性化 PPR 与 HITS 中心性零分配库

零堆分配（Zero-Alloc）的纯 CP 稀疏有向图中心性算法库，基于经典 CSR（Compressed Sparse Row）图布局，提供标准 PageRank（含悬挂节点概率质量重分配）、个性化 Personalized PageRank（PPR）以及 Kleinberg HITS（Hubs & Authorities）双向幂迭代，适用于知识图谱排序、推荐系统随机游走、网页/引用网络分析与 Agent 依赖中心性评估。

## 核心特性

- **零堆分配（Zero-Alloc）**：全部迭代状态与临时工作区由调用方通过定长栈数组或切片传入，运行过程零动态内存分配。
- **紧凑 CSR 图表示**：`CsrGraph` 直接借用调用方的 `row_ptr` 与 `col_idx` 切片，内置 `validate()` 完整性校验、`outDegree()`、`neighbors()` 与 `numEdges()` 查询。
- **概率质量严格守恒**：`pagerank` 与 `personalizedPagerank` 自动收集所有出度为 0 的悬挂节点（Dangling Nodes）质量并按跳转分布重分配，每轮迭代严格保持 $\sum r_i = 1.0$。
- **支持任意非负个性化跳转向量**：`personalizedPagerank` 自动归一化任意非负且和大于 0 的种子权重向量 `teleport`。
- **稳定双向 HITS 迭代**：`hits` 采用 Kleinberg 交替投影（先更新 Authority $a \leftarrow A^T h$，再更新 Hub $h \leftarrow A a$）配合 L2 归一化，避免二分图奇偶振荡并安全处理无边图。
- **完备边界防护**：全面拦截非法 CSR（越界、非单调、长度不匹配）、非法阻尼系数/容差、负值或全零跳转向量以及缓冲区不足，直接返回 `null`。

## 导出 API

### 数据模型

- `CsrGraph`
  - 字段：`num_nodes: usize`, `row_ptr: []const usize`, `col_idx: []const usize`
  - `validate(): bool` — 校验 `row_ptr.len == num_nodes + 1`、`row_ptr[0] == 0`、单调非减、`row_ptr[num_nodes] <= col_idx.len` 及所有目标节点 `< num_nodes`
  - `numEdges(): usize` — 返回有向边总数
  - `outDegree(u: usize): usize` — 返回节点 `u` 的出度（越界返回 `0`）
  - `neighbors(u: usize): []const usize` — 返回节点 `u` 的出邻居切片（越界返回空切片）
- `PageRankResult { iterations: u32, converged: bool, l1_diff: f64 }`
- `HitsResult { iterations: u32, converged: bool, l1_diff: f64 }`

### 核心算法

- `pagerank(graph: CsrGraph, damping: f64, tol: f64, max_iter: u32, ranks_out: []f64, next_scratch: []f64): PageRankResult?`
  - 要求：`graph.validate() && graph.num_nodes > 0`，`0 < damping < 1`，`tol > 0`，`ranks_out.len >= n && next_scratch.len >= n`。
- `personalizedPagerank(graph: CsrGraph, teleport: []const f64, damping: f64, tol: f64, max_iter: u32, ranks_out: []f64, next_scratch: []f64): PageRankResult?`
  - 额外要求：`teleport.len >= n`，`teleport[0..n]` 全为有限非负数且和 `> 0`。
- `hits(graph: CsrGraph, tol: f64, max_iter: u32, hubs_out: []f64, auths_out: []f64, next_hubs: []f64, next_auths: []f64): HitsResult?`
  - 要求：`graph.validate() && graph.num_nodes > 0`，`tol > 0`，四个输出/临时缓冲切片长度均 `>= n`。

### 辅助函数

- `topNode(scores: []const f64): usize?` — 返回最大有限分值对应的节点下标（平局取最小下标；空切片或无有限值返回 `null`）。
- `sumScores(scores: []const f64): f64` — 使用 Kahan 补偿求和计算分值总和。

## 使用示例

```cp
import std from "std";
import {
    CsrGraph,
    pagerank,
    personalizedPagerank,
    hits,
    topNode,
    sumScores,
} from "cap:pagerank";

fn example() {
    // 4-node directed graph: 0->1, 0->2, 1->2, 2->0, 2->3 (node 3 is dangling)
    const row_ptr: [5]usize = [0, 2, 3, 5, 5];
    const col_idx: [5]usize = [1, 2, 2, 0, 3];
    const g = CsrGraph {
        num_nodes: 4,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
    };

    let ranks: [4]f64 = std.mem.zeroes([4]f64);
    let scratch: [4]f64 = std.mem.zeroes([4]f64);

    if (res := pagerank(g, 0.85, 1.0e-12, 200, ranks[0..], scratch[0..])) {
        * = res.converged;
        * = sumScores(ranks[0..]); // == 1.0
        * = topNode(ranks[0..]);   // == 2
    }

    const seed: [4]f64 = [1.0, 0.0, 0.0, 0.0];
    * = personalizedPagerank(g, seed[0..], 0.85, 1.0e-12, 200, ranks[0..], scratch[0..]);

    let hubs: [4]f64 = std.mem.zeroes([4]f64);
    let auths: [4]f64 = std.mem.zeroes([4]f64);
    let nh: [4]f64 = std.mem.zeroes([4]f64);
    let na: [4]f64 = std.mem.zeroes([4]f64);
    * = hits(g, 1.0e-12, 200, hubs[0..], auths[0..], nh[0..], na[0..]);
}
```
