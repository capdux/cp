# `cap:scc` — 纯 CP 零堆分配 Tarjan SCC、拓扑排序、DAG 最长路径与 2-SAT 库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:graph`，导入请使用 `import { ... } from "cap:graph"`。

`cap:scc` 提供基于 CSR（Compressed Sparse Row）有向图表示的纯 CP 零堆分配图论核心算法集，包含纯迭代式防栈溢出 Tarjan 强连通分量、Kahn 拓扑排序与环检测、DAG 最长路径以及基于 SCC 的线性时间 2-SAT 布尔可满足性求解器。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部图结构与算法工作区均由调用方提供栈/切片内存。
- **纯迭代 Tarjan SCC（$O(V + E)$）**：显式调用栈模拟 DFS，彻底避免深度图导致的系统调用栈溢出；产出 SCC 编号严格满足**逆拓扑序**（跨 SCC 边 `u -> v` 必有 `comp[u] > comp[v]`）。
- **Kahn 拓扑排序与 DAG 最长路径（$O(V + E)$）**：一次 BFS 同时完成拓扑排序与有向环检测，并支持基于拓扑序的 DAG 最长路径 DP。
- **线性时间 2-SAT 求解器（$O(V + E)$）**：零堆两遍扫描构建 `2 * num_vars` 节点蕴含图 CSR 并调用迭代 Tarjan SCC，自动检测矛盾并按逆拓扑序输出可满足真值赋值。

## API 参考

### `CsrGraph`

```cp
pub struct CsrGraph {
    num_nodes: usize,
    row_ptr: []const usize,
    col_idx: []const usize,
    fn validate(): bool;
    fn neighbors(u: usize): []const usize;
    fn edgeCount(): usize;
}
```

- `validate()`：校验 `row_ptr.len >= num_nodes + 1`、`row_ptr[0] == 0`、行偏移单调递增且不越界、所有目标顶点 `< num_nodes`。
- `neighbors(u)`：返回顶点 `u` 的出边邻接切片（越界安全返回空切片）。
- `edgeCount()`：返回图中的有向边总数。

### `TarjanWorkspace`

```cp
pub struct TarjanWorkspace {
    disc: []i32,
    low: []i32,
    on_stack: []u8,
    tarjan_stack: []usize,
    call_node: []usize,
    call_edge: []usize,
}
```

每个切片长度均须 `>= num_nodes`（用于 `solve2Sat` 时须 `>= 2 * num_vars`）。

### `tarjanScc(g: CsrGraph, comp_out: *[]usize, ws: *TarjanWorkspace): usize?`

计算有向图 `g` 的全部强连通分量，将每个顶点的 SCC 编号（`0..num_sccs-1`，按逆拓扑序）写入 `comp_out[0..n]` 并返回总分量数 `num_sccs`。图非法或缓冲不足返回 `null`。

### `topoSort(g: CsrGraph, order_out: *[]usize, indeg_scratch: *[]usize, queue_scratch: *[]usize): bool`

对 DAG 执行 Kahn 拓扑排序，结果写入 `order_out[0..n]` 并返回 `true`；若存在有向环、图非法或缓冲不足返回 `false`。

### `isDag(g: CsrGraph, indeg_scratch: *[]usize, queue_scratch: *[]usize, order_scratch: *[]usize): bool`

判断 `g` 是否为合法有向无环图（DAG）。

### `dagLongestPath(g: CsrGraph, dist_out: *[]usize, order_scratch: *[]usize, indeg_scratch: *[]usize, queue_scratch: *[]usize): usize?`

计算 DAG 中到达各节点的最长路径边数（写入 `dist_out`）并返回全局最长路径边数；若图含环或缓冲不足返回 `null`。

### `TwoSatClause` 与 `solve2Sat`

```cp
pub struct TwoSatClause {
    a: i32,
    b: i32,
}

pub fn solve2Sat(
    num_vars: usize,
    clauses: []const TwoSatClause,
    assignment_out: *[]u8,
    row_ptr_s: *[]usize,
    col_idx_s: *[]usize,
    comp_s: *[]usize,
    ws: *TarjanWorkspace,
): bool
```

- 字面量约定：正整数 `1..num_vars` 表示 $x_k$，负整数 `-1..-num_vars` 表示 $\neg x_k$。
- 缓冲要求：`assignment_out.len >= num_vars`，`row_ptr_s.len >= 2 * num_vars + 1`，`col_idx_s.len >= 2 * clauses.len`，`comp_s.len >= 2 * num_vars`，`ws` 全部 6 个切片长度 `>= 2 * num_vars`。
- 有解返回 `true` 并将各变量赋值（`0` 或 `1`）写入 `assignment_out[0..num_vars]`；无解或参数非法返回 `false`。

## 使用示例

```cp
import std from "std";
import { CsrGraph, TarjanWorkspace, tarjanScc } from "cap:graph";

const row_ptr: [4]usize = [0, 1, 2, 3];
const col_idx: [3]usize = [1, 2, 0]; // 3-cycle: 0 -> 1 -> 2 -> 0
const g = CsrGraph { num_nodes: 3, row_ptr: row_ptr[0..], col_idx: col_idx[0..] };

let disc: [3]i32 = std.mem.zeroes([3]i32);
let low: [3]i32 = std.mem.zeroes([3]i32);
let on_st: [3]u8 = std.mem.zeroes([3]u8);
let tst: [3]usize = std.mem.zeroes([3]usize);
let cn: [3]usize = std.mem.zeroes([3]usize);
let ce: [3]usize = std.mem.zeroes([3]usize);
let ws = TarjanWorkspace {
    disc: disc[0..], low: low[0..], on_stack: on_st[0..],
    tarjan_stack: tst[0..], call_node: cn[0..], call_edge: ce[0..],
};
let comp_buf: [3]usize = std.mem.zeroes([3]usize);
let comp: []usize = comp_buf[0..];

const n_scc = tarjanScc(g, *comp, *ws); // Some(1)
* = n_scc;
```
