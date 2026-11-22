# `cap:huber` — 纯 CP 零堆分配鲁棒统计与 M-估计回归库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

`cap:huber` 提供 100% 纯 CP、零堆分配的抗离群点鲁棒统计与回归工具集，包含鲁棒位置/尺度估计（`median`、`madScale`、`trimmedMean`、`huberLocation`）、Huber/Tukey 损失与权重原语、多元 IRLS 鲁棒线性回归（`huberRegress`，支持 Huber 与 Tukey Biweight 及可选 L2 岭正则）以及高崩溃点 1D Theil-Sen 中位数斜率回归（`theilSen1D`）。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部排序缓冲与 IRLS 正规方程工作区由调用方提供，原地无递归排序 + 列主元高斯消元。
- **抗极端离群点**：
  - `madScale` 采用 `1.4826 * median(|x - median(x)|)`（正态分布一致标准差估计）。
  - `tukeyBiweight` 在 `|r| >= c` 时权重彻底归零，配合凸 Huber 预热（warm-start）避免非凸局部陷阱。
  - `theilSen1D` 具备 29.3% 理论崩溃点。
- **完备边界防御**：自动拦截 `delta <= 0`、空数组、`trim_fraction` 越界、缓冲不足、垂直共线点与 `NaN`/`Inf`。

## API 参考

### 鲁棒位置与尺度估计
- `median(data: []const f64, scratch: *[]f64): f64?`
- `madScale(data: []const f64, scratch: *[]f64, dev_scratch: *[]f64): f64?`
- `trimmedMean(data: []const f64, trim_fraction: f64, scratch: *[]f64): f64?`
- `huberLocation(data: []const f64, k: f64, max_iter: usize, tol: f64, scratch: *[]f64, dev_scratch: *[]f64): f64?`

### 损失与权重原语
- `huberLoss(r: f64, delta: f64): f64`
- `huberWeight(r: f64, delta: f64): f64`
- `tukeyBiweight(r: f64, c: f64): f64`

### 多元 IRLS 鲁棒回归与 Theil-Sen 1D
- `RobustKind`：`Huber` | `Tukey`
- `HuberRegConfig`：`kind`, `delta`, `l2_reg`, `fit_intercept`, `max_iter`, `tol`；提供 `HuberRegConfig.defaultHuber()` / `HuberRegConfig.defaultTukey()`
- `HuberRegResult`：`intercept`, `scale`, `iterations`, `converged`, `loss`
- `HuberRegWorkspace`：`weights`, `residuals`, `ata`, `atb`, `beta`, `prev_beta`, `piv`, `scratch_a`, `scratch_b`
- `huberRegress(x, y, n_samples, n_features, cfg, w_out: *[]f64, ws: *HuberRegWorkspace): HuberRegResult?`
- `TheilSenResult`：`slope`, `intercept`
- `theilSen1D(xs, ys, slopes_scratch: *[]f64, intercepts_scratch: *[]f64): TheilSenResult?`

## 使用示例

```cp
import {
    median,
    madScale,
    theilSen1D,
} from "cap:ml";

const xs: [5]f64 = [0.0, 1.0, 2.0, 3.0, 4.0];
const ys: [5]f64 = [-1.0, 1.5, 4.0, 999.0, 9.0]; // 20% 极端离群点
let slopes_b: [10]f64 = [0.0; 10];
let ints_b: [5]f64 = [0.0; 5];
let slopes: []f64 = slopes_b[0..];
let ints: []f64 = ints_b[0..];

if (res := theilSen1D(xs[0..], ys[0..], *slopes, *ints)) {
    // res.slope == 2.5, res.intercept == -1.0
    * = res;
}
```
