# `cap:nelder` — 零堆分配无导数优化与一维极值/求根库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:nelder` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的无导数数值优化与一维求根工具集，涵盖多维 Nelder-Mead 单纯形优化、一维黄金分割极小化与 Brent-Dekker 超线性求根法。

## 核心特性

- **100% 零堆分配**：单纯形顶点池与反射/扩张/收缩临时向量均由调用方通过 `NelderWorkspace` 传入。
- **Nelder-Mead 单纯形法（`nelderMead`）**：完整实现反射（$\alpha = 1$）、扩张（$\gamma = 2$）、外/内收缩（$\rho = 0.5$）与整体收缩（$\sigma = 0.5$），双重容差（函数跨度 `tol_f` 与单纯形直径 `tol_x`）判定收敛。
- **黄金分割极小化（`goldenSectionMin`）**：每步仅需一次新函数评估，严格保证单峰区间线性收敛。
- **Brent 求根法（`brentRoot`）**：结合逆二次插值、割线法与二分回退，兼顾超线性收敛速度与括号区间安全保障。

---

## 导出清单

### 类型与结构

- `ObjectiveFn = fn([]const f64): f64`
- `ScalarFn = fn(f64): f64`
- `NelderConfig`：`{ alpha, gamma, rho, sigma, tol_f, tol_x, max_iter, init_step }`，附 `NelderConfig.default()`
- `NelderWorkspace`：`{ simplex: []f64, fvals: []f64, centroid: []f64, xr: []f64, xe: []f64, xc: []f64 }`
- `NelderResult`：`{ converged: bool, best_value: f64, iterations: usize, evals: usize }`
- `Minimize1DResult`：`{ x: f64, fval: f64, iterations: usize, converged: bool }`
- `Root1DResult`：`{ root: f64, fval: f64, iterations: usize, converged: bool }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `nelderMead(obj: ObjectiveFn, x0: []const f64, x_out: *[]f64, cfg: NelderConfig, ws: *NelderWorkspace): NelderResult?` | 多维无导数单纯形极小化 |
| `goldenSectionMin(f: ScalarFn, a: f64, b: f64, tol: f64, max_iter: usize): Minimize1DResult?` | 一维区间 `[a, b]` 黄金分割极小化 |
| `brentRoot(f: ScalarFn, a_init: f64, b_init: f64, tol: f64, max_iter: usize): Root1DResult?` | 一维括号区间 Brent 求根法（端点同号返回 `null`） |

---

## 使用示例

```typescript
import std from "std";
import {
    NelderConfig,
    NelderWorkspace,
    nelderMead,
    goldenSectionMin,
    brentRoot,
} from "cap:math";

fn rosenbrock(x: []const f64): f64 {
    const a = 1.0 - x[0];
    const b = x[1] - x[0] * x[0];
    return a * a + 100.0 * b * b;
}

fn dottie(x: f64): f64 {
    return @cos(x) - x;
}

fn optimizeDemo() {
    const x0 = [-1.2, 1.0];
    let x_out: [2]f64 = std.mem.zeroes([2]f64);
    let simplex: [6]f64 = std.mem.zeroes([6]f64);
    let fvals: [3]f64 = std.mem.zeroes([3]f64);
    let c: [2]f64 = std.mem.zeroes([2]f64);
    let xr: [2]f64 = std.mem.zeroes([2]f64);
    let xe: [2]f64 = std.mem.zeroes([2]f64);
    let xc: [2]f64 = std.mem.zeroes([2]f64);

    let ws = NelderWorkspace {
        simplex: simplex[0..],
        fvals: fvals[0..],
        centroid: c[0..],
        xr: xr[0..],
        xe: xe[0..],
        xc: xc[0..],
    };
    let xo_s: []f64 = x_out[0..];
    const res = nelderMead(rosenbrock, x0[0..], &xo_s, NelderConfig.default(), &ws);
    const root = brentRoot(dottie, 0.0, 1.0, 1.0e-12, 100);
    * = res;
    * = root;
}
```
