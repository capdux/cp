# `cap:dtw` — 零堆分配动态时间规整（DTW）与时间序列对齐库

`cap:dtw` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的 L2 动态时间规整（DTW）算法套件，包含 $O(m)$ 滚动双行距离计算、完整规整路径回溯、多维轨迹对齐、Sakoe-Chiba 带宽约束、LB_Keogh 严格下界剪枝与 Z-归一化预处理。

## 核心特性

- **100% 零堆分配**：距离计算仅需调用方提供 $O(m)$ 双行工作区；路径回溯使用调用方提供的 DP 矩阵与路径切片。
- **L2 欧氏一致度量**：$\text{DTW}(a, b) = \sqrt{\min_P \sum_{(i,j)\in P} \|a_i - b_j\|_2^2}$，严格满足 $\text{LB\_Keogh}(q, \text{env}(c, w)) \le \text{DTW}(q, c, w) \le \text{Euclidean}(q, c)$ 与对称性。
- **Sakoe-Chiba 带宽约束**：`window == 0` 表示无带宽限制；当 `0 < window < |n - m|` 时自动扩展有效带宽至 `|n - m|`，保证起止点可达。
- **LB_Keogh 快速下界**：$O(n)$ 计算候选序列上下包络与查询下界，用于大规模时间序列检索前置剪枝。

---

## 导出清单

| 符号 | 签名 / 说明 |
| :--- | :--- |
| `DTW_INF` | `f64 = 1.0e300`，带宽外不可达格点哨兵 |
| `WarpPair` | `{ i: usize, j: usize }`，规整路径索引对 |
| `DtwPathResult` | `{ distance: f64, path_len: usize }` |
| `dtwDistance1D(a, b, window, prev_row_s, curr_row_s): f64?` | 1D 滚动双行 $O(m)$ DTW 距离（工作区长度 $\ge b.\text{len} + 1$） |
| `dtwAlign1D(a, b, window, dp_mat_s, path_out): DtwPathResult?` | 1D 完整 DP 回溯，正序输出 `(0,0) -> (n-1,m-1)` 最优路径 |
| `dtwDistanceMulti(a, n, b, m, dims, window, prev_row_s, curr_row_s): f64?` | 多维时间序列 DTW（欧氏帧距离） |
| `computeKeoghEnvelope(seq, window, upper_out, lower_out): bool` | 计算 Sakoe-Chiba 上下包络 |
| `lbKeogh(query, upper, lower): f64?` | 计算 LB_Keogh 严格下界 |
| `zNormalize(seq, out): bool` | 单序列零均值、单位标准差 Z-归一化（常数序列输出全零） |

---

## 使用示例

```typescript
import std from "std";
import {
    WarpPair,
    dtwDistance1D,
    dtwAlign1D,
    computeKeoghEnvelope,
    lbKeogh,
} from "cap:dtw";

fn dtwDemo() {
    const a = [0.0, 1.0, 2.0, 1.0, 0.0];
    const b = [0.0, 0.0, 1.0, 2.0, 2.0, 1.0, 0.0];
    let prev: [8]f64 = std.mem.zeroes([8]f64);
    let curr: [8]f64 = std.mem.zeroes([8]f64);
    let p_s: []f64 = prev[0..];
    let c_s: []f64 = curr[0..];

    const dist = dtwDistance1D(a[0..], b[0..], 0, &p_s, &c_s) ?? -1.0;

    let dp: [48]f64 = std.mem.zeroes([48]f64);
    let path: [12]WarpPair = std.mem.zeroes([12]WarpPair);
    let dp_s: []f64 = dp[0..];
    let path_s: []WarpPair = path[0..];
    const aligned = dtwAlign1D(a[0..], b[0..], 0, &dp_s, &path_s);
    * = dist;
    * = aligned;
}
```
