# `cap:spectral` — 纯 CP 零分配谱图聚类与图拉普拉斯分析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

100% 纯 CP 零堆分配（Zero-Alloc）图拉普拉斯构造与谱图聚类标准库：
- 支持未归一化拉普拉斯 $L = D - W$ 与对称归一化拉普拉斯 $L_{\text{sym}} = I - D^{-1/2} W D^{-1/2}$（安全处理孤立点 $d_i = 0$）；
- 内置零分配循环 Jacobi 对称特征值求解器；
- 提供代数连通度 $\lambda_2$ / Fiedler 向量二分、Ng-Jordan-Weiss (NJW) $k$-路行归一化谱嵌入、Normalized Cut 图割度量与高斯 RBF 相似度矩阵构造。

## 导入方式

```cp
import {
    LaplacianKind,
    FiedlerResult,
    buildLaplacian,
    fiedlerBisection,
    spectralEmbedding,
    normalizedCut,
    rbfAffinityMatrix,
} from "cap:ml";
```

## API 契约

- `pub enum LaplacianKind { Unnormalized, SymmetricNormalized }`
- `pub struct FiedlerResult { algebraic_connectivity: f64, connected_components: usize, sweeps: usize }`
- `pub fn buildLaplacian(adj: []const f64, n: usize, kind: LaplacianKind, lap_out: *[]f64, deg_out: *[]f64): bool`
  校验对称非负有限邻接矩阵 `adj`（`n x n`），输出拉普拉斯矩阵与度向量。
- `pub fn fiedlerBisection(adj: []const f64, n: usize, kind: LaplacianKind, fiedler_vec_out: *[]f64, partition_out: *[]u8, lap_s: *[]f64, deg_s: *[]f64, evals_s: *[]f64, evecs_s: *[]f64, max_sweeps: usize, tol: f64): FiedlerResult?`
  计算第二小特征值 $\lambda_2$、零特征值重数（连通分量数）、Fiedler 特征向量及 `0`/`1` 二分社区标签。
- `pub fn spectralEmbedding(adj: []const f64, n: usize, k: usize, embedding_out: *[]f64, lap_s: *[]f64, deg_s: *[]f64, evals_s: *[]f64, evecs_s: *[]f64, max_sweeps: usize, tol: f64): bool`
  提取 $L_{\text{sym}}$ 最小 `k` 个特征向量并逐行 L2 归一化写入 `embedding_out`（`n x k`，行主序）。
- `pub fn normalizedCut(adj: []const f64, n: usize, partition: []const u8): f64?`
  计算 $\text{NCut}(A, B) = \text{cut}(A, B)/\text{vol}(A) + \text{cut}(A, B)/\text{vol}(B)$。
- `pub fn rbfAffinityMatrix(points: []const f64, n_samples: usize, n_features: usize, sigma: f64, adj_out: *[]f64): bool`
  构造对称高斯 RBF 相似度矩阵 $W_{ij} = \exp(-\|x_i - x_j\|^2 / (2\sigma^2))$（对角线置 `0`）。

## 示例

```cp
import { LaplacianKind, fiedlerBisection, normalizedCut } from "cap:ml";

fn demo(adj: []const f64) {
    let fvec_b: [8]f64 = [0.0; 8];
    let part_b: [8]u8 = [0; 8];
    let lap_b: [64]f64 = [0.0; 64];
    let deg_b: [8]f64 = [0.0; 8];
    let evals_b: [8]f64 = [0.0; 8];
    let evecs_b: [64]f64 = [0.0; 64];
    let fvec: []f64 = fvec_b[0..];
    let part: []u8 = part_b[0..];
    let lap: []f64 = lap_b[0..];
    let deg: []f64 = deg_b[0..];
    let evals: []f64 = evals_b[0..];
    let evecs: []f64 = evecs_b[0..];

    if (res := fiedlerBisection(adj, 8, LaplacianKind.SymmetricNormalized, *fvec, *part, *lap, *deg, *evals, *evecs, 100, 1.0e-12)) {
        * = res;
        * = normalizedCut(adj, 8, part);
    }
}
```
