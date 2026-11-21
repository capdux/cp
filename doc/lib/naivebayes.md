# `cap:naivebayes` — 纯 CP 零堆分配朴素贝叶斯分类库

`cap:naivebayes` 提供 100% 纯 CP、零堆分配的三大经典朴素贝叶斯多分类器：**Gaussian Naive Bayes**（连续特征）、**Multinomial Naive Bayes**（文本词频/非负计数特征）与 **Bernoulli Naive Bayes**（二值特征及显式缺失特征惩罚）。全链路采用对数概率与 **log-sum-exp** 归一化，避免高维特征连乘下溢。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部模型参数（`log_priors`、`means`、`vars`、`feature_log_probs`、`feat_log_prob_1`/`0`）与预测概率缓冲均由调用方提供。
- **高维数值稳定**：对数空间累加条件似然 + log-sum-exp 平移归一化，100+ 维特征下仍保持后验概率严格和为 1.0 且无下溢。
- **平滑与防除零**：
  - Gaussian：`var_smoothing` 自适应缩放最大方差（全常数特征回退至原始 `var_smoothing`），防止零方差除零。
  - Multinomial：Laplace/Lidstone 平滑 `alpha >= 0`，保证未见词具有非零概率。
  - Bernoulli：`x > binarize_threshold` 阈值二值化 + `alpha` 平滑（分母 $N_c + 2\alpha$），显式使用 $\log P(x_j = 0 \mid c)$ 惩罚缺失特征。
- **完备边界防御**：拦截空输入、越界类别、负计数、负平滑系数、缓冲不足与 `NaN`/`Inf`。

## API 参考

### Gaussian Naive Bayes
- `GaussianNbModel`：`num_classes`, `num_features`, `log_priors`, `means`, `vars`, `class_counts`
- `fitGaussianNb(x, y, n_samples, n_features, num_classes, var_smoothing, log_priors_out, means_out, vars_out, counts_out): GaussianNbModel?`
- `gaussianNbPredictProba(model, sample, proba_out: *[]f64): bool`
- `gaussianNbPredict(model, sample, proba_scratch: *[]f64): usize?`

### Multinomial Naive Bayes
- `MultinomialNbModel`：`num_classes`, `num_features`, `log_priors`, `feature_log_probs`
- `fitMultinomialNb(counts, y, n_samples, n_features, num_classes, alpha, log_priors_out, feat_log_probs_out, class_counts_s): MultinomialNbModel?`
- `multinomialNbPredictProba(model, sample, proba_out: *[]f64): bool`
- `multinomialNbPredict(model, sample, proba_scratch: *[]f64): usize?`

### Bernoulli Naive Bayes
- `BernoulliNbModel`：`num_classes`, `num_features`, `binarize_threshold`, `log_priors`, `feat_log_prob_1`, `feat_log_prob_0`
- `fitBernoulliNb(x, y, n_samples, n_features, num_classes, alpha, binarize_threshold, log_priors_out, log_p1_out, log_p0_out, class_counts_s): BernoulliNbModel?`
- `bernoulliNbPredictProba(model, sample, proba_out: *[]f64): bool`
- `bernoulliNbPredict(model, sample, proba_scratch: *[]f64): usize?`

## 使用示例

```cp
import {
    fitGaussianNb,
    gaussianNbPredict,
    gaussianNbPredictProba,
} from "cap:naivebayes";

const x: [8]f64 = [
    -2.0, -1.5,  -1.8, -2.2,
     2.1,  1.9,   1.7,  2.3,
];
const y: [4]usize = [0, 0, 1, 1];

let lp_b: [2]f64 = [0.0; 2];
let m_b: [4]f64 = [0.0; 4];
let v_b: [4]f64 = [0.0; 4];
let c_b: [2]usize = [0; 2];
let lp: []f64 = lp_b[0..];
let m: []f64 = m_b[0..];
let v: []f64 = v_b[0..];
let c: []usize = c_b[0..];

if (model := fitGaussianNb(x[0..], y[0..], 4, 2, 2, 1.0e-9, *lp, *m, *v, *c)) {
    let prob_b: [2]f64 = [0.0; 2];
    let prob: []f64 = prob_b[0..];
    const q: [2]f64 = [2.0, 2.0];
    const cls = gaussianNbPredict(model, q[0..], *prob); // Some(1)
    * = cls;
}
```
