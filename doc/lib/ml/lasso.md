# `cap:lasso` — 纯 CP 零堆分配稀疏正则化回归库（Lasso / Ridge / ElasticNet）

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

`cap:lasso` 提供 100% 纯 CP、零堆分配的稀疏正则化线性回归求解器，基于 **循环坐标下降（Cyclic Coordinate Descent）** 与 **软阈值算子（Soft-Thresholding）** 统一求解 ElasticNet、纯 Lasso（`l1_ratio = 1.0`）与纯 Ridge（`l1_ratio = 0.0`），并内置线性预测与 $R^2$ / MSE 评估指标。

## 目标函数

$$\min_{w, b} \; \frac{1}{2n} \| y - Xw - b\mathbf{1} \|_2^2 + \alpha \cdot \text{l1\_ratio} \cdot \| w \|_1 + \frac{1}{2} \alpha (1 - \text{l1\_ratio}) \| w \|_2^2$$

## 特性概览

- **零堆分配（Zero-Alloc）**：残差 `residual`、列范数平方 `col_norm_sq`、特征均值 `x_mean` 与输出权重 `w_out` 全部由调用方提供，不构造 $O(p^2)$ 的 $X^T X$ 矩阵。
- **增量残差更新**：每次坐标更新仅需 $O(n)$ 维护残差 $r = y - Xw - b$，对高维稀疏特征极速收敛。
- **精确稀疏压缩**：L1 软阈值算子可将无关噪声特征精确置为 `0.0`；当 $\alpha \ge \alpha_{\max}$ 时全部权重一步精确归零。
- **完备边界防御**：自动拦截负 `alpha`、越界 `l1_ratio`、零样本/零特征、缓冲不足以及 `NaN`/`Inf`。

## API 参考

### 原语、配置与工作区
- `softThreshold(z: f64, gamma: f64): f64`
- `LassoConfig`：`alpha`, `l1_ratio`, `fit_intercept`, `max_iter`, `tol`；提供 `LassoConfig.defaultLasso(alpha)` / `LassoConfig.defaultRidge(alpha)` / `LassoConfig.defaultElasticNet(alpha, l1_ratio)`
- `LassoResult`：`intercept`, `iterations`, `converged`, `mse`, `non_zero_count`
- `LassoWorkspace`：`residual: []f64`（`>= n_samples`）、`col_norm_sq: []f64`（`>= n_features`）、`x_mean: []f64`（`>= n_features`）

### 拟合与评估
- `elasticNetFit(x, y, n_samples, n_features, cfg, w_out: *[]f64, ws: *LassoWorkspace): LassoResult?`
- `lassoFit(x, y, n_samples, n_features, alpha, fit_intercept, w_out: *[]f64, ws: *LassoWorkspace): LassoResult?`
- `ridgeFit(x, y, n_samples, n_features, alpha, fit_intercept, w_out: *[]f64, ws: *LassoWorkspace): LassoResult?`
- `linearPredict(w: []const f64, intercept: f64, sample: []const f64): f64?`
- `mseScore(y_true: []const f64, y_pred: []const f64): f64?`
- `r2Score(y_true: []const f64, y_pred: []const f64): f64?`

## 使用示例

```cp
import {
    LassoWorkspace,
    lassoFit,
    linearPredict,
} from "cap:ml";

const x: [8]f64 = [
    1.0, 0.2,
    2.0, -0.1,
    3.0, 0.3,
    4.0, -0.2,
];
const y: [4]f64 = [3.0, 5.0, 7.0, 9.0]; // y = 2 * x0 + 1

let w_b: [2]f64 = [0.0; 2];
let r_b: [4]f64 = [0.0; 4];
let cn_b: [2]f64 = [0.0; 2];
let xm_b: [2]f64 = [0.0; 2];
let w: []f64 = w_b[0..];

let ws = LassoWorkspace {
    residual: r_b[0..],
    col_norm_sq: cn_b[0..],
    x_mean: xm_b[0..],
};

if (res := lassoFit(x[0..], y[0..], 4, 2, 0.05, true, *w, *ws)) {
    const q: [2]f64 = [5.0, 0.0];
    const pred = linearPredict(w, res.intercept, q[0..]);
    * = pred;
}
```
