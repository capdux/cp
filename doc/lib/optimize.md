# CP 标准库：数值求根与多维函数最优化引擎 (`cap:optimize` / `cap.optimize`)

## 一、 架构总览与设计准则

`cap:optimize` 是 CP 语言官方内置的纯原生、零 C 依赖的 **数值求根与多维函数最优化引擎**，面向科学仿真、参数标定、物理反演与轻量机器学习场景设计。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:optimize"` 与 `import cap from "cap"; cap.optimize.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/optimize/opt.cp` | `// Source: cp/lib/optimize/opt.cp (...)` | `OptError`、回调/结果类型、`Optimizer`（Bisection、Brent、Golden-Section、中心差分梯度、Nelder-Mead、Adam） |
| `lib/optimize/index.cp` | `// Source: cp/lib/optimize/index.cp (...)` | `cap:optimize` 与 `cap.optimize` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与回调类型 (`OptError`, `ScalarFn`, `VectorFn`, `GradFn`)

```typescript
pub err OptError {
    InvalidInterval,
    SameSignBracket,
    MaxIterationsExceeded,
    InvalidDimension,
    InvalidTolerance,
    OutOfMemory,
}

pub type ScalarFn = *const fn (*anyopaque, f64) f64;
pub type VectorFn = *const fn (*anyopaque, []const f64) f64;
pub type GradFn = *const fn (*anyopaque, []const f64, []f64) void;
```

### 2. 结果结构体 (`RootResult`, `MinResult1D`, `OptResult`)

- `RootResult`: `{ root: f64, f_val: f64, iterations: u32, converged: bool }`
- `MinResult1D`: `{ x: f64, f_val: f64, iterations: u32, converged: bool }`
- `OptResult`: `{ x: []f64, f_val: f64, iterations: u32, converged: bool }`
  - `deinit(*_, alloc: std.mem.Allocator)`: 释放由多维优化器分配的解向量 `x`。

### 3. 优化器套件 (`Optimizer`)

- **`Optimizer.bisect(ctx, f: ScalarFn, a: f64, b: f64, tol: f64, max_iter: u32): RootResult ! OptError`**
  - 一维二分法求根。校验 `tol > 0`、`a < b` 且 `f(a)` 与 `f(b)` 异号（同号返回 `SameSignBracket`）。
- **`Optimizer.brentRoot(ctx, f: ScalarFn, a: f64, b: f64, tol: f64, max_iter: u32): RootResult ! OptError`**
  - Brent 超线性求根算法（结合逆二次插值、割线法与二分法回退），收敛速度显著优于二分法。
- **`Optimizer.goldenSectionMin(ctx, f: ScalarFn, a: f64, b: f64, tol: f64, max_iter: u32): MinResult1D ! OptError`**
  - 黄金分割一维区间极小化（$\phi = (\sqrt{5}-1)/2$），适用于单谷函数极小点搜索。
- **`Optimizer.numericalGradient(ctx, f: VectorFn, x: []const f64, eps: f64, out_grad: []f64): void ! OptError`**
  - 中心差分数值梯度计算：$g_i = \frac{f(x + \epsilon e_i) - f(x - \epsilon e_i)}{2\epsilon}$。
- **`Optimizer.nelderMead(alloc, ctx, f: VectorFn, x0: []const f64, initial_step: f64, tol: f64, max_iter: u32): OptResult ! OptError`**
  - 无导数 Nelder-Mead 单纯形法（支持反射、扩张、内外收缩与整体缩减），返回 `OptResult`（需调用 `deinit(alloc)`）。
- **`Optimizer.adamMinimize(alloc, ctx, f: VectorFn, grad_fn_opt: GradFn?, x0: []const f64, lr: f64, tol: f64, max_iter: u32): OptResult ! OptError`**
  - 一阶 Adam 自适应矩估计优化器（$\beta_1=0.9, \beta_2=0.999, \epsilon=10^{-8}$ 含偏置修正）；当 `grad_fn_opt` 为 `null` 时自动使用中心差分数值梯度。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { Optimizer, OptError } from "cap:optimize";

fn cosMinusX(ctx: *anyopaque, x: f64): f64 {
    * = ctx;
    return @cos(x) - x;
}

fn rosenbrock(ctx: *anyopaque, x: []const f64): f64 {
    * = ctx;
    const a = 1.0 - x[0];
    const b = x[1] - x[0] * x[0];
    return a * a + 100.0 * b * b;
}

pub fn demoOptimize(): void ! OptError {
    const alloc = std.heap.page_allocator;
    let dummy: u8 = 0;
    const ctx: *anyopaque = unsafe { *dummy as *anyopaque };

    // 1. Brent 求根
    const r = Optimizer.brentRoot(ctx, cosMinusX, 0.0, 1.0, 0.000000000001, 100)?;
    _ = r.root;

    // 2. Nelder-Mead 无导数求解 Rosenbrock
    const x0 = [-1.2, 1.0];
    let res = Optimizer.nelderMead(alloc, ctx, rosenbrock, &x0, 0.2, 0.00000001, 2000)?;
    defer res.deinit(alloc);
}
```
