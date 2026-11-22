# `cap:louvain` — 零堆分配 Louvain 模块度社区发现库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

`cap:louvain` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的图社区发现引擎，基于带权无向 CSR 图实现精确 Newman-Girvan 模块度计算（`computeModularity`）、Louvain 局部移动与规范重标号（`louvainDetect`，支持可调分辨率参数 `resolution`）以及零分配标签传播算法（`labelPropagation`）。

## 核心特性

- **100% 零堆分配**：节点度、社区总权、邻居社区稀疏累加表及重标号映射全部通过调用方提供的 `LouvainWorkspace` 切片完成，每轮局部移动时间复杂度严格 $O(m)$。
- **精确 Newman-Girvan 模块度**：支持分辨率参数 $\gamma$，计算 $Q = \sum_c \left[\frac{\Sigma_{\text{in}}(c)}{2m} - \gamma \left(\frac{\Sigma_{\text{tot}}(c)}{2m}\right)^2\right]$。
- **Louvain 局部移动 + 规范重标号**：按精确模块度增量 $\Delta Q$ 贪心迁移节点（高分辨率下自动支持拆入空社区），收敛后将社区 ID 规范重标号为连续 `0..num_communities-1`。
- **零分配标签传播（LPA）**：提供 `labelPropagation` 异步加权标签传播，适合大规模图快速社区初始化与基准对比。
- **完整防御校验**：校验 CSR 完整性、列索引范围、非负有限边权、合法分辨率及工作区缓冲长度。

---

## 导出清单

### 结构体

- `WeightedGraph`：`{ num_nodes, row_ptr, col_idx, weights }`，附 `validate(): bool` 与 `totalWeight(): f64`
- `LouvainConfig`：`{ resolution, min_gain, max_passes }`，附 `LouvainConfig.default()`
- `LouvainResult`：`{ num_communities, modularity, passes, moves }`
- `LouvainWorkspace`：`{ node_deg, comm_tot, comm_weights, touched_comms, relabel_map }`（全部长度 `>= num_nodes`）

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `computeModularity(g, comm, resolution, comm_in_s, comm_tot_s): f64?` | 计算带分辨率参数的精确 Newman-Girvan 模块度 |
| `louvainDetect(g, cfg, comm_out, ws): LouvainResult?` | Louvain 局部移动社区发现与规范重标号 |
| `labelPropagation(g, max_iter, labels_out, ws_weights, ws_touched): usize?` | 零分配异步加权标签传播社区发现 |

---

## 使用示例

```typescript
import std from "std";
import {
    WeightedGraph,
    LouvainConfig,
    LouvainWorkspace,
    louvainDetect,
} from "cap:ml";

fn louvainDemo() {
    // Two triangles {0,1,2} and {3,4,5} linked by weak edge (2,3) of weight 0.1
    const row_ptr: [7]usize = [0, 2, 4, 7, 10, 12, 14];
    const col_idx: [14]usize = [1, 2, 0, 2, 0, 1, 3, 2, 4, 5, 3, 5, 3, 4];
    const weights: [14]f64 = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.1, 0.1, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0];
    const g = WeightedGraph {
        num_nodes: 6,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
        weights: weights[0..],
    };

    let comm_arr: [6]usize = std.mem.zeroes([6]usize);
    let node_deg: [6]f64 = std.mem.zeroes([6]f64);
    let comm_tot: [6]f64 = std.mem.zeroes([6]f64);
    let comm_weights: [6]f64 = std.mem.zeroes([6]f64);
    let touched: [6]usize = std.mem.zeroes([6]usize);
    let relabel: [6]i32 = std.mem.zeroes([6]i32);
    let ws = LouvainWorkspace {
        node_deg: node_deg[0..],
        comm_tot: comm_tot[0..],
        comm_weights: comm_weights[0..],
        touched_comms: touched[0..],
        relabel_map: relabel[0..],
    };
    let comm_out: []usize = comm_arr[0..];
    const res = louvainDetect(g, LouvainConfig.default(), &comm_out, &ws);
    * = res;
}
```
