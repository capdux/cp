# `cap:bipartite` — 零堆分配二分图与 Hopcroft-Karp 最大匹配库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:graph`，导入请使用 `import { ... } from "cap:graph"`。

`cap:bipartite` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的二分图判定、Hopcroft-Karp $O(E\sqrt{V})$ 最大基数匹配以及基于 Kőnig 定理的最小点覆盖与最大独立集提取算法。

## 核心特性

- **100% 零堆分配**：基于压缩稀疏行（CSR）邻接表与调用方提供的切片工作区运行。
- **二分性判定（`isBipartite`）**：BFS 二染色支持多连通分量，自动检测奇环与自环。
- **Hopcroft-Karp 最大匹配（`hopcroftKarp`）**：多源 BFS 分层 + DFS 顶点不相交最短增广路，$O(E\sqrt{V})$ 时间复杂度，输出左右双向匹配表。
- **Kőnig 最小点覆盖与最大独立集（`konigMinVertexCover`）**：从未匹配左部点出发执行交替路径 BFS，精确提取满足 $|MVC| = |MaxMatching|$ 的最小点覆盖（`1`）及其补集最大独立集（`0`）。

---

## 导出清单

| 符号 | 签名 / 说明 |
| :--- | :--- |
| `UNMATCHED` | `i32 = -1`，未匹配顶点哨兵 |
| `BipartiteGraph` | 左到右 CSR 二分图 `{ num_left, num_right, row_ptr, col_idx }`，附 `init` / `validate` |
| `isBipartite(num_nodes, row_ptr, col_idx, color_out: *[]i8, queue_scratch: *[]usize): bool` | 通用无向 CSR 图 BFS 二染色判定 |
| `hopcroftKarp(g: BipartiteGraph, match_left_out: *[]i32, match_right_out: *[]i32, dist_scratch: *[]u32, queue_scratch: *[]usize): usize?` | $O(E\sqrt{V})$ 最大基数匹配 |
| `konigMinVertexCover(g: BipartiteGraph, match_left, match_right, cover_left_out: *[]u8, cover_right_out: *[]u8, vis_left_scratch: *[]u8, vis_right_scratch: *[]u8, queue_scratch: *[]usize): usize?` | Kőnig 最小点覆盖提取（返回覆盖点数，补集即为最大独立集） |

---

## 使用示例

```typescript
import std from "std";
import {
    BipartiteGraph,
    hopcroftKarp,
    konigMinVertexCover,
} from "cap:graph";

fn matchingDemo() {
    // 3x3 bipartite graph: L0->{R0,R1}, L1->{R1}, L2->{R1,R2}
    const row_ptr: [4]usize = [0, 2, 3, 5];
    const col_idx: [5]usize = [0, 1, 1, 1, 2];
    const g = BipartiteGraph.init(3, 3, row_ptr[0..], col_idx[0..]) ?? return;

    let ml: [3]i32 = std.mem.zeroes([3]i32);
    let mr: [3]i32 = std.mem.zeroes([3]i32);
    let dist: [3]u32 = std.mem.zeroes([3]u32);
    let q: [3]usize = std.mem.zeroes([3]usize);
    let ml_s: []i32 = ml[0..];
    let mr_s: []i32 = mr[0..];
    let d_s: []u32 = dist[0..];
    let q_s: []usize = q[0..];

    const m = hopcroftKarp(g, &ml_s, &mr_s, &d_s, &q_s) ?? 0;

    let cl: [3]u8 = std.mem.zeroes([3]u8);
    let cr: [3]u8 = std.mem.zeroes([3]u8);
    let vl: [3]u8 = std.mem.zeroes([3]u8);
    let vr: [3]u8 = std.mem.zeroes([3]u8);
    let cl_s: []u8 = cl[0..];
    let cr_s: []u8 = cr[0..];
    let vl_s: []u8 = vl[0..];
    let vr_s: []u8 = vr[0..];

    const mvc = konigMinVertexCover(g, ml[0..], mr[0..], &cl_s, &cr_s, &vl_s, &vr_s, &q_s) ?? 0;
    // m == 3, mvc == 3
    * = m;
    * = mvc;
}
```
