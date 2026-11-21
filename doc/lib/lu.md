# `cap:lu` — 零分配 Doolittle 部分选主元 LU 分解与三对角 Thomas 求解库

零堆分配（Zero-Alloc）的纯 CP 数值线性代数库，提供带部分选主元（Partial Pivoting）的原地 Doolittle LU 分解（$PA = LU$）、无损复用的前代/回代线性方程组求解、行列式计算、显式矩阵求逆、无穷范数残差检验，以及 O(n) 三对角方程组 Thomas 求解器。

## 核心特性

- **零堆分配（Zero-Alloc）**：全部矩阵分解、置换向量、求逆与 Thomas 消元工作区均由调用方扁平切片提供。
- **数值稳定部分选主元**：`luFactorize` 按列选取幅值最大主元并交换整行，记录置换奇偶性 `sign`、交换次数 `swaps` 及最小主元幅值 `min_pivot`，精准拦截奇异与病态矩阵。
- **一次分解多次求解**：`luSolve` 只读访问 `(lu_mat, piv)`，支持以 O(n²) 复杂度反复求解多个右端项。
- **O(n) 三对角求解器**：`solveTridiagonal` 实现经典 Thomas 追赶法，以线性时间与线性工作区求解大型三对角方程组（如样条插值、1D 偏微分方程隐式离散）。
- **完备边界防护**：全面拦截 `n == 0`、缓冲区不足、负容差、非有限数（`NaN`/`Inf`）及零主元。

## 导出 API

### 数据模型

- `LuInfo { sign: f64, swaps: usize, min_pivot: f64 }`

### 核心函数

- `luFactorize(lu_mat: []f64, n: usize, piv_out: []usize, tol: f64): LuInfo?`
  - 原地分解 `n x n` 行主序矩阵 `lu_mat` 为 `PA = LU`；奇异/非法时返回 `null`。
- `luSolve(lu_mat: []const f64, piv: []const usize, n: usize, b: []const f64, x_out: []f64): bool`
  - 前代解 $Ly = Pb$ + 回代解 $Ux = y$。
- `luDeterminant(lu_mat: []const f64, info: LuInfo, n: usize): f64`
  - 计算 $\det(A) = \text{info.sign} \cdot \prod_i U_{i,i}$。
- `luInverse(lu_mat: []const f64, piv: []const usize, n: usize, inv_out: []f64, rhs_scratch: []f64, col_scratch: []f64): bool`
  - 求解 `n` 组单位基右端项构造 `A^-1`。
- `matVecResidualInf(a: []const f64, x: []const f64, b: []const f64, n: usize): f64`
  - 计算无穷范数残差 $\|Ax - b\|_\infty$。
- `solveTridiagonal(lower: []const f64, diag: []const f64, upper: []const f64, rhs: []const f64, x_out: []f64, c_scratch: []f64, d_scratch: []f64): bool`
  - O(n) Thomas 算法求解三对角方程组（`diag` 长 `n`，`lower`/`upper` 长 `n - 1`）。

## 使用示例

```cp
import std from "std";
import {
    luFactorize,
    luSolve,
    luDeterminant,
    luInverse,
    matVecResidualInf,
    solveTridiagonal,
} from "cap:lu";

fn example() {
    const a: [9]f64 = [
        0.0, 2.0, 1.0,
        1.0, -2.0, -3.0,
        -1.0, 1.0, 2.0,
    ];
    let lu: [9]f64 = a;
    let piv: [3]usize = std.mem.zeroes([3]usize);

    if (info := luFactorize(lu[0..], 3, piv[0..], 1.0e-12)) {
        const b: [3]f64 = [4.0, -6.0, 3.0];
        let x: [3]f64 = std.mem.zeroes([3]f64);
        * = luSolve(lu[0..], piv[0..], 3, b[0..], x[0..]);
        * = matVecResidualInf(a[0..], x[0..], b[0..], 3); // < 1e-12
        * = luDeterminant(lu[0..], info, 3);

        let inv: [9]f64 = std.mem.zeroes([9]f64);
        let r_buf: [3]f64 = std.mem.zeroes([3]f64);
        let c_buf: [3]f64 = std.mem.zeroes([3]f64);
        * = luInverse(lu[0..], piv[0..], 3, inv[0..], r_buf[0..], c_buf[0..]);
    }

    const lower: [2]f64 = [-1.0, -1.0];
    const diag: [3]f64 = [4.0, 4.0, 4.0];
    const upper: [2]f64 = [-1.0, -1.0];
    const rhs: [3]f64 = [3.0, 2.0, 3.0];
    let tx: [3]f64 = std.mem.zeroes([3]f64);
    let cs: [3]f64 = std.mem.zeroes([3]f64);
    let ds: [3]f64 = std.mem.zeroes([3]f64);
    * = solveTridiagonal(lower[0..], diag[0..], upper[0..], rhs[0..], tx[0..], cs[0..], ds[0..]);
}
```
