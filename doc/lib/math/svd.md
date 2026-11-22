# `cap:svd` — 纯 CP 零堆分配奇异值分解与 Moore-Penrose 广义逆库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:svd` 提供纯 CP、零堆分配的单边 Jacobi 奇异值分解（One-Sided Jacobi SVD），支持方阵与高矩阵（`m >= n`）的高精度经济型分解 $A = U \operatorname{diag}(S) V^T$、截断奇异值最小范数最小二乘求解以及 Moore-Penrose 伪逆 $A^+$ 计算。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部输出矩阵 `U`、`S`、`V`、`pinv_out` 及临时缓冲均由调用方栈/切片内存提供。
- **单边 Jacobi 高精度**：直接对列对做正交旋转，避开双对角化病态精度损失；自动按奇异值降序重排 `U`/`S`/`V`，并对零奇异值尾列执行二次 Modified Gram-Schmidt 正交补全，确保任意矩阵（含零矩阵与秩亏矩阵）均满足 $U^T U = I_n$ 与 $V^T V = I_n$。
- **秩亏友好求解与广义逆**：`svdSolve` 与 `svdPseudoinverse` 基于相对截断阈值 `rcond * max(S)` 滤除病态/零奇异值，输出满足四大 Penrose 公理的伪逆与最小欧氏范数解。

## API 参考

### `SvdResult`

```cp
pub struct SvdResult {
    converged: bool,
    sweeps: usize,
    rank: usize,
    conditionNumber: f64,
}
```

### 核心函数

- `svdJacobi(a: []const f64, m: usize, n: usize, u_out: *[]f64, s_out: *[]f64, v_out: *[]f64, max_sweeps: usize, tol: f64): SvdResult?`
  计算行主序 `m x n` 矩阵（`m >= n >= 1`）的经济型 SVD。`u_out.len >= m * n`，`s_out.len >= n`，`v_out.len >= n * n`。
- `svdSolve(u: []const f64, s: []const f64, v: []const f64, m: usize, n: usize, b: []const f64, x_out: *[]f64, utb_scratch: *[]f64, rcond: f64): bool`
  计算最小范数最小二乘解 $x = V S^+ U^T b$。
- `svdPseudoinverse(u: []const f64, s: []const f64, v: []const f64, m: usize, n: usize, pinv_out: *[]f64, rcond: f64): bool`
  计算行主序 `n x m` Moore-Penrose 伪逆 $A^+ = V S^+ U^T$。
- `svdReconstructionError(a: []const f64, u: []const f64, s: []const f64, v: []const f64, m: usize, n: usize): f64`
  计算无穷范数重构误差 $\|A - U \operatorname{diag}(S) V^T\|_\infty$（非法尺寸返回 `-1.0`）。

## 使用示例

```cp
import std from "std";
import { svdJacobi, svdSolve, svdReconstructionError } from "cap:math";

const a: [9]f64 = [
    3.0, 2.0, 2.0,
    2.0, 3.0, -2.0,
    1.0, 0.0, 1.0,
];
let u_buf: [9]f64 = std.mem.zeroes([9]f64);
let s_buf: [3]f64 = std.mem.zeroes([3]f64);
let v_buf: [9]f64 = std.mem.zeroes([9]f64);
let u: []f64 = u_buf[0..];
let s: []f64 = s_buf[0..];
let v: []f64 = v_buf[0..];

if (res := svdJacobi(a[0..], 3, 3, *u, *s, *v, 60, 1.0e-14)) {
    const err = svdReconstructionError(a[0..], u, s, v, 3, 3);
    const b: [3]f64 = [7.0, 3.0, 2.0];
    let x_buf: [3]f64 = std.mem.zeroes([3]f64);
    let sc_buf: [3]f64 = std.mem.zeroes([3]f64);
    let x: []f64 = x_buf[0..];
    let sc: []f64 = sc_buf[0..];
    * = svdSolve(u, s, v, 3, 3, b[0..], *x, *sc, 1.0e-12);
    * = res;
    * = err;
}
```
