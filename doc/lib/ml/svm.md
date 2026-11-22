# `cap:svm` — 纯 CP 零堆分配支持向量机（SVM）分类库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

`cap:svm` 提供纯 CP、零堆分配的支持向量机二分类库，同时涵盖基于 **Platt SMO（Sequential Minimal Optimization）** 的核 SVM 对偶求解器（支持 `Linear` / `Poly` / `Rbf` 核）以及基于 **Pegasos** 随机次梯度下降的高速原问题线性 SVM。

## 特性概览

- **零堆分配（Zero-Alloc）**：拉格朗日乘子 `alphas_out`、误差缓存 `errors_scratch` 与线性权重 `w_out` 均由调用方栈/切片内存提供，无 $O(n^2)$ 核矩阵堆分配。
- **Platt 完整启发式 SMO**：维护增量误差缓存 $E_k = f(x_k) - y_k$、采用最大 $|E_1 - E_2|$ 二阶启发式选择配对变量，保证 $\sum \alpha_i y_i = 0$ 与 $0 \le \alpha_i \le C$ 严格成立。
- **高速线性 Pegasos**：$O(d)$ 单步更新，无需核计算，内置确定性 SplitMix64 采样。
- **完备边界防御**：自动拦截非法标签（非 `±1.0`）、单类别样本、`C <= 0` / `lambda <= 0` / `gamma <= 0`、缓冲不足与 `NaN`/`Inf`。

## API 参考

### 核函数
- `KernelKind`（`Linear`, `Poly`, `Rbf`）
- `KernelParams`（`KernelParams.linear()`, `KernelParams.rbf(gamma)`, `KernelParams.poly(gamma, coef0, degree)`）
- `evalKernel(a: []const f64, b: []const f64, kp: KernelParams): f64`

### 核 SVM（Platt SMO）
- `SmoConfig`（`c`, `tol`, `eps`, `max_passes`, `max_iter`；`SmoConfig.default()`）
- `SmoResult`（`bias`, `support_vectors`, `iterations`, `converged`）
- `svmFitSmo(x, y, n_samples, n_features, kp, cfg, alphas_out: *[]f64, errors_scratch: *[]f64): SmoResult?`
- `svmDecision(x_train, y_train, alphas, bias, n_train, n_features, kp, query): f64?`
- `svmPredict(x_train, y_train, alphas, bias, n_train, n_features, kp, query): f64?`

### 线性 SVM（Pegasos）
- `LinearSvmResult`（`bias`, `iterations`）
- `linearSvmFit(x, y, n_samples, n_features, lambda, epochs, seed, w_out: *[]f64): LinearSvmResult?`
- `linearSvmDecision(w: []const f64, bias: f64, query: []const f64): f64?`

## 使用示例

```cp
import std from "std";
import { KernelParams, SmoConfig, svmFitSmo, svmPredict } from "cap:ml";

const x: [8]f64 = [
    -1.0, -1.0,
     1.0,  1.0,
    -1.0,  1.0,
     1.0, -1.0,
];
const y: [4]f64 = [1.0, 1.0, -1.0, -1.0];

let alphas_b: [4]f64 = std.mem.zeroes([4]f64);
let errs_b: [4]f64 = std.mem.zeroes([4]f64);
let alphas: []f64 = alphas_b[0..];
let errs: []f64 = errs_b[0..];

const kp = KernelParams.rbf(1.0);
let cfg = SmoConfig.default();
cfg.c = 10.0;

if (res := svmFitSmo(x[0..], y[0..], 4, 2, kp, cfg, *alphas, *errs)) {
    const q: [2]f64 = [0.9, 1.1];
    const pred = svmPredict(x[0..], y[0..], alphas, res.bias, 4, 2, kp, q[0..]); // Some(1.0)
    * = pred;
}
```
