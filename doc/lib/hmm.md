# `cap:hmm` — 纯 CP 零堆分配离散隐马尔可夫模型（HMM）库

`cap:hmm` 提供纯 CP、零堆分配的离散隐马尔可夫模型（Discrete HMM）完整推理与训练工具集，全程采用对数空间 Viterbi 解码与 Rabiner 归一化缩放 Forward-Backward 算法，彻底杜绝长序列数值下溢。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部模型参数、前向/后向表、回溯表及 EM 重估输出均由调用方栈/切片内存提供。
- **防长序列下溢**：Viterbi 全程在对数域运行；Forward-Backward 与 Baum-Welch 采用 Rabiner 归一化缩放因子 $c_t$，支持数百至数千步长序列精确计算对数似然与后验分布。
- **单轮无额外矩阵 Baum-Welch EM**：`baumWelchStep` 直接在缩放前向/后向表上累加期望转移与发射计数并归一化，无需额外 `gamma` / `xi` 中间矩阵。
- **严格模型与边界校验**：`HmmModel.validate()` 校验维度、非负有限性与每行归一化；所有算法拦截越界观测符号、空序列及缓冲不足。

## API 参考

### 数据模型

```cp
pub struct HmmModel {
    num_states: usize,       // N
    num_obs: usize,          // M
    init_prob: []const f64,  // len >= N
    trans_prob: []const f64, // row-major N x N
    emit_prob: []const f64,  // row-major N x M
    fn validate(): bool;
}

pub struct ViterbiResult {
    log_prob: f64,
}
```

### 核心算法函数

- `viterbiDecode(model, obs, path_out: *[]usize, dp_prev_s: *[]f64, dp_curr_s: *[]f64, backptr_s: *[]usize): ViterbiResult?`
  对数域 Viterbi 最优隐状态路径解码（`path_out >= T`, `dp_prev_s/dp_curr_s >= N`, `backptr_s >= T * N`）。
- `forwardScaled(model, obs, alpha_out: *[]f64, scale_out: *[]f64): f64?`
  Rabiner 缩放前向算法，返回精确对数似然 $\ln P(O \mid \lambda)$（`alpha_out >= T * N`, `scale_out >= T`）。
- `backwardScaled(model, obs, scale, beta_out: *[]f64): bool`
  与 `forwardScaled` 配套的缩放后向算法（`beta_out >= T * N`）。
- `posteriorStates(alpha, beta, t_len, num_states, gamma_out: *[]f64): bool`
  计算各时刻归一化隐状态后验分布 $\gamma_t(i) = P(S_t = i \mid O, \lambda)$（每行和为 `1.0`）。
- `baumWelchStep(model, obs, next_init, next_trans, next_emit, alpha_s, beta_s, scale_s): f64?`
  单轮 Baum-Welch EM 参数重估，写入更新后的参数并返回更新前的对数似然。

## 使用示例

```cp
import std from "std";
import { HmmModel, viterbiDecode, forwardScaled } from "cap:hmm";

const pi: [2]f64 = [0.6, 0.4];
const trans: [4]f64 = [0.7, 0.3, 0.4, 0.6];
const emit: [6]f64 = [0.5, 0.4, 0.1, 0.1, 0.3, 0.6];
const model = HmmModel {
    num_states: 2, num_obs: 3,
    init_prob: pi[0..], trans_prob: trans[0..], emit_prob: emit[0..],
};

const obs: [3]usize = [0, 1, 2]; // Walk, Shop, Clean
let path_b: [3]usize = std.mem.zeroes([3]usize);
let dp0_b: [2]f64 = std.mem.zeroes([2]f64);
let dp1_b: [2]f64 = std.mem.zeroes([2]f64);
let bp_b: [6]usize = std.mem.zeroes([6]usize);
let path: []usize = path_b[0..];
let dp0: []f64 = dp0_b[0..];
let dp1: []f64 = dp1_b[0..];
let bp: []usize = bp_b[0..];

if (vres := viterbiDecode(model, obs[0..], *path, *dp0, *dp1, *bp)) {
    let alpha_b: [6]f64 = std.mem.zeroes([6]f64);
    let scale_b: [3]f64 = std.mem.zeroes([3]f64);
    let alpha: []f64 = alpha_b[0..];
    let scale: []f64 = scale_b[0..];
    const ll = forwardScaled(model, obs[0..], *alpha, *scale);
    * = vres;
    * = ll;
}
```
