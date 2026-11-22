# `cap:quadrature` — 纯 CP 零堆分配高精度数值积分库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:quadrature` 提供纯 CP、零堆分配的一维高精度数值求积工具集，包含 5/10 点 Gauss-Legendre、复合 Simpson 1/3、QUADPACK G7-K15 高斯-克朗罗德规则、无递归显式栈自适应细分积分以及 Romberg（Richardson）外推积分。

## 特性概览

- **零堆分配 & 无递归**：全静态 20 位精度节点权重表，自适应细分使用调用方提供的显式区间栈，Romberg 外推使用两行工作缓冲。
- **开型 G7-K15 端点奇异性友好**：`gaussKronrod15` 与 `adaptiveGk15` 不在区间端点采样，可直接高精度积分端点弱奇异函数（如 $\int_0^1 x^{-1/2}\,dx$）与局部窄尖峰函数。
- **方向感知与完备防御**：所有方法自然支持 `a > b` 反向变号与 `a == b` 零积分，并拦截 `NaN`/`Inf`、非法容差、奇/零子区间及缓冲不足。

## API 参考

### 类型定义

```cp
pub type IntegrandFn = fn(x: f64): f64;

pub struct Gk15Estimate {
    value: f64,
    abs_error: f64,
}

pub struct QuadResult {
    value: f64,
    abs_error: f64,
    evals: usize,
    converged: bool,
}
```

### 求积函数

- `gaussLegendre5(f: IntegrandFn, a: f64, b: f64): f64`（最高 9 次多项式精确）
- `gaussLegendre10(f: IntegrandFn, a: f64, b: f64): f64`（最高 19 次多项式精确）
- `compositeSimpson(f: IntegrandFn, a: f64, b: f64, panels: usize): f64?`（`panels` 须为正偶数）
- `gaussKronrod15(f: IntegrandFn, a: f64, b: f64): Gk15Estimate`
- `adaptiveGk15(f: IntegrandFn, a: f64, b: f64, abs_tol: f64, rel_tol: f64, max_subdivisions: usize, stack_a: *[]f64, stack_b: *[]f64): QuadResult?`
- `romberg(f: IntegrandFn, a: f64, b: f64, tol: f64, max_order: usize, prev_row: *[]f64, curr_row: *[]f64): QuadResult?`

## 使用示例

```cp
import std from "std";
import { adaptiveGk15, gaussLegendre10 } from "cap:math";

fn invSqrt(x: f64): f64 {
    return 1.0 / std.math.sqrt(x);
}

fn piIntegrand(x: f64): f64 {
    return 4.0 / (1.0 + x * x);
}

const pi_approx = gaussLegendre10(piIntegrand, 0.0, 1.0);

let sa_buf: [128]f64 = std.mem.zeroes([128]f64);
let sb_buf: [128]f64 = std.mem.zeroes([128]f64);
let sa: []f64 = sa_buf[0..];
let sb: []f64 = sb_buf[0..];
const sing_res = adaptiveGk15(invSqrt, 0.0, 1.0, 1.0e-8, 1.0e-8, 256, *sa, *sb);
* = pi_approx;
* = sing_res;
```
