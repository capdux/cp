# `cap:ga` — 纯 CP 零堆分配遗传算法与演化计算库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:ga` 提供 100% 纯 CP、零堆分配、确定性可复现的遗传算法库，覆盖 **实数编码 GA（连续盒约束优化）** 与 **排列编码 GA（TSP / 排序调度组合优化）** 两大经典场景。

## 特性概览

- **零堆分配（Zero-Alloc）**：双缓冲种群 `pop_a`/`pop_b`、适应度 `fitness`、排序索引 `order` 与排列掩码 `used_mark` 均由调用方提供，零拷贝双缓冲代际切换。
- **确定性无状态 PRNG**：内置 SplitMix64 与 Box-Muller 高斯采样，相同 `seed` 保证比特级完全复现。
- **实数编码 GA**：锦标赛选择 + BLX-α 交叉 + 线性退火高斯变异 + 精英保留 + 盒边界钳位。
- **排列编码 GA**：锦标赛选择 + 标准 **Order Crossover (OX1)** 合法排列交叉 + 反转变异（2-opt 片段翻转）/ 交换变异 + 精英保留。
- **完备边界防御**：拦截 `pop_size < 2`、`lower >= upper`、越界交叉/变异率、非法精英数/锦标赛规模、缓冲不足以及 `NaN`/`Inf` 边界与目标值。

## API 参考

### 回调与结果类型
- `RealObjectiveFn = fn(x: []const f64): f64`
- `PermObjectiveFn = fn(perm: []const usize): f64`
- `GaResult`：`best_value: f64`, `generations: usize`, `evals: usize`

### 实数编码遗传算法
- `RealGaConfig`：`generations`, `crossover_rate`, `mutation_rate`, `mutation_sigma`, `tournament_k`, `elitism_count`, `seed`；提供 `RealGaConfig.default()`
- `RealGaWorkspace`：`pop_a: []f64`, `pop_b: []f64`, `fitness: []f64`, `order: []usize`
- `realGaMinimize(obj, lower, upper, pop_size, cfg, best_x_out: *[]f64, ws: *RealGaWorkspace): GaResult?`

### 排列编码遗传算法与排列算子
- `PermGaConfig`：`generations`, `crossover_rate`, `mutation_rate`, `tournament_k`, `elitism_count`, `seed`；提供 `PermGaConfig.default()`
- `PermGaWorkspace`：`pop_a: []usize`, `pop_b: []usize`, `fitness: []f64`, `order: []usize`, `used_mark: []u8`
- `isValidPermutation(perm: []const usize, seen_scratch: *[]u8): bool`
- `orderCrossover(p1, p2, cut1, cut2, child_out: *[]usize, used_mark: *[]u8): bool`
- `permGaMinimize(obj, n_genes, pop_size, cfg, best_perm_out: *[]usize, ws: *PermGaWorkspace): GaResult?`

## 使用示例

```cp
import {
    RealGaConfig,
    RealGaWorkspace,
    realGaMinimize,
} from "cap:math";

fn sphere(x: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < x.len) {
        s += x[i] * x[i];
        i += 1;
    }
    return s;
}

const lower: [2]f64 = [-5.0, -5.0];
const upper: [2]f64 = [5.0, 5.0];
let pa: [80]f64 = [0.0; 80];
let pb: [80]f64 = [0.0; 80];
let fit: [40]f64 = [0.0; 40];
let ord: [40]usize = [0; 40];
let best_b: [2]f64 = [0.0; 2];
let best: []f64 = best_b[0..];

let ws = RealGaWorkspace {
    pop_a: pa[0..],
    pop_b: pb[0..],
    fitness: fit[0..],
    order: ord[0..],
};

const cfg = RealGaConfig.default();
if (res := realGaMinimize(sphere, lower[0..], upper[0..], 40, cfg, *best, *ws)) {
    * = res.best_value;
}
```
