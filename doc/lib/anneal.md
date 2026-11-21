# `cap:anneal` — 零堆分配模拟退火全局优化库（Simulated Annealing）

`cap:anneal` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的模拟退火全局优化引擎，同时覆盖**连续盒约束优化**（`annealContinuous`）与**离散排列组合优化**（`annealPermutation`，2-opt 区间反转 + 随机交换混合邻域，适用于 TSP/QAP）。内置无全局状态 SplitMix64 确定性 PRNG，保证相同 `seed` 的比特级确定性复现。

## 核心特性

- **100% 零堆分配**：当前解、候选解与历史最优解均由调用方切片工作区承载，无任何动态内存分配。
- **四种经典降温计划（`CoolingSchedule`）**：`Geometric`（指数几何）、`Linear`（线性）、`Fast`（Cauchy $T_0 / (1 + k)$）与 `Logarithmic`（Boltzmann $T_0 / (1 + \ln(1 + k))$）。
- **Metropolis 接受准则（`metropolisAccept`）**：能量下降或持平必接受，能量上升按 $\exp(-\Delta E / T)$ 概率接受以逃离局部极小。
- **连续盒约束退火（`annealContinuous`）**：温度自适应高斯步长缩放 + 全维/单维混合提议 + 盒边界反射钳位。
- **离散排列退火（`annealPermutation`）**：内置 $O(n)$ 零分配排列合法性校验 + 80% 2-opt 区间反转 / 20% 随机交换混合邻域。

---

## 导出清单

### 枚举、类型与结构体

- `CoolingSchedule`：`Geometric | Linear | Fast | Logarithmic`
- `RealEnergyFn = fn(x: []const f64): f64`
- `PermEnergyFn = fn(perm: []const usize): f64`
- `AnnealConfig`：`{ t_init, t_min, cooling_rate, step_scale, max_steps, schedule, seed }`，附 `AnnealConfig.default()`
- `AnnealResult`：`{ best_energy, final_energy, accepted, steps, final_temp }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `temperatureAt(t0, t_min, alpha, step, max_steps, schedule): f64` | 计算第 `step` 步退火温度 |
| `metropolisAccept(delta_energy, temp, u01): bool` | Metropolis-Hastings 接受判定 |
| `annealContinuous(energy_fn, x0, lower, upper, cfg, best_x_out, curr_s, cand_s): AnnealResult?` | 连续盒约束模拟退火 |
| `annealPermutation(energy_fn, init_perm, cfg, best_perm_out, curr_s, cand_s): AnnealResult?` | 离散排列（2-opt / swap）模拟退火 |

---

## 使用示例

```typescript
import std from "std";
import {
    AnnealConfig,
    annealContinuous,
} from "cap:anneal";

fn sphere2d(x: []const f64): f64 {
    return x[0] * x[0] + x[1] * x[1];
}

fn annealDemo() {
    const x0 = [3.5, -2.5];
    const lower = [-5.0, -5.0];
    const upper = [5.0, 5.0];
    let best_x: [2]f64 = std.mem.zeroes([2]f64);
    let curr: [2]f64 = std.mem.zeroes([2]f64);
    let cand: [2]f64 = std.mem.zeroes([2]f64);
    let best_s: []f64 = best_x[0..];
    let curr_s: []f64 = curr[0..];
    let cand_s: []f64 = cand[0..];

    const res = annealContinuous(
        sphere2d,
        x0[0..],
        lower[0..],
        upper[0..],
        AnnealConfig.default(),
        &best_s,
        &curr_s,
        &cand_s,
    );
    * = res;
}
```
