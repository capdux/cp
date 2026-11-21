# `cap:nmf` — 纯 CP 零分配非负矩阵分解（NMF）库

100% 纯 CP 零堆分配（Zero-Alloc）的非负矩阵分解（Non-negative Matrix Factorization, $V \approx W H$）标准库，基于 Lee & Seung (2001) 乘法更新规则，支持 Frobenius 范数与广义 KL 散度（I-Divergence）两种目标函数，内置无全局状态 SplitMix64 确定性初始化，保证目标函数单调非增与因子严格非负（$W \ge 0, H \ge 0$）。

## 导入方式

```cp
import {
    NmfLoss,
    NmfConfig,
    NmfResult,
    NmfWorkspace,
    nmfFit,
    nmfTransform,
    nmfReconstruct,
    nmfFrobeniusError,
    nmfKlDivergence,
} from "cap:nmf";
```

## API 契约

### 1. 类型与工作区

- `pub enum NmfLoss { Frobenius, KlDivergence }`
- `pub struct NmfConfig`
  - 字段：`loss: NmfLoss`, `max_iter: usize`, `tol: f64`, `eps: f64`, `seed: u64`
  - 默认工厂：`NmfConfig.default()`（`Frobenius`, `max_iter: 1000`, `tol: 1e-7`, `eps: 1e-12`, `seed: 0x9E3779B97F4A7C15`）
- `pub struct NmfResult`
  - 字段：`converged: bool`, `iterations: usize`, `loss: f64`, `frobenius_error: f64`
- `pub struct NmfWorkspace`
  - `num_buf: []f64`：长度 `>= max(m * r, r * n)`（`nmfTransform` 仅需 `>= m_new * r`）
  - `den_buf: []f64`：长度 `>= max(m * r, r * n)`（`nmfTransform` 仅需 `>= m_new * r`）
  - `wh_buf: []f64`：长度 `>= m * n`
  - `rr_buf: []f64`：长度 `>= r * r`

### 2. 分解与投影

- `pub fn nmfFit(v: []const f64, m: usize, n: usize, r: usize, cfg: NmfConfig, w_out: *[]f64, h_out: *[]f64, ws: *NmfWorkspace): NmfResult?`
  将行主序非负矩阵 `v`（`m x n`）分解为非负因子 `w_out`（`m x r`）与 `h_out`（`r x n`）。任何维度为 0、包含负值/非有限值、非法配置或缓冲不足均返回 `null`。
- `pub fn nmfTransform(v_new: []const f64, m_new: usize, n: usize, r: usize, h_fixed: []const f64, cfg: NmfConfig, w_out: *[]f64, ws: *NmfWorkspace): NmfResult?`
  保持基矩阵 `h_fixed`（`r x n`）不变，仅更新 `w_out`（`m_new x r`）以将新非负样本 `v_new`（`m_new x n`）投影到低维非负子空间。

### 3. 重构与误差辅助

- `pub fn nmfReconstruct(w: []const f64, h: []const f64, m: usize, n: usize, r: usize, v_hat_out: *[]f64): bool`
  计算重构矩阵 $\hat{V} = W H$ 写入 `v_hat_out`。
- `pub fn nmfFrobeniusError(v: []const f64, w: []const f64, h: []const f64, m: usize, n: usize, r: usize): f64`
  零工作区计算 $\|V - W H\|_F$（非法输入返回 `NaN`）。
- `pub fn nmfKlDivergence(v: []const f64, w: []const f64, h: []const f64, m: usize, n: usize, r: usize, eps: f64): f64`
  零工作区计算广义 KL 散度 $\sum_{i,j} (V_{i,j}\ln\frac{V_{i,j}}{\max((WH)_{i,j},\epsilon)} - V_{i,j} + (WH)_{i,j})$（非法输入返回 `NaN`）。

## 示例

```cp
import { NmfConfig, NmfWorkspace, nmfFit, nmfReconstruct } from "cap:nmf";

fn demo() {
    const v: [6]f64 = [1.0, 2.0, 3.0, 2.0, 4.0, 6.0]; // 2x3 rank-1
    let w_buf: [2]f64 = [0.0; 2];
    let h_buf: [3]f64 = [0.0; 3];
    let num_buf: [3]f64 = [0.0; 3];
    let den_buf: [3]f64 = [0.0; 3];
    let wh_buf: [6]f64 = [0.0; 6];
    let rr_buf: [1]f64 = [0.0; 1];

    let w_s: []f64 = w_buf[0..];
    let h_s: []f64 = h_buf[0..];
    let ws: NmfWorkspace = {
        num_buf: num_buf[0..],
        den_buf: den_buf[0..],
        wh_buf: wh_buf[0..],
        rr_buf: rr_buf[0..],
    };

    const cfg = NmfConfig.default();
    if (res := nmfFit(v[0..], 2, 3, 1, cfg, *w_s, *h_s, *ws)) {
        * = res;
    }
}
```
