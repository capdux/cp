# `cap:mst` — 零堆分配最小生成树与生成森林算法库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:graph`，导入请使用 `import { ... } from "cap:graph"`。

`cap:mst` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的最小生成树（MST）、最大生成树（MaxST）与最小生成森林（MSF）算法套件，涵盖迭代式并查集、索引堆排序 Kruskal 以及 $O(V^2)$ 稠密邻接矩阵 Prim。

## 核心特性

- **100% 零堆分配**：所有并查集、排序索引、松弛数组与输出缓冲均由调用方以切片传入，不修改调用方原始边表。
- **迭代并查集（`DisjointSet`）**：两趟迭代路径压缩 + 按秩合并，无递归栈开销。
- **Kruskal 最小/最大生成树（`kruskalMst` / `maxSpanningTree`）**：基于零分配索引堆排序（确定性决胜），天然支持稀疏图、负权边、零权边、重边、自环与非连通生成森林。
- **稠密 Prim（`primDenseMst`）**：$O(V^2)$ 邻接矩阵求解，支持 `MST_INF_WEIGHT` 断边与多连通分量最小生成森林。
- **生成树合法性验证（`isValidSpanningTree`）**：校验给定边集是否无环且连通并精确累加总权值。

---

## 导出清单

### 常量

| 名称 | 类型 | 说明 |
| :--- | :--- | :--- |
| `MST_INF_WEIGHT` | `f64` | 稠密邻接矩阵断边哨兵（`1.0e300`） |

### 数据结构

- `WeightedEdge`：`{ u: usize, v: usize, weight: f64, id: u32 }`，附 `WeightedEdge.init(u, v, weight, id)`
- `PrimEdge`：`{ u: usize, v: usize, weight: f64 }`
- `MstResult`：`{ totalWeight: f64, edgeCount: usize, componentCount: usize, isConnected: bool }`
- `SpanningTreeCheck`：`{ valid: bool, totalWeight: f64 }`
- `DisjointSet`：`init`, `find`, `unite`, `connected`, `components`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `kruskalMst(num_nodes: usize, edges: []const WeightedEdge, parent_scratch: *[]usize, rank_scratch: *[]u8, order_scratch: *[]usize, selectedEdgesOut: *[]usize): MstResult?` | Kruskal 最小生成树/森林 |
| `maxSpanningTree(num_nodes: usize, edges: []const WeightedEdge, parent_scratch: *[]usize, rank_scratch: *[]u8, order_scratch: *[]usize, selectedEdgesOut: *[]usize): MstResult?` | Kruskal 最大生成树/森林 |
| `primDenseMst(num_nodes: usize, adj_matrix: []const f64, min_weight_scratch: *[]f64, parent_scratch: *[]i32, in_mst_scratch: *[]u8, selectedEdgesOut: *[]PrimEdge): MstResult?` | 稠密邻接矩阵 $O(V^2)$ Prim 最小生成树/森林 |
| `isValidSpanningTree(num_nodes: usize, edges: []const WeightedEdge, selected_indices: []const usize, parent_scratch: *[]usize, rank_scratch: *[]u8): SpanningTreeCheck` | 校验边集是否为无环连通生成树并返回总权值 |

---

## 使用示例

```typescript
import std from "std";
import {
    WeightedEdge,
    kruskalMst,
    isValidSpanningTree,
} from "cap:graph";

fn mstExample() {
    const edges = [
        WeightedEdge.init(0, 1, 4.0, 0),
        WeightedEdge.init(0, 2, 1.0, 1),
        WeightedEdge.init(2, 1, 2.0, 2),
        WeightedEdge.init(1, 3, 5.0, 3),
        WeightedEdge.init(2, 3, 8.0, 4),
    ];

    let parent: [4]usize = std.mem.zeroes([4]usize);
    let rank: [4]u8 = std.mem.zeroes([4]u8);
    let order: [5]usize = std.mem.zeroes([5]usize);
    let out: [3]usize = std.mem.zeroes([3]usize);

    let p_s: []usize = parent[0..];
    let r_s: []u8 = rank[0..];
    let o_s: []usize = order[0..];
    let out_s: []usize = out[0..];

    const res = kruskalMst(4, edges[0..], &p_s, &r_s, &o_s, &out_s) ?? return;
    // res.totalWeight == 8.0, res.edgeCount == 3, res.isConnected == true
    const chk = isValidSpanningTree(4, edges[0..], out[0..res.edgeCount], &p_s, &r_s);
    // chk.valid == true, chk.totalWeight == 8.0
    * = chk;
}
```
