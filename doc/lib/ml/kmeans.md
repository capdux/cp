# `cap:kmeans` — 纯 CP 零堆分配 K-Means++ 聚类库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

`cap:kmeans` 提供纯 CP、零堆分配的任意 `D` 维欧氏空间 K-Means++ 聚类实现，涵盖确定性 SplitMix64 $D^2$ 加权质心初始化、Lloyd 迭代与空簇自动恢复、类内平方误差和（SSE / Inertia）计算、单样本预测及全局平均 Silhouette 轮廓系数评估。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部输出与工作区缓冲区均由调用方提供栈/切片内存；质心更新采用 64 维栈分块扫描，支持任意维度 `D`（含 `D > 64`）。
- **确定性 K-Means++ 初始化**：基于 SplitMix64 PRNG 与 $D^2$ 距离加权概率抽样，相同 `seed` 在任意平台产生完全一致的初始化与聚类结果。
- **空簇防退化恢复**：Lloyd 迭代中若出现空簇，自动从非单点簇中选取当前距离质心最远的样本点重新初始化该簇。
- **严格边界防御**：自动拦截 `k == 0`、`k > n`、`dim == 0`、`n == 0`、负容差、缓冲区不足、越界标签及 NaN/Inf 非有限坐标。

## 数据布局

样本矩阵 `points: []const f64` 采用行主序扁平布局：第 `i` 个样本（`0 <= i < n`）的第 `d` 维坐标（`0 <= d < dim`）位于 `points[i * dim + d]`，总长度至少为 `n * dim`。质心矩阵 `centroids` 同理，总长度至少为 `k * dim`。

## API 参考

### `KmeansResult`

```cp
pub struct KmeansResult {
    iterations: u32,
    converged: bool,
    inertia: f64,
}
```

- `iterations`：实际执行的 Lloyd 迭代轮数。
- `converged`：是否在 `max_iter` 步内收敛（标签无变化或最大质心欧氏漂移 `<= tol`）。
- `inertia`：最终聚类的总类内平方误差和（SSE）。

### `sqDist(a: []const f64, b: []const f64): f64`

计算两个向量的欧氏距离平方（按 `min(a.len, b.len)` 遍历）。

### `kmeansPredictOne(point: []const f64, centroids: []const f64, k: usize, dim: usize): usize?`

计算单个 `dim` 维样本 `point` 最近的质心索引（`0..k-1`）。参数非法、缓冲不足或包含非有限值时返回 `null`。

### `kmeansInitPlusPlus(points: []const f64, n: usize, dim: usize, k: usize, seed: u64, centroids_out: *[]f64, min_dists_scratch: *[]f64): bool`

执行确定性 SplitMix64 + $D^2$ 加权 K-Means++ 质心初始化：
- `centroids_out`：长度 `>= k * dim`；
- `min_dists_scratch`：长度 `>= n`。
成功返回 `true`，非法输入或缓冲不足返回 `false`。

### `kmeansFit(points: []const f64, n: usize, dim: usize, k: usize, max_iter: u32, tol: f64, seed: u64, centroids_out: *[]f64, labels_out: *[]usize, counts_scratch: *[]usize, dists_scratch: *[]f64): KmeansResult?`

执行完整的 K-Means++ 初始化 + Lloyd 迭代聚类：
- `max_iter`：最大迭代轮数；
- `tol`：收敛容差（最大质心欧氏漂移 `<= tol` 即停止，`tol >= 0.0`）；
- `centroids_out`：长度 `>= k * dim`；
- `labels_out`：长度 `>= n`；
- `counts_scratch`：长度 `>= k`；
- `dists_scratch`：长度 `>= n`。
非法参数或缓冲不足时返回 `null`。

### `kmeansInertia(points: []const f64, n: usize, dim: usize, centroids: []const f64, k: usize, labels: []const usize): f64?`

计算给定质心与标签分配下的总类内平方误差和（SSE）。

### `silhouetteScore(points: []const f64, n: usize, dim: usize, labels: []const usize, k: usize): f64?`

计算全局平均轮廓系数（范围 `[-1.0, 1.0]`），衡量类内紧致度与类间分离度。要求 `n >= 2`、`2 <= k <= n` 且至少包含 2 个非空簇，否则返回 `null`。

## 使用示例

```cp
import std from "std";
import { kmeansFit, kmeansPredictOne, silhouetteScore } from "cap:ml";

const points: [12]f64 = [
    0.0, 0.0,  0.2, 0.1,  -0.1, 0.2,
    10.0, 10.0,  10.2, 9.9,  9.8, 10.1,
];

let c_buf: [4]f64 = std.mem.zeroes([4]f64);
let l_buf: [6]usize = std.mem.zeroes([6]usize);
let cnt_buf: [2]usize = std.mem.zeroes([2]usize);
let d_buf: [6]f64 = std.mem.zeroes([6]f64);

let centroids: []f64 = c_buf[0..];
let labels: []usize = l_buf[0..];
let counts: []usize = cnt_buf[0..];
let dists: []f64 = d_buf[0..];

if (res := kmeansFit(points[0..], 6, 2, 2, 50, 1.0e-6, 42, *centroids, *labels, *counts, *dists)) {
    const score = silhouetteScore(points[0..], 6, 2, labels, 2);
    const query: [2]f64 = [9.9, 10.2];
    const pred = kmeansPredictOne(query[0..], centroids, 2, 2);
    * = res;
    * = score;
    * = pred;
}
```
