# `cap:bandit` — 纯 CP 零堆分配多臂老虎机在线决策库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

`cap:bandit` 提供纯 CP、零堆分配的多臂老虎机（Multi-Armed Bandit）在线探索与利用算法套件，内置无全局状态确定性 SplitMix64 伪随机数生成器与 Marsaglia-Tsang Gamma/Beta 采样器，涵盖 UCB1、Epsilon-Greedy、Beta-Bernoulli Thompson Sampling 与对数权重防溢出 EXP3。

## 特性概览

- **零堆分配（Zero-Alloc）**：各臂计数、均值、Beta 超参及 EXP3 权重/概率切片均由调用方栈/切片内存提供。
- **完全确定性**：所有随机策略均由调用方显式传入 `seed: u64`，无全局可变状态，跨平台可 100% 复现。
- **数值稳健**：EXP3 每步执行最大对数权重就地平移（Log-Sum-Exp stabilization），运行任意长步数绝不上溢/下溢；Thompson Sampling 采用精确 Marsaglia-Tsang 采样。

## API 参考

### `Ucb1Bandit`
- `Ucb1Bandit.init(counts: []u64, means: []f64, c: f64): Ucb1Bandit?`
- `selectArm(): usize?`（冷启动保证先逐臂拉取一次）
- `update(*_, arm: usize, reward: f64): bool`
- `bestArm(): usize?`
- `armScore(arm: usize): f64`（未拉取返回 `+inf`，越界返回 `-inf`）

### `EpsilonGreedyBandit`
- `EpsilonGreedyBandit.init(counts: []u64, means: []f64, epsilon: f64, seed: u64): EpsilonGreedyBandit?`
- `setDecay(*_, decay: f64): bool`（配置反比衰减 $\varepsilon_t = \varepsilon / (1 + \text{decay} \cdot t)$）
- `selectArm(*_): usize?`、`update(*_, arm: usize, reward: f64): bool`、`bestArm(): usize?`

### `ThompsonBandit`
- `ThompsonBandit.init(alphas: []f64, betas: []f64, seed: u64): ThompsonBandit?`（默认均匀 `Beta(1, 1)` 先验）
- `selectArm(*_): usize?`、`update(*_, arm: usize, reward_01: f64): bool`、`bestArm(): usize?`

### `Exp3Bandit`
- `Exp3Bandit.init(log_weights: []f64, probs: []f64, gamma: f64, seed: u64): Exp3Bandit?`
- `selectArm(*_): usize?`、`update(*_, arm: usize, reward_01: f64): bool`、`probabilities(): []const f64`

### 辅助函数
- `cumulativeRegret(best_mean: f64, chosen_means: []const f64): f64`

## 使用示例

```cp
import std from "std";
import { Ucb1Bandit, ThompsonBandit } from "cap:ml";

let counts: [3]u64 = std.mem.zeroes([3]u64);
let means: [3]f64 = std.mem.zeroes([3]f64);
if (ucb_init := Ucb1Bandit.init(counts[0..], means[0..], 1.41421356)) {
    let ucb = ucb_init;
    const arm = ucb.selectArm() ?? 0;
    * = ucb.update(arm, 1.0);
}

let alphas: [3]f64 = std.mem.zeroes([3]f64);
let betas: [3]f64 = std.mem.zeroes([3]f64);
if (ts_init := ThompsonBandit.init(alphas[0..], betas[0..], 42)) {
    let ts = ts_init;
    const arm = ts.selectArm() ?? 0;
    * = ts.update(arm, 1.0);
}
```
