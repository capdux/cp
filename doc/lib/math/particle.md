# `cap:particle` — 纯 CP 零堆分配序贯蒙特卡洛 / 粒子滤波库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:particle` 提供 100% 纯 CP、零堆分配、确定性可复现的序贯蒙特卡洛（Sequential Monte Carlo / Particle Filter）非线性非高斯状态估计库，内置低方差系统重采样（`Systematic`）、分层重采样（`Stratified`）与多项式重采样（`Multinomial`），全程采用 log-sum-exp 权重更新防止高维/极小似然下溢。

## 特性概览

- **零堆分配（Zero-Alloc）**：粒子状态矩阵 `particles`（`N * dim`）、权重 `weights`（`N`）以及重采样临时缓冲全部由调用方提供。
- **确定性无状态 PRNG**：内置 SplitMix64 与 Box-Muller 高斯采样器，相同 `seed` 保证比特级完全复现。
- **数值稳定**：`normalizeLogWeights` 采用 log-sum-exp 平移防下溢并返回对数边际似然；自动计算有效样本数 $\text{ESS} = 1 / \sum w_i^2$ 实现自适应重采样。
- **完备边界防御**：自动拦截 `n_particles == 0`、`dim == 0`、负/全零权重、缓冲不足与 `NaN`/`+Inf`。

## API 参考

### 枚举与回调
- `ResampleKind`：`Systematic` | `Stratified` | `Multinomial`
- `PropagateFn = fn(state_inout: *[]f64, u: []const f64, dt: f64, noise: []const f64)`
- `LogLikelihoodFn = fn(state: []const f64, z: []const f64): f64`

### 独立原语
- `normalizeLogWeights(log_weights: []const f64, weights_out: *[]f64): f64?`
- `effectiveSampleSize(weights: []const f64): f64`
- `resampleIndices(weights: []const f64, kind: ResampleKind, seed: u64, indices_out: *[]usize): bool`

### `ParticleFilter` 方法
- `ParticleFilter.initUniform(particles_buf, weights_buf, n_particles, lower, upper, seed): ParticleFilter?`
- `ParticleFilter.initGaussian(particles_buf, weights_buf, n_particles, mean, std_dev, seed): ParticleFilter?`
- `predict(*_, prop_fn, u, dt, process_std, noise_scratch): bool`
- `update(*_, ll_fn, z, log_w_scratch): f64?`
- `resampleIfNeeded(*_, ess_threshold_ratio, kind, indices_s, tmp_particles): bool`
- `estimateMean(mean_out: *[]f64): bool`
- `estimateCovariance(mean: []const f64, cov_out: *[]f64): bool`

## 使用示例

```cp
import {
    ResampleKind,
    ParticleFilter,
} from "cap:math";

fn prop1d(st: *[]f64, u: []const f64, dt: f64, noise: []const f64) {
    st.*[0] = st.*[0] + (u[0] * dt) + noise[0];
}

fn ll1d(st: []const f64, z: []const f64): f64 {
    const diff: f64 = z[0] - st[0];
    return -0.5 * diff * diff;
}

let p_b: [64]f64 = [0.0; 64];
let w_b: [64]f64 = [0.0; 64];
let pts: []f64 = p_b[0..];
let wts: []f64 = w_b[0..];
const mean0: [1]f64 = [0.0];
const std0: [1]f64 = [1.0];

if (pf := ParticleFilter.initGaussian(*pts, *wts, 64, mean0[0..], std0[0..], 42)) {
    let mut_pf = pf;
    const u: [1]f64 = [1.0];
    const pstd: [1]f64 = [0.1];
    const z: [1]f64 = [1.05];
    let n_b: [1]f64 = [0.0];
    let lw_b: [64]f64 = [0.0; 64];
    let n_s: []f64 = n_b[0..];
    let lw_s: []f64 = lw_b[0..];
    * = mut_pf.predict(prop1d, u[0..], 1.0, pstd[0..], *n_s);
    * = mut_pf.update(ll1d, z[0..], *lw_s);
}
```
