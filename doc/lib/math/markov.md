# `cap:markov` — 零分配离散时间马尔可夫链（DTMC）分析与采样库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

零堆分配（Zero-Alloc）的纯 CP 离散时间马尔可夫链（DTMC）数值分析与蒙特卡洛采样标准库，提供随机转移矩阵校验、分布演化、Chapman-Kolmogorov 二进制快速幂、平稳分布幂迭代、吸收马尔可夫链基本矩阵 $N = (I - Q)^{-1}$ / 期望吸收步数 / 吸收概率、Shannon 熵率以及确定性 SplitMix64 轨迹采样。

## 核心特性

- **零堆分配（Zero-Alloc）**：全部矩阵幂、LU 求逆工作区与轨迹采样输出由调用方预分配切片提供。
- **Chapman-Kolmogorov 快速幂**：`transitionPower` 以 $O(n^3 \log k)$ 计算任意步数 $P^k$。
- **吸收链精确解析**：`absorbingFundamental` 内置部分选主元 LU 求逆计算 $N = (I - Q)^{-1}$，配合 `expectedAbsorptionSteps`（$N \mathbf{1}$）与 `absorptionProbabilities`（$B = N R$）一键求得精确期望吸收时间与吸收分布。
- **确定性轨迹采样与信息论度量**：`sampleTrajectory` 使用 53-bit IEEE-754 精度 SplitMix64 行 CDF 采样；`entropyRate` 计算平稳链每步比特熵率。

## 导出 API

### 数据模型

- `StationaryResult { converged: bool, iterations: usize, residual_l1: f64 }`

### 核心函数

- `isStochasticMatrix(p: []const f64, n: usize, tol: f64): bool`
- `stepDistribution(dist: []const f64, p: []const f64, n: usize, next_out: []f64): bool`
- `transitionPower(p: []const f64, n: usize, steps: u64, out_mat: []f64, base_s: []f64, tmp_s: []f64): bool`
- `stationaryPowerIter(p: []const f64, n: usize, pi_out: []f64, tmp_s: []f64, max_iter: usize, tol: f64): StationaryResult?`
- `absorbingFundamental(q_mat: []const f64, t: usize, fundamental_out: []f64, lu_scratch: []f64, piv_scratch: []usize, rhs_s: []f64, col_s: []f64): bool`
- `expectedAbsorptionSteps(fundamental: []const f64, t: usize, steps_out: []f64): bool`
- `absorptionProbabilities(fundamental: []const f64, r_mat: []const f64, t: usize, r: usize, probs_out: []f64): bool`
- `sampleTrajectory(p: []const f64, n: usize, start_state: usize, seed: u64, path_out: []usize): bool`
- `entropyRate(p: []const f64, pi: []const f64, n: usize): f64?`

## 使用示例

```cp
import std from "std";
import {
    stationaryPowerIter,
    entropyRate,
    sampleTrajectory,
} from "cap:math";

fn example() {
    const p: [9]f64 = [
        0.7, 0.2, 0.1,
        0.3, 0.4, 0.3,
        0.2, 0.3, 0.5,
    ];
    let pi: [3]f64 = std.mem.zeroes([3]f64);
    let tmp: [3]f64 = std.mem.zeroes([3]f64);

    if (res := stationaryPowerIter(p[0..], 3, pi[0..], tmp[0..], 500, 1.0e-13)) {
        * = res.converged;
        * = entropyRate(p[0..], pi[0..], 3);
    }

    let path: [64]usize = std.mem.zeroes([64]usize);
    * = sampleTrajectory(p[0..], 3, 0, 42, path[0..]);
}
```
