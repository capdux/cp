# `cap:logreg` — 纯 CP 零分配逻辑回归库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

100% 纯 CP 零堆分配（Zero-Alloc）的二分类与多分类（Softmax / Multinomial）L2 正则逻辑回归标准库。所有权重矩阵、偏置向量与梯度工作区缓冲均由调用方在栈或静态缓冲区提供，使用带 Armijo 自适应回溯线搜索的确定性批量梯度下降，保证凸目标函数单调稳定收敛。

## 导入方式

```cp
import {
    sigmoid,
    logLossBinary,
    softmax,
    LogRegConfig,
    LogRegResult,
    MultinomialResult,
    ClassificationMetrics,
    fitBinary,
    predictProbaBinary,
    predictBinary,
    fitMultinomial,
    predictProbaMultinomial,
    predictClassMultinomial,
    binaryMetrics,
} from "cap:ml";
```

## API 契约

### 1. 数值稳定基础原语

- `pub fn sigmoid(z: f64): f64`
  分段防溢出 Logistic 函数 $\sigma(z) = 1 / (1 + e^{-z})$；对极大/极小实数与 $\pm\infty$ 安全返回 `1.0` / `0.0`，不发生 `exp` 溢出。
- `pub fn logLossBinary(probs: []const f64, y: []const f64): f64?`
  二分类平均对数损失（Binary Cross-Entropy），自动将概率裁剪至 $[10^{-15}, 1 - 10^{-15}]$ 避免 $\ln(0)$。空切片、长度不匹配、概率越界、非 `{0.0, 1.0}` 标签或非有限值返回 `null`。
- `pub fn softmax(logits: []const f64, probs_out: *[]f64): bool`
  最大值平移（max-shift）数值稳定 Softmax，结果写入 `probs_out.*[0..logits.len]`。空输入、输出缓冲不足或存在 `NaN`/`Inf` 返回 `false`。

### 2. 配置与结果类型

- `pub struct LogRegConfig`
  - 字段：`learning_rate: f64`, `l2_reg: f64`, `max_iter: usize`, `tol: f64`
  - 默认工厂：`LogRegConfig.default()`（`learning_rate: 0.5, l2_reg: 1e-4, max_iter: 1000, tol: 1e-6`）
- `pub struct LogRegResult`
  - 字段：`bias: f64`, `loss: f64`, `grad_norm: f64`, `iterations: usize`, `converged: bool`
- `pub struct MultinomialResult`
  - 字段：`loss: f64`, `grad_norm: f64`, `iterations: usize`, `converged: bool`
- `pub struct ClassificationMetrics`
  - 字段：`tp: usize`, `fp: usize`, `tn: usize`, `fn_count: usize`, `accuracy: f64`, `precision: f64`, `recall: f64`, `f1: f64`

### 3. 二分类 L2 逻辑回归

- `pub fn fitBinary(x: []const f64, y: []const f64, n_samples: usize, n_features: usize, cfg: LogRegConfig, w_out: *[]f64, grad_scratch: *[]f64): LogRegResult?`
  行主序特征矩阵 `x`（`n_samples * n_features`），二值标签 `y[i] ∈ {0.0, 1.0}`；优化目标为 $\frac{1}{N}\sum \text{softplus}(z_i) - y_i z_i + \frac{\lambda}{2}\|w\|_2^2$（偏置不参与正则惩罚）。权重写入 `w_out.*[0..n_features]`。
- `pub fn predictProbaBinary(w: []const f64, bias: f64, sample: []const f64): f64?`
  计算单样本正类概率 $\sigma(w \cdot \text{sample} + \text{bias})$。
- `pub fn predictBinary(w: []const f64, bias: f64, sample: []const f64, threshold: f64): u8?`
  按阈值 `threshold ∈ [0, 1]` 输出 `1` 或 `0`。

### 4. 多分类 Softmax L2 逻辑回归

- `pub fn fitMultinomial(x: []const f64, y_class: []const usize, n_samples: usize, n_features: usize, n_classes: usize, cfg: LogRegConfig, w_out: *[]f64, b_out: *[]f64, grad_w_s: *[]f64, grad_b_s: *[]f64, prob_s: *[]f64): MultinomialResult?`
  行主序权重矩阵 `w_out`（`>= n_classes * n_features`，第 `c` 类权重位于 `w[c * n_features .. (c + 1) * n_features]`），偏置 `b_out`（`>= n_classes`），工作区缓冲 `grad_w_s`、`grad_b_s`、`prob_s` 均由调用方提供。要求 `n_classes >= 2` 且 `y_class[i] < n_classes`。
- `pub fn predictProbaMultinomial(w: []const f64, b: []const f64, n_classes: usize, n_features: usize, sample: []const f64, proba_out: *[]f64): bool`
  计算单样本在 `n_classes` 个类别上的归一化概率分布写入 `proba_out.*[0..n_classes]`。
- `pub fn predictClassMultinomial(w: []const f64, b: []const f64, n_classes: usize, n_features: usize, sample: []const f64, proba_scratch: *[]f64): usize?`
  返回最大概率对应的类别索引（`0..n_classes - 1`）。

### 5. 二分类评估指标

- `pub fn binaryMetrics(probs: []const f64, y: []const f64, threshold: f64): ClassificationMetrics?`
  统计 `tp` / `fp` / `tn` / `fn_count` 并计算 `accuracy`、`precision`、`recall`、`f1`（零分母安全回退为 `0.0`）。

## 示例

```cp
import {
    LogRegConfig,
    fitBinary,
    predictProbaBinary,
    predictBinary,
    logLossBinary,
} from "cap:ml";

fn demo() {
    const x: [8]f64 = [
        -2.0, -1.5,
        -1.5, -2.0,
         2.0,  1.5,
         1.5,  2.0,
    ];
    const y: [4]f64 = [0.0, 0.0, 1.0, 1.0];

    let w_buf: [2]f64 = [0.0, 0.0];
    let g_buf: [2]f64 = [0.0, 0.0];
    let w_s: []f64 = w_buf[0..];
    let g_s: []f64 = g_buf[0..];

    const cfg = LogRegConfig.default();
    if (res := fitBinary(x[0..], y[0..], 4, 2, cfg, *w_s, *g_s)) {
        const sample: [2]f64 = [1.8, 1.6];
        if (cls := predictBinary(w_s, res.bias, sample[0..], 0.5)) {
            * = cls;
        }
    }
}
```
