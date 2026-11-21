# `cap:savitzky` — 纯 CP 零分配 Savitzky-Golay 平滑/微分与 LOESS 回归库

100% 纯 CP 零堆分配（Zero-Alloc）信号平滑与数值微分标准库：
- **Savitzky-Golay 滤波器**：支持任意奇数窗宽 `window_len >= 3`、多项式阶数 `poly_order < window_len` 与任意导数阶 `deriv_order <= poly_order`，通过局部 Vandermonde 正规方程精确求解零相位卷积核，支持平滑保峰与高精度 1/2 阶数值微分；
- **Cleveland (1979) LOESS / LOWESS**：基于 Tricube 核的 1D 局部加权线性/二次回归，支持非均匀采样与 Bisquare 鲁棒迭代抗离群点。

## 导入方式

```cp
import {
    BoundaryMode,
    SavgolWorkspace,
    LoessConfig,
    savgolCoeffs,
    savgolFilter,
    loessSmooth1D,
} from "cap:savitzky";
```

## API 契约

### 1. Savitzky-Golay 卷积核设计与滤波

- `pub enum BoundaryMode { Mirror, Nearest, ConstantZero }`
- `pub struct SavgolWorkspace { ata: []f64, rhs: []f64, sol: []f64, piv: []usize }`
  令 `m = poly_order + 1`，要求 `ata.len >= m*m`，`rhs.len >= m`，`sol.len >= m`，`piv.len >= m`。
- `pub fn savgolCoeffs(window_len: usize, poly_order: usize, deriv_order: usize, delta: f64, coeffs_out: *[]f64, ws: *SavgolWorkspace): bool`
  设计 `window_len` 点零相位卷积核写入 `coeffs_out.*[0..window_len]`（`delta > 0` 为采样间隔）。
- `pub fn savgolFilter(signal: []const f64, coeffs: []const f64, mode: BoundaryMode, out: *[]f64): bool`
  按指定边界模式对 `signal` 应用奇数长度卷积核 `coeffs`，写入 `out.*[0..signal.len]`。

### 2. LOESS / LOWESS 局部加权回归

- `pub struct LoessConfig { bandwidth: f64, robust_iters: usize, degree: usize }`
  - `LoessConfig.default()`：`{ bandwidth: 0.3, robust_iters: 2, degree: 1 }`
  - 约束：`bandwidth ∈ (0, 1]`，`degree ∈ {1, 2}`
- `pub fn loessSmooth1D(xs: []const f64, ys: []const f64, cfg: LoessConfig, y_out: *[]f64, rob_w_s: *[]f64, dist_s: *[]f64): bool`
  对任意（含非均匀/无序）采样点 `(xs, ys)` 执行 Tricube 核局部加权回归与 Bisquare 鲁棒迭代；`y_out`、`rob_w_s`、`dist_s` 长度均需 `>= xs.len`。

## 示例

```cp
import { BoundaryMode, SavgolWorkspace, savgolCoeffs, savgolFilter } from "cap:savitzky";

fn demo() {
    let ata: [9]f64 = [0.0; 9];
    let rhs: [3]f64 = [0.0; 3];
    let sol: [3]f64 = [0.0; 3];
    let piv: [3]usize = [0; 3];
    let ws: SavgolWorkspace = { ata: ata[0..], rhs: rhs[0..], sol: sol[0..], piv: piv[0..] };

    let k_buf: [5]f64 = [0.0; 5];
    let kernel: []f64 = k_buf[0..];
    if (savgolCoeffs(5, 2, 0, 1.0, *kernel, *ws)) {
        const sig: [5]f64 = [1.0, 2.0, 3.0, 4.0, 5.0];
        let out_buf: [5]f64 = [0.0; 5];
        let out_s: []f64 = out_buf[0..];
        * = savgolFilter(sig[0..], kernel, BoundaryMode.Mirror, *out_s);
    }
}
```
