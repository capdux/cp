# `cap:ekf` — 零堆分配非线性状态估计库（EKF & UKF）

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:ekf` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的非线性状态估计工具集，涵盖中心差分数值雅可比计算、Joseph 稳定化协方差更新的扩展卡尔曼滤波（EKF）以及基于 Merwe 缩放 Sigma 点与 Cholesky 分解的无迹卡尔曼滤波（UKF）。

## 核心特性

- **100% 零堆分配**：状态向量、协方差矩阵、Sigma 点池与所有中间矩阵均由调用方以切片工作区传入。
- **有限差分雅可比（`numericalJacobianF` / `numericalJacobianH`）**：二阶中心差分精确近似状态转移与观测雅可比。
- **Joseph 形式 EKF（`ekfPredict` / `ekfUpdate`）**：采用 $P = (I - KH) P (I - KH)^T + K R K^T$ 稳定化更新与强制对称化，极端低噪声下仍保持对称正定；通过 Cholesky 分解求解 $S^{-1}$ 并返回新息马氏距离平方 NIS（$y^T S^{-1} y$）。
- **Merwe 缩放 UKF（`generateSigmaPoints` / `ukfPredict`）**：生成 $2n + 1$ 个 Sigma 点与均值/协方差权重，无需计算雅可比即可高精度传播非线性统计矩。

---

## 导出清单

### 类型与结构

- `TransitionFn = fn(x: []const f64, u: []const f64, dt: f64, x_next_out: *[]f64)`
- `MeasurementFn = fn(x: []const f64, z_out: *[]f64)`
- `EkfWorkspace`：`{ z_pred, innov, ph_t, s_cov, s_chol, k_gain, i_kh, tmp_nn, tmp_nm, tmp_m }`
- `UkfParams`：`{ alpha, beta, kappa }`，附 `UkfParams.default()`（`0.001, 2.0, 0.0`）

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `numericalJacobianF(f, x, u, dt, eps, x_pert_s, f_plus_s, f_minus_s, j_out): bool` | 计算 `n x n` 状态转移雅可比 $F$ |
| `numericalJacobianH(h, x, m, eps, x_pert_s, h_plus_s, h_minus_s, j_out): bool` | 计算 `m x n` 观测雅可比 $H$ |
| `ekfPredict(x_inout, p_inout, n, f_fn, u, dt, f_jac, q_cov, tmp_nn, x_next_s): bool` | EKF 状态与协方差预测 |
| `ekfUpdate(x_inout, p_inout, n, m, h_fn, z, h_jac, r_cov, ws): f64?` | Joseph 形式 EKF 更新，返回 NIS（奇异 `S` 返回 `null`） |
| `generateSigmaPoints(x, p, n, params, sigma_out, wm_out, wc_out, chol_scratch): bool` | 生成 $2n + 1$ 个 Merwe Sigma 点与权重 |
| `ukfPredict(x_inout, p_inout, n, f_fn, u, dt, q_cov, params, sigma_in, sigma_out, wm_s, wc_s, chol_s): bool` | UKF 非线性均值与协方差预测 |

---

## 使用示例

```typescript
import std from "std";
import {
    UkfParams,
    generateSigmaPoints,
    ukfPredict,
} from "cap:math";

fn cvModel(x: []const f64, u: []const f64, dt: f64, out: *[]f64) {
    * = u;
    out.*[0] = x[0] + x[1] * dt;
    out.*[1] = x[1];
}

fn ukfDemo() {
    let x: [2]f64 = [0.0, 2.0];
    let p: [4]f64 = [1.0, 0.0, 0.0, 0.5];
    const q: [4]f64 = [0.01, 0.0, 0.0, 0.01];
    const u: [0]f64 = [];

    let sig_in: [10]f64 = std.mem.zeroes([10]f64);
    let sig_out: [10]f64 = std.mem.zeroes([10]f64);
    let wm: [5]f64 = std.mem.zeroes([5]f64);
    let wc: [5]f64 = std.mem.zeroes([5]f64);
    let chol: [4]f64 = std.mem.zeroes([4]f64);

    let x_s: []f64 = x[0..];
    let p_s: []f64 = p[0..];
    let si_s: []f64 = sig_in[0..];
    let so_s: []f64 = sig_out[0..];
    let wm_s: []f64 = wm[0..];
    let wc_s: []f64 = wc[0..];
    let ch_s: []f64 = chol[0..];

    * = ukfPredict(&x_s, &p_s, 2, cvModel, u[0..], 0.5, q[0..], UkfParams.default(), &si_s, &so_s, &wm_s, &wc_s, &ch_s);
}
```
