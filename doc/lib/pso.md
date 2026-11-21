# `cap:pso` — 零堆分配全局启发式优化库（PSO & DE）

`cap:pso` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的无导数全局启发式优化引擎，包含粒子群优化（`psoMinimize`）与经典差分进化（`deMinimize`，`DE/rand/1/bin` 策略）。内置无全局状态 SplitMix64 伪随机数生成器，保证相同 `seed` 的比特级确定性复现。

## 核心特性

- **100% 零堆分配**：粒子群位置/速度/个体最优（PSO）及种群/适应度/试验向量（DE）全部由调用方以切片工作区传入。
- **粒子群优化（`psoMinimize`）**：线性衰减惯性权重（`inertia_start -> inertia_end`）+ 速度钳位（`±0.5 * span`）+ 盒边界反射/钳位与阻尼反弹。
- **差分进化（`deMinimize`）**：经典 `DE/rand/1/bin` 变异与二项交叉策略，对高维非凸、多峰函数具备极强的全局寻优与逃离局部极小能力。
- **确定性复现**：无全局随机状态，相同 `seed` 在相同输入下产生完全一致的优化轨迹与比特级一致结果。
- **完整防御校验**：自动拦截非法边界（`lower >= upper`、`NaN`/`Inf`）、空/过小种群（`swarm_size == 0` / `pop_size < 4`）、缓冲不足及全非有限目标值。

---

## 导出清单

### 类型与结构

- `ObjectiveFn = fn(x: []const f64): f64`
- `GlobalOptResult`：`{ best_value: f64, iterations: usize, evals: usize, converged: bool }`
- `PsoConfig`：`{ inertia_start, inertia_end, cognitive_c1, social_c2, max_iter, tol, seed }`，附 `PsoConfig.default()`
- `PsoWorkspace`：`{ pos, vel, pbest_pos, pbest_val }`（要求 `pos/vel/pbest_pos.len >= swarm_size * dim`，`pbest_val.len >= swarm_size`）
- `DeConfig`：`{ mutation_f, crossover_cr, max_iter, tol, seed }`，附 `DeConfig.default()`
- `DeWorkspace`：`{ pop, fitness, trial }`（要求 `pop.len >= pop_size * dim`，`fitness.len >= pop_size`，`trial.len >= dim`）

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `psoMinimize(obj: ObjectiveFn, lower: []const f64, upper: []const f64, swarm_size: usize, cfg: PsoConfig, best_x_out: *[]f64, ws: *PsoWorkspace): GlobalOptResult?` | 粒子群优化（盒约束全局极小化） |
| `deMinimize(obj: ObjectiveFn, lower: []const f64, upper: []const f64, pop_size: usize, cfg: DeConfig, best_x_out: *[]f64, ws: *DeWorkspace): GlobalOptResult?` | 差分进化 `DE/rand/1/bin`（盒约束全局极小化，`pop_size >= 4`） |

---

## 使用示例

```typescript
import std from "std";
import {
    PsoConfig,
    PsoWorkspace,
    DeConfig,
    DeWorkspace,
    psoMinimize,
    deMinimize,
} from "cap:pso";

fn sphere2d(x: []const f64): f64 {
    return x[0] * x[0] + x[1] * x[1];
}

fn demo() {
    const lower = [-5.0, -5.0];
    const upper = [5.0, 5.0];

    // 1. Particle Swarm Optimization (swarm_size = 24, dim = 2)
    let best_x: [2]f64 = std.mem.zeroes([2]f64);
    let pos: [48]f64 = std.mem.zeroes([48]f64);
    let vel: [48]f64 = std.mem.zeroes([48]f64);
    let pbest_pos: [48]f64 = std.mem.zeroes([48]f64);
    let pbest_val: [24]f64 = std.mem.zeroes([24]f64);
    let pso_ws = PsoWorkspace {
        pos: pos[0..],
        vel: vel[0..],
        pbest_pos: pbest_pos[0..],
        pbest_val: pbest_val[0..],
    };
    let bx_s: []f64 = best_x[0..];
    const pso_res = psoMinimize(sphere2d, lower[0..], upper[0..], 24, PsoConfig.default(), &bx_s, &pso_ws);
    * = pso_res;

    // 2. Differential Evolution DE/rand/1/bin (pop_size = 20, dim = 2)
    let pop: [40]f64 = std.mem.zeroes([40]f64);
    let fitness: [20]f64 = std.mem.zeroes([20]f64);
    let trial: [2]f64 = std.mem.zeroes([2]f64);
    let de_ws = DeWorkspace {
        pop: pop[0..],
        fitness: fitness[0..],
        trial: trial[0..],
    };
    const de_res = deMinimize(sphere2d, lower[0..], upper[0..], 20, DeConfig.default(), &bx_s, &de_ws);
    * = de_res;
}
```
