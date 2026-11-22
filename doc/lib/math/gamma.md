# `cap:gamma` — 纯 CP 零堆分配特殊数学函数库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:gamma` 提供纯 CP、零堆分配、双精度（目标相对误差 `< 1e-13`）特殊数学函数库，涵盖 Gamma / 对数 Gamma / Digamma、正则化不完全 Gamma 与 Beta 函数、无灾难抵消误差函数 `erf`/`erfc` 以及对数阶乘与组合数。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部算法采用纯栈标量与静态常数表实现。
- **高精度双精度逼近**：
  - `lgamma` / `tgamma`：9 项 Lanczos (`g = 7`) + 负半轴 Euler 反射公式 + 小整数精确阶乘；
  - `digamma`：负半轴反射 + 递推平移至 `x >= 10` + 7 项 Bernoulli 渐近展开；
  - `gammaP` / `gammaQ`：按 `x < a + 1` 自动切换幂级数与修正 Lentz 连分式；
  - `betainc`：按 `x < (a + 1)/(a + b + 2)` 对称变换 + 修正 Lentz 连分式；
  - `erf` / `erfc`：基于不完全 Gamma 级数与连分式双分支实现，大 `x` 尾部无灾难抵消。

## API 参考

- **Gamma 与 Digamma**：
  - `lgamma(x: f64): f64`（$\ln|\Gamma(x)|$；非正整数极点返回 `+inf`）
  - `tgamma(x: f64): f64`（$\Gamma(x)$；非正整数极点返回 `NaN`）
  - `digamma(x: f64): f64`（$\psi(x) = \Gamma'(x)/\Gamma(x)$；非正整数极点返回 `NaN`）
- **正则化不完全 Gamma**：
  - `gammaP(a: f64, x: f64): f64?`（$P(a, x) = \gamma(a, x)/\Gamma(a)$，`a > 0, x >= 0`）
  - `gammaQ(a: f64, x: f64): f64?`（$Q(a, x) = 1 - P(a, x)$，`a > 0, x >= 0`）
- **Beta 与正则化不完全 Beta**：
  - `lbeta(a: f64, b: f64): f64?`、`beta(a: f64, b: f64): f64?`
  - `betainc(x: f64, a: f64, b: f64): f64?`（$I_x(a, b)$，`0 <= x <= 1, a > 0, b > 0`）
- **误差函数与组合数**：
  - `erf(x: f64): f64`、`erfc(x: f64): f64`
  - `logFactorial(n: u64): f64`、`logBinomial(n: u64, k: u64): f64?`

## 使用示例

```cp
import { tgamma, digamma, gammaP, betainc, erfc, logBinomial } from "cap:math";

const g_half = tgamma(0.5);             // sqrt(pi)
const psi_1 = digamma(1.0);             // -0.5772156649015329 (-gamma_E)
const chi2_cdf = gammaP(2.0, 3.0);      // Chi-square(df=4) CDF at x=6
const beta_cdf = betainc(0.5, 3.0, 3.0);// 0.5
const tail = erfc(5.0);                 // ~1.537459794428035e-12
const log_c = logBinomial(10, 3);       // ln(120)
* = g_half;
* = psi_1;
* = chi2_cdf;
* = beta_cdf;
* = tail;
* = log_c;
```
