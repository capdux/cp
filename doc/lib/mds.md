# `cap:mds` — 零堆分配多维尺度分析（Classical MDS & SMACOF）降维嵌入库

`cap:mds` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的多维尺度分析降维与嵌入引擎，包含经典解析解 **Classical / Torgerson-Gower MDS**（双中心化 Gram 矩阵 + 循环 Jacobi 对称特征分解）与迭代度量优化 **SMACOF**（Guttman 变换单调最小化 Kruskal Stress-1），配套成对欧氏距离矩阵与 Kruskal Stress-1 评估原语。

## 核心特性

- **100% 零堆分配**：Gram 矩阵、特征值/特征向量矩阵、Guttman 矩阵与嵌入坐标全部由调用方以切片工作区传入。
- **Classical MDS（`classicalMds`）**：计算双中心化内积矩阵 $B = -\frac{1}{2} J D^{(2)} J$，通过数值稳定的 Rutishauser 循环 Jacobi 算法求解对称特征分解，对任意欧氏距离矩阵零误差（`stress1 < 1e-12`）恢复等距嵌入。
- **SMACOF 度量 MDS（`smacofRefine`）**：基于 Majorization-Minimization 的 Guttman 变换迭代优化，保证每步 Kruskal Stress-1 单调非增，在非欧氏或含噪距离矩阵上进一步降低嵌入失真。
- **完整防御校验**：严格校验距离矩阵对称性、非负性、零对角线、`NaN`/无穷大及全部工作区缓冲容量。

---

## 导出清单

### 结构体

- `MdsResult`：`{ positive_eigenvalues, variance_explained, stress1, sweeps }`
- `SmacofResult`：`{ stress1, iterations, converged }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `pairwiseEuclidean(points, n, dims, dist_out): bool` | 计算行主序点集 `n x dims` 的对称欧氏距离矩阵 |
| `kruskalStress1(target_dist, coords, n, dims): f64?` | 计算归一化 Kruskal Stress-1 |
| `doubleCenterGram(dist_mat, n, gram_out, row_mean_s): bool` | 计算双中心化内积矩阵 $B = -\frac{1}{2} J D^{(2)} J$ |
| `classicalMds(dist_mat, n, out_dims, coords_out, gram_s, row_mean_s, evals_s, evecs_s, max_sweeps, tol): MdsResult?` | 经典 Torgerson-Gower MDS 解析嵌入 |
| `smacofRefine(dist_mat, n, out_dims, coords_inout, max_iter, tol, b_mat_s, next_coords_s): SmacofResult?` | SMACOF 迭代应力最小化细化 |

---

## 使用示例

```typescript
import std from "std";
import {
    pairwiseEuclidean,
    classicalMds,
    smacofRefine,
} from "cap:mds";

fn mdsDemo() {
    const pts = [0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0];
    let dist_arr: [16]f64 = std.mem.zeroes([16]f64);
    let dist_s: []f64 = dist_arr[0..];
    * = pairwiseEuclidean(pts[0..], 4, 2, &dist_s);

    let coords_arr: [8]f64 = std.mem.zeroes([8]f64);
    let gram_arr: [16]f64 = std.mem.zeroes([16]f64);
    let rmean_arr: [4]f64 = std.mem.zeroes([4]f64);
    let evals_arr: [4]f64 = std.mem.zeroes([4]f64);
    let evecs_arr: [16]f64 = std.mem.zeroes([16]f64);

    let coords_s: []f64 = coords_arr[0..];
    let gram_s: []f64 = gram_arr[0..];
    let rmean_s: []f64 = rmean_arr[0..];
    let evals_s: []f64 = evals_arr[0..];
    let evecs_s: []f64 = evecs_arr[0..];

    const res = classicalMds(dist_arr[0..], 4, 2, &coords_s, &gram_s, &rmean_s, &evals_s, &evecs_s, 50, 1.0e-12);
    * = res;

    let b_arr: [16]f64 = std.mem.zeroes([16]f64);
    let next_arr: [8]f64 = std.mem.zeroes([8]f64);
    let b_s: []f64 = b_arr[0..];
    let next_s: []f64 = next_arr[0..];
    const ref = smacofRefine(dist_arr[0..], 4, 2, &coords_s, 100, 1.0e-9, &b_s, &next_s);
    * = ref;
}
```
