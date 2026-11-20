# CP 标准库：概率分布、确定性采样与假设检验引擎手册 (`cap:prob` / `cap.prob`)

## 一、 架构总览与设计准则

`cap:prob` 是 CP 语言官方内置的纯原生、零 C 依赖的 **概率分布（PDF/PMF/CDF/PPF 逆分布）、确定性伪随机采样器（`Rng`）与统计假设检验（比例 Z-test / 均值 Z-test / 95% 置信区间）引擎**。

全模块专用于 CP 科学仿真、AI 概率推理与 A/B 实验统计显著性判定，支持 `import ... from "cap:prob"` 与 `import cap from "cap"; cap.prob.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/prob/dist.cp` | `// Source: cp/lib/prob/dist.cp (...)` | `ProbError`、`Rng`、四大概率分布与 `HypothesisTest` 引擎 |
| `lib/prob/index.cp` | `// Source: cp/lib/prob/index.cp (...)` | `cap:prob` 与 `cap.prob` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `ProbError`

- `InvalidParameter`：分布参数、范围或权重非法（如 `stddev <= 0`、`lambda <= 0`、`low > high`、`conv > total`、负权重或 `NaN`/`Inf`）
- `InvalidProbability`：概率或显著性水平 `alpha` 越界（不在 `[0, 1]` 或 `(0, 1)` 区间内）
- `EmptyWeights`：`weightedChoice` 传入空权重切片
- `SampleSizeTooSmall`：假设检验样本量为 0
- `OutOfMemory`：分配器内存不足

### 2. 确定性随机采样器 `Rng`（SplitMix64 + xoshiro256**）

- `Rng.init(seed: u64): Rng`：由 64 位种子经 SplitMix64 确定性初始化 256 位状态。
- `nextU64(*_): u64`：生成下一 64 位无符号随机整数。
- `nextF64(*_): f64`：生成 `[0, 1)` 区间 53 位均匀分布浮点数。
- `uniform(*_, low: f64, high: f64): f64 ! ProbError`：采样 `[low, high)` 均匀分布。
- `bernoulli(*_, p: f64): bool ! ProbError`：以概率 `p` 采样伯努利试验。
- `normal(*_, mean: f64, stddev: f64): f64 ! ProbError`：基于 Box-Muller 变换（带 spare 缓存）采样正态分布。
- `exponential(*_, lambda: f64): f64 ! ProbError`：基于逆变换采样指数分布。
- `poisson(*_, lambda: f64): u64 ! ProbError`：自适应切换 Knuth（`lambda < 30`）与 PTRS（`lambda >= 30`）采样泊松分布。
- `weightedChoice(*_, weights: []const f64): usize ! ProbError`：按非负权重切片进行加权随机索引选择。
- `shuffle(*_, slice: auto)`：原地 Fisher-Yates 洗牌。

### 3. 连续与离散概率分布

- **`NormalDist`**（`mean: f64`, `stddev: f64`）：
  - `NormalDist.init(mean, stddev): NormalDist ! ProbError` / `NormalDist.standard(): NormalDist`
  - `pdf(x: f64): f64`、 `cdf(x: f64): f64`（高精度 `erf` 近似，误差 `< 1e-14`）、`invCdf(p: f64): f64 ! ProbError`（Acklam 有理逼近，误差 `< 1.15e-9`）
- **`ExponentialDist`**（`lambda: f64`）：
  - `ExponentialDist.init(lambda): ExponentialDist ! ProbError`
  - `pdf(x: f64): f64`、`cdf(x: f64): f64`、`invCdf(p: f64): f64 ! ProbError`
- **`BinomialDist`**（`n: u64`, `p: f64`）：
  - `BinomialDist.init(n, p): BinomialDist ! ProbError`
  - `pmf(k: u64): f64`（基于 Lanczos `lgamma` 对数空间计算，彻底防止大 `n` 阶乘溢出）、`cdf(k: u64): f64`
- **`PoissonDist`**（`lambda: f64`）：
  - `PoissonDist.init(lambda): PoissonDist ! ProbError`
  - `pmf(k: u64): f64`、`cdf(k: u64): f64`

### 4. A/B 实验统计假设检验 (`AbTestResult` & `HypothesisTest`)

- **`AbTestResult`**：
  - `z_score: f64`、 `p_value: f64`（双尾）、`ci_low: f64`、`ci_high: f64`、`relative_lift: f64`、`significant: bool`
- **`HypothesisTest`**：
  - `twoProportionZTest(conv_a: u64, total_a: u64, conv_b: u64, total_b: u64, alpha: f64): AbTestResult ! ProbError`
  - `twoSampleZTest(mean_a: f64, std_a: f64, n_a: u64, mean_b: f64, std_b: f64, n_b: u64, alpha: f64): AbTestResult ! ProbError`

---

## 三、 使用示例

```typescript
import cap from "cap";
import { NormalDist, Rng, HypothesisTest } from "cap:prob";

pub fn runExperiment(): ! {
    const norm = NormalDist.standard();
    const z975 = norm.invCdf(0.975)?; // ~1.959964

    let rng = Rng.init(42);
    const sample = rng.normal(10.0, 2.0)?;

    const ab = HypothesisTest.twoProportionZTest(120, 1000, 180, 1000, 0.05)?;
    const same = cap.prob.HypothesisTest.twoSampleZTest(10.0, 2.0, 500, 10.1, 2.0, 500, 0.05)?;
    * = z975;
    * = sample;
    * = ab;
    * = same;
}
```
