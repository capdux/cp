# `cap:bessel` — 纯 CP 零堆分配柱贝塞尔与修正贝塞尔特殊函数库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:bessel` 提供纯 CP、零堆分配、高精度（双精度目标误差 `< 1e-12`）的第一类/第二类柱贝塞尔函数与第一类/第二类修正贝塞尔函数（含大自变量指数缩放版本）。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部采用纯栈标量、Double-Double（106 位尾数）无损幂级数求和、Hankel 渐近展开与 Miller 向下递推。
- **全实数域高精度**：
  - `j0`/`j1`/`y0`/`y1` 在 `|x| <= 18` 使用 Double-Double 幂级数，`x > 18` 切换至 Hankel 渐近展开，接缝平滑精度 `< 1e-13`。
  - `jn` 在 `0 < |x| <= |n|` 自动切换至 Miller 向下递推（配合 $\sum \epsilon_k J_{2k}(x) = 1$ 归一化），大阶小自变量无数值发散。
  - `i0e`/`i1e` 与 `k0e`/`k1e` 提供 $e^{-|x|} I_\nu(x)$ 与 $e^x K_\nu(x)$ 指数缩放版本，在 `x = 100` 乃至 `x = 1000` 下不溢出不下溢。

## API 参考

- **第一类柱贝塞尔**：
  - `j0(x: f64): f64`、`j1(x: f64): f64`、`jn(n: i32, x: f64): f64`
- **第二类柱贝塞尔（Neumann，`x > 0`）**：
  - `y0(x: f64): f64`、`y1(x: f64): f64`、`yn(n: i32, x: f64): f64`
- **第一类修正贝塞尔（双曲贝塞尔）**：
  - `besselI0(x: f64): f64`、`besselI1(x: f64): f64`（为避开 Zig 原生任意位宽整型 `i0`/`i1` 命名冲突采用 `besselI0`/`besselI1`）
  - `i0e(x: f64): f64`、`i1e(x: f64): f64`（$e^{-|x|} I_0(x)$、$e^{-|x|} I_1(x)$）
- **第二类修正贝塞尔（Macdonald，`x > 0`）**：
  - `k0(x: f64): f64`、`k1(x: f64): f64`
  - `k0e(x: f64): f64`、`k1e(x: f64): f64`（$e^x K_0(x)$、$e^x K_1(x)$）

## 使用示例

```cp
import { j0, jn, y0, i0e, k1e } from "cap:math";

const root_check = j0(2.4048255576957727); // ~0.0
const high_ord = jn(20, 5.0);              // ~2.770330052128947e-11
const neumann = y0(1.0);                   // ~0.088256964215677
const scaled_w = i0e(100.0) * k1e(100.0);  // overflow/underflow-free
* = root_check;
* = high_ord;
* = neumann;
* = scaled_w;
```
