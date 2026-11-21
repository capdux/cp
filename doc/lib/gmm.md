# `cap:gmm` — 零堆分配高斯混合模型（GMM / EM）聚类与密度估计库

`cap:gmm` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的对角协方差多维高斯混合模型（Gaussian Mixture Model），基于确定性 SplitMix64 k-means++ 初始化、log-sum-exp 数值稳定 EM 迭代与方差下界正则，支持密度估计、后验概率预测、硬聚类与 AIC/BIC 模型选择。

## 核心特性

- **100% 零堆分配**：混合权重、均值矩阵、对角方差矩阵及责任度矩阵全部由调用方以切片传入。
- **确定性 k-means++ 初始化**：内置 SplitMix64 伪随机数生成器，复用责任度工作区零额外内存完成加权初始中心选取。
- **Log-Sum-Exp 数值稳定**：E-step 与密度评估全链路使用对数域稳定计算，杜绝高维下溢。
- **奇异塌缩防护**：M-step 强制施加 `reg_var` 方差下界，防止单点簇方差归零导致似然发散。
- **内置模型选择指标**：直接计算并返回总对数似然、AIC（$2p - 2\ln L$）与 BIC（$p\ln N - 2\ln L$）。

---

## 导出清单

### 数据结构

- `GmmConfig`：`{ max_iter: usize, tol: f64, reg_var: f64, seed: u64 }`，附 `GmmConfig.default()`
- `GmmFitResult`：`{ converged: bool, iterations: usize, log_likelihood: f64, aic: f64, bic: f64 }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `gmmFit(data, n_samples, n_features, k, cfg, weights_out, means_out, vars_out, resp_scratch): GmmFitResult?` | EM 算法拟合对角协方差 GMM |
| `gmmLogProbSample(sample, k, n_features, weights, means, vars): f64?` | 计算单样本对数概率密度 $\ln p(x)$ |
| `gmmPredictProba(data, n_samples, n_features, k, weights, means, vars, proba_out): bool` | 输出逐样本后验责任度矩阵（每行和为 `1.0`） |
| `gmmPredict(data, n_samples, n_features, k, weights, means, vars, labels_out): bool` | 输出硬聚类标签 $\arg\max_c P(c \mid x)$ |
| `gmmScoreSamples(data, n_samples, n_features, k, weights, means, vars, log_probs_out): f64?` | 输出逐样本对数密度并返回总对数似然 |

---

## 使用示例

```typescript
import std from "std";
import {
    GmmConfig,
    gmmFit,
    gmmPredict,
    gmmPredictProba,
} from "cap:gmm";

fn gmmDemo() {
    const data = [-5.1, -4.9, -5.0, -4.8, 4.9, 5.1, 5.0, 5.2];
    let weights: [2]f64 = std.mem.zeroes([2]f64);
    let means: [2]f64 = std.mem.zeroes([2]f64);
    let vars: [2]f64 = std.mem.zeroes([2]f64);
    let resp: [16]f64 = std.mem.zeroes([16]f64);
    let labels: [8]usize = std.mem.zeroes([8]usize);

    let w_s: []f64 = weights[0..];
    let m_s: []f64 = means[0..];
    let v_s: []f64 = vars[0..];
    let r_s: []f64 = resp[0..];
    let l_s: []usize = labels[0..];

    const res = gmmFit(data[0..], 8, 1, 2, GmmConfig.default(), &w_s, &m_s, &v_s, &r_s);
    * = gmmPredict(data[0..], 8, 1, 2, weights[0..], means[0..], vars[0..], &l_s);
    * = res;
}
```
