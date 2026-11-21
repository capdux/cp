# `cap:newton` — 纯 CP 零分配非线性方程组与 Levenberg-Marquardt 最小二乘库

100% 纯 CP 零堆分配（Zero-Alloc）非线性数值求解库，提供：
1. **阻尼多维 Newton-Raphson**（`F(x) = 0`，中心差分雅可比 + 部分选主元 LU 分解 + Armijo 回溯线搜索防发散）；
2. **Levenberg-Marquardt 非线性最小二乘**（$\min \frac{1}{2}\|r(x)\|_2^2,\; m \ge n$，Marquardt 缩放自适应阻尼）；
3. **一维求根器**（`newton1D` 解析导数牛顿法与 `secant1D` 无导数割线法）。

## 导入方式

```cp
import {
    ResidualFn,
    ScalarFn,
    NewtonConfig,
    NewtonResult,
    NewtonWorkspace,
    LmConfig,
    LmResult,
    LmWorkspace,
    RootResult,
    newtonSystem,
    levenbergMarquardt,
    newton1D,
    secant1D,
} from "cap:newton";
```

## API 契约

### 1. 回调类型

- `pub type ResidualFn = fn(x: []const f64, f_out: *[]f64);`
- `pub type ScalarFn = fn(x: f64): f64;`

### 2. 阻尼多维 Newton-Raphson（`F(x) = 0`）

- `pub struct NewtonConfig`：`tol_f`, `tol_x`, `fd_eps`, `max_iter`, `max_backtrack`；默认工厂 `NewtonConfig.default()`
- `pub struct NewtonResult`：`converged`, `residual_norm`, `iterations`, `evals`
- `pub struct NewtonWorkspace`：`fx`, `fx_trial`, `jac` (`>= n*n`), `dx`, `x_trial`, `piv` (`>= n`)
- `pub fn newtonSystem(f: ResidualFn, x0: []const f64, x_out: *[]f64, cfg: NewtonConfig, ws: *NewtonWorkspace): NewtonResult?`

### 3. Levenberg-Marquardt 非线性最小二乘（`min 0.5 * ||r(x)||_2^2`）

- `pub struct LmConfig`：`tol_r`, `tol_x`, `fd_eps`, `lambda_init`, `max_iter`；默认工厂 `LmConfig.default()`
- `pub struct LmResult`：`converged`, `cost`, `residual_norm`, `iterations`, `evals`
- `pub struct LmWorkspace`：`r` (`>= m`), `r_trial` (`>= m`), `jac` (`>= m*n`), `jtj` (`>= n*n`), `jtr` (`>= n`), `dx` (`>= n`), `x_trial` (`>= n`), `piv` (`>= n`)
- `pub fn levenbergMarquardt(r_fn: ResidualFn, m: usize, x0: []const f64, x_out: *[]f64, cfg: LmConfig, ws: *LmWorkspace): LmResult?`

### 4. 一维求根辅助

- `pub struct RootResult`：`root`, `fval`, `iterations`, `converged`
- `pub fn newton1D(f: ScalarFn, df: ScalarFn, x0: f64, tol: f64, max_iter: usize): RootResult?`
- `pub fn secant1D(f: ScalarFn, x0: f64, x1: f64, tol: f64, max_iter: usize): RootResult?`

## 示例

```cp
import { NewtonConfig, NewtonWorkspace, newtonSystem } from "cap:newton";

fn circleHyperbola(x: []const f64, f_out: *[]f64) {
    f_out.*[0] = x[0] * x[0] + x[1] * x[1] - 4.0;
    f_out.*[1] = x[0] * x[1] - 1.0;
}

fn demo() {
    const x0: [2]f64 = [1.8, 0.3];
    let x_buf: [2]f64 = [0.0; 2];
    let x_out: []f64 = x_buf[0..];
    let fx: [2]f64 = [0.0; 2];
    let fxt: [2]f64 = [0.0; 2];
    let jac: [4]f64 = [0.0; 4];
    let dx: [2]f64 = [0.0; 2];
    let xt: [2]f64 = [0.0; 2];
    let piv: [2]usize = [0; 2];
    let ws: NewtonWorkspace = {
        fx: fx[0..], fx_trial: fxt[0..], jac: jac[0..],
        dx: dx[0..], x_trial: xt[0..], piv: piv[0..],
    };
    if (res := newtonSystem(circleHyperbola, x0[0..], *x_out, NewtonConfig.default(), *ws)) {
        * = res;
    }
}
```
