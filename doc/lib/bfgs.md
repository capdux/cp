# `cap:bfgs` — 零堆分配拟牛顿法与梯度优化库

`cap:bfgs` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的拟牛顿法数值优化引擎，包含维护完整逆 Hessian 的 `bfgsMinimize`、面向高维问题的有限内存 `lbfgsMinimize`（Two-Loop Recursion）、Nocedal & Wright 强 Wolfe 线搜索以及中心差分梯度计算。

## 核心特性

- **100% 零堆分配**：逆 Hessian 矩阵（BFGS）与 $O(m n)$ 环形历史缓冲（L-BFGS）全部由调用方以切片工作区传入。
- **完整 BFGS（`bfgsMinimize`）**：Sherman-Morrison-Woodbury 逆 Hessian 更新 + 首步缩放 + 曲率条件 $s^T y > 0$ 保护 + 非下降方向自动重置。
- **有限内存 L-BFGS（`lbfgsMinimize`）**：Two-Loop Recursion 仅需 $O(m n)$ 空间，适用于中高维无约束连续优化（如逻辑回归、高维非线性拟合）。
- **强 Wolfe 线搜索**：结合 Armijo 充分下降与强 Wolfe 曲率条件及区间缩放（Zoom），在 2D Rosenbrock 上 40 步内达到 `< 1e-10` 精度。
- **中心差分梯度（`finiteDiffGradient`）**：$O(\varepsilon^2)$ 二阶精度数值梯度验证与无解析梯度场景辅助。

---

## 导出清单

### 类型与结构

- `ValueGradFn = fn(x: []const f64, grad_out: *[]f64): f64`
- `ValueOnlyFn = fn(x: []const f64): f64`
- `BfgsConfig`：`{ grad_tol, step_tol, max_iter, c1_armijo, c2_wolfe, max_line_search }`，附 `BfgsConfig.default()`
- `BfgsResult`：`{ converged: bool, best_value: f64, grad_norm: f64, iterations: usize, evals: usize }`
- `BfgsWorkspace`：`{ h_inv, grad, grad_next, dir, x_next, s, y, hy }`
- `LbfgsWorkspace`：`{ s_hist, y_hist, rho_hist, alpha_hist, grad, grad_next, dir, x_next }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `bfgsMinimize(fg: ValueGradFn, x0: []const f64, x_out: *[]f64, cfg: BfgsConfig, ws: *BfgsWorkspace): BfgsResult?` | 完整 BFGS 拟牛顿极小化 |
| `lbfgsMinimize(fg: ValueGradFn, x0: []const f64, m: usize, x_out: *[]f64, cfg: BfgsConfig, ws: *LbfgsWorkspace): BfgsResult?` | 有限内存 L-BFGS（历史深度 `m`）极小化 |
| `finiteDiffGradient(f: ValueOnlyFn, x: []const f64, eps: f64, x_scratch: *[]f64, grad_out: *[]f64): bool` | 中心差分梯度计算 |

---

## 使用示例

```typescript
import std from "std";
import {
    BfgsConfig,
    BfgsWorkspace,
    bfgsMinimize,
} from "cap:bfgs";

fn rosenbrockFg(x: []const f64, g: *[]f64): f64 {
    const a = 1.0 - x[0];
    const b = x[1] - x[0] * x[0];
    g.*[0] = -2.0 * a - 400.0 * x[0] * b;
    g.*[1] = 200.0 * b;
    return a * a + 100.0 * b * b;
}

fn bfgsDemo() {
    const x0 = [-1.2, 1.0];
    let x_out: [2]f64 = std.mem.zeroes([2]f64);
    let h_inv: [4]f64 = std.mem.zeroes([4]f64);
    let grad: [2]f64 = std.mem.zeroes([2]f64);
    let grad_next: [2]f64 = std.mem.zeroes([2]f64);
    let dir: [2]f64 = std.mem.zeroes([2]f64);
    let x_next: [2]f64 = std.mem.zeroes([2]f64);
    let s: [2]f64 = std.mem.zeroes([2]f64);
    let y: [2]f64 = std.mem.zeroes([2]f64);
    let hy: [2]f64 = std.mem.zeroes([2]f64);

    let ws = BfgsWorkspace {
        h_inv: h_inv[0..],
        grad: grad[0..],
        grad_next: grad_next[0..],
        dir: dir[0..],
        x_next: x_next[0..],
        s: s[0..],
        y: y[0..],
        hy: hy[0..],
    };
    let xo_s: []f64 = x_out[0..];
    const res = bfgsMinimize(rosenbrockFg, x0[0..], &xo_s, BfgsConfig.default(), &ws);
    * = res;
}
```
