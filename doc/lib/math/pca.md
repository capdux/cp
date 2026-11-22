# `cap:pca` — 零分配主成分分析（PCA）降维与对称矩阵 Jacobi 特征值分解库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

零堆分配（Zero-Alloc）的纯 CP 数值降维与对称特征值分解标准库，提供经典循环 Jacobi 实对称矩阵特征值/特征向量求解器 `jacobiEigenSymmetric`，以及完整的无偏样本协方差 PCA 拟合 `pcaFit`、降维投影 `pcaTransform`、逆变换重构 `pcaInverseTransform` 与均方重构误差 `pcaReconstructionMse`。

## 核心特性

- **零堆分配（Zero-Alloc）**：全部协方差矩阵、特征值/特征向量缓冲与投影输出切片由调用方在栈或静态内存中提供。
- **高精度循环 Jacobi 特征求解器**：采用 Rutishauser 数值稳定旋转公式求解实对称矩阵全部特征值（降序排列）与单位正交列特征向量（满足 $A V = V \Lambda$ 与 $V^T V = I$）。
- **无偏样本协方差 PCA**：自动中心化并计算 $\Sigma = \frac{1}{N-1}(X - \mu)^T(X - \mu)$，钳位微小负浮点噪声，安全处理常数零方差特征（避免除零 `NaN`）。
- **完备边界防护**：全面拦截 `n_samples < 2`、`n_components > n_features`、非法 `tol`、缓冲区不足与 `NaN`/`Inf` 输入。

## 导出 API

### 数据模型

- `PcaFitResult { converged: bool, totalVariance: f64, explainedVarianceSum: f64, explainedRatioSum: f64, sweeps: usize }`

### 核心函数

- `jacobiEigenSymmetric(sym_mat: []f64, n: usize, evals_out: []f64, evecs_out: []f64, max_sweeps: usize, tol: f64): usize?`
  - 原地对角化 `n x n` 实对称矩阵，返回迭代扫描轮数；特征值按降序存入 `evals_out`，对应单位特征向量按列存入 `evecs_out`。
- `pcaFit(data: []const f64, n_samples: usize, n_features: usize, n_components: usize, mean_out: []f64, components_out: []f64, explained_var_out: []f64, explained_ratio_out: []f64, cov_scratch: []f64, evals_scratch: []f64, evecs_scratch: []f64, max_sweeps: usize, tol: f64): PcaFitResult?`
  - 计算列均值、协方差矩阵与前 `n_components` 个主成分（按行存入 `components_out`）。
- `pcaTransform(data: []const f64, n_samples: usize, n_features: usize, mean: []const f64, components: []const f64, n_components: usize, projected_out: []f64): bool`
  - 计算 $Z = (X - \mu) W^T$。
- `pcaInverseTransform(projected: []const f64, n_samples: usize, n_components: usize, mean: []const f64, components: []const f64, n_features: usize, reconstructed_out: []f64): bool`
  - 计算 $\hat{X} = Z W + \mu$。
- `pcaReconstructionMse(original: []const f64, reconstructed: []const f64, len: usize): f64`
  - 计算逐元素均方误差 $\frac{1}{\text{len}}\sum (x_i - \hat{x}_i)^2$。

## 使用示例

```cp
import std from "std";
import {
    pcaFit,
    pcaTransform,
    pcaInverseTransform,
    pcaReconstructionMse,
} from "cap:math";

fn example() {
    // 4 samples x 2 features
    const data: [8]f64 = [
        1.0, 2.0,
        3.0, 4.0,
        5.0, 6.0,
        7.0, 8.0,
    ];
    let mean: [2]f64 = std.mem.zeroes([2]f64);
    let comps: [2]f64 = std.mem.zeroes([2]f64); // 1 component x 2 features
    let evar: [1]f64 = std.mem.zeroes([1]f64);
    let eratio: [1]f64 = std.mem.zeroes([1]f64);
    let cov: [4]f64 = std.mem.zeroes([4]f64);
    let evals: [2]f64 = std.mem.zeroes([2]f64);
    let evecs: [4]f64 = std.mem.zeroes([4]f64);

    if (res := pcaFit(data[0..], 4, 2, 1, mean[0..], comps[0..], evar[0..], eratio[0..], cov[0..], evals[0..], evecs[0..], 50, 1.0e-12)) {
        * = res.explainedRatioSum; // ~1.0
        let proj: [4]f64 = std.mem.zeroes([4]f64);
        let recon: [8]f64 = std.mem.zeroes([8]f64);
        * = pcaTransform(data[0..], 4, 2, mean[0..], comps[0..], 1, proj[0..]);
        * = pcaInverseTransform(proj[0..], 4, 1, mean[0..], comps[0..], 2, recon[0..]);
        * = pcaReconstructionMse(data[0..], recon[0..], 8);
    }
}
```
