# `cap:qr` — 零堆分配 Householder QR 分解与线性最小二乘库

`cap:qr` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的 Householder 反射 QR 分解与线性最小二乘求解器，支持方阵与超定矩形矩阵（$m \ge n > 0$，行主序存储），采用 LAPACK `dgeqrf` 风格的缩放范数与无相消符号选择，在病态矩阵（如 Hilbert 矩阵）上仍保持机器精度级正交性。

## 核心特性

- **100% 零堆分配**：原地紧凑存储上三角 $R$ 与下三角 Householder 反射向量，工作区与输出全部由调用方以切片传入。
- **高数值稳定性**：列范数缩放防溢出/下溢，符号选择避免相消误差，在 Hilbert 病态矩阵上保持 $\|Q^T Q - I_n\|_\infty < 10^{-12}$。
- **无需显式构造 $Q$**：`qrApplyQt` 直接隐式左乘 $Q^T$ 求解最小二乘；需要显式正交基时可调用 `qrFormQ` 构造经济型 $m \times n$ 矩阵 $Q$。
- **严格防御校验**：自动检测 $m < n$、零维、缓冲不足、非有限浮点（`NaN`/`Inf`）及秩亏（$|R_{ii}| \le \text{tol}$）。

---

## 导出清单

| 符号 | 签名 / 说明 |
| :--- | :--- |
| `QR_DEFAULT_TOL` | `f64 = 1.0e-12`，默认秩亏判断阈值 |
| `qrFactorize(a_qr: *[]f64, m: usize, n: usize, tau_out: *[]f64): bool` | 原地 Householder QR 分解（行主序 `m x n`） |
| `qrApplyQt(a_qr: []const f64, tau: []const f64, m: usize, n: usize, y_inout: *[]f64): bool` | 原地隐式应用 $y \leftarrow Q^T y$ |
| `qrFormQ(a_qr: []const f64, tau: []const f64, m: usize, n: usize, q_out: *[]f64): bool` | 构造经济型 $m \times n$ 正交矩阵 $Q$ |
| `qrExtractR(a_qr: []const f64, m: usize, n: usize, r_out: *[]f64): bool` | 提取 $n \times n$ 上三角矩阵 $R$ |
| `qrSolveLeastSquares(a_qr: []const f64, tau: []const f64, m: usize, n: usize, b: []const f64, x_out: *[]f64, qty_scratch: *[]f64, tol: f64): bool` | 求解超定线性最小二乘 $\min_x \|Ax - b\|_2$，秩亏返回 `false` |
| `qrOrthogonalityError(q: []const f64, m: usize, n: usize): f64` | 计算 $\|Q^T Q - I_n\|_\infty$（非法输入返回 `-1.0`） |
| `qrReconstructionError(a: []const f64, q: []const f64, r: []const f64, m: usize, n: usize): f64` | 计算 $\|A - Q R\|_\infty$（非法输入返回 `-1.0`） |

---

## 使用示例

```typescript
import std from "std";
import {
    QR_DEFAULT_TOL,
    qrFactorize,
    qrFormQ,
    qrExtractR,
    qrSolveLeastSquares,
    qrOrthogonalityError,
    qrReconstructionError,
} from "cap:qr";

fn leastSquaresDemo() {
    // Fit y = c0 + c1 * t on 4 points
    const a_orig = [
        1.0, 0.0,
        1.0, 1.0,
        1.0, 2.0,
        1.0, 3.0,
    ];
    let a_qr = a_orig;
    let tau: [2]f64 = std.mem.zeroes([2]f64);
    let a_s: []f64 = a_qr[0..];
    let tau_s: []f64 = tau[0..];

    if (!qrFactorize(&a_s, 4, 2, &tau_s)) {
        return;
    }

    const b = [1.0, 3.0, 5.0, 7.0];
    let x: [2]f64 = std.mem.zeroes([2]f64);
    let qty: [4]f64 = std.mem.zeroes([4]f64);
    let x_s: []f64 = x[0..];
    let qty_s: []f64 = qty[0..];

    const ok = qrSolveLeastSquares(a_qr[0..], tau[0..], 4, 2, b[0..], &x_s, &qty_s, QR_DEFAULT_TOL);
    // ok == true, x ~= [1.0, 2.0]
    * = ok;
}
```
