# CP 标准库：通用稠密矩阵与数值线性代数引擎 (`cap:linalg` / `cap.linalg`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ml`，导入请使用 `import { ... } from "cap:ml"`。

## 一、 架构总览与设计准则

`cap:linalg` 是 CP 语言官方内置的纯原生、零 C 依赖的 **通用任意维度（$M \times N$）稠密矩阵与数值线性代数引擎**，面向科学计算、AI 数值优化、图形变换与控制工程场景设计。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:ml"` 与 `import cap from "cap"; cap.linalg.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/linalg/matrix.cp` | `// Source: cp/lib/linalg/matrix.cp (...)` | `LinalgError`、`Matrix` 稠密矩阵、列主元 LU 求解/求逆/行列式、Householder QR 最小二乘及向量范数 |
| `lib/linalg/index.cp` | `// Source: cp/lib/linalg/index.cp (...)` | `cap:linalg` 与 `cap.linalg` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 (`LinalgError`)

```typescript
pub err LinalgError {
    DimensionMismatch,
    NotSquare,
    SingularMatrix,
    InvalidDimensions,
    OutOfMemory,
}
```

### 2. 稠密矩阵 (`Matrix`)

`Matrix` 采用行主序连续 `[]f64` 扁平存储（元素 `(r, c)` 位于 `data[r * cols + c]`），显式管理堆内存并在离开作用域时通过 `deinit()` 释放。

- **构造与生命周期**：
  - `Matrix.zeros(alloc, rows, cols): Matrix ! LinalgError` — 分配 `rows x cols` 零矩阵（维度为 0 时返回 `InvalidDimensions`）。
  - `Matrix.identity(alloc, n): Matrix ! LinalgError` — 分配 `n x n` 单位矩阵。
  - `Matrix.fromSlice(alloc, rows, cols, slice: []const f64): Matrix ! LinalgError` — 从行主序切片深拷贝构造矩阵（长度不匹配返回 `DimensionMismatch`）。
  - `clone(alloc): Matrix ! LinalgError` — 深拷贝矩阵。
  - `deinit(*_)` — 释放底层 `data` 内存。
- **元素访问与矩阵度量**：
  - `get(r, c): f64` / `set(*_, r, c, val)` — 读写 `(r, c)` 处元素。
  - `trace(): f64 ! LinalgError` — 主对角线迹（非方阵返回 `NotSquare`）。
  - `frobeniusNorm(): f64` — Frobenius 范数 $\|A\|_F = \sqrt{\sum a_{ij}^2}$。
  - `maxAbsDiff(other): f64 ! LinalgError` — 两矩阵最大逐元素绝对差 $\max |A_{ij} - B_{ij}|$。
- **矩阵代数运算**：
  - `add(alloc, other): Matrix ! LinalgError` — 矩阵加法 $A + B$。
  - `sub(alloc, other): Matrix ! LinalgError` — 矩阵减法 $A - B$。
  - `scale(alloc, factor): Matrix ! LinalgError` — 数乘 $\alpha A$。
  - `transpose(alloc): Matrix ! LinalgError` — 矩阵转置 $A^T$。
  - `mul(alloc, other): Matrix ! LinalgError` — 矩阵乘法 $A \cdot B$（`i-k-j` 缓存友好循环序；维度不匹配返回 `DimensionMismatch`）。
  - `mulVec(alloc, x: []const f64): []f64 ! LinalgError` — 矩阵-向量乘积 $Ax$。
- **列主元 LU 分解与线性系统求解**：
  - `determinant(alloc): f64 ! LinalgError` — 基于列主元 LU 分解 $O(n^3)$ 精确计算行列式 $\det(A)$（奇异矩阵返回 `0.0`，非方阵返回 `NotSquare`）。
  - `solve(alloc, b: []const f64): []f64 ! LinalgError` — 列主元部分选主元（Partial Pivoting）LU 消元求解 $Ax = b$（自动处理主元为 0 的行交换；奇异矩阵返回 `SingularMatrix`）。
  - `inverse(alloc): Matrix ! LinalgError` — 基于单次列主元 LU 分解计算逆矩阵 $A^{-1}$（奇异矩阵返回 `SingularMatrix`）。
- **最小二乘与向量算子**：
  - `leastSquares(alloc, A, b): []f64 ! LinalgError`（亦提供 `Matrix.leastSquares`）— 基于数值稳定的 **Householder QR 分解** 求解超定线性方程组 $\min \|Ax - b\|_2$（要求 `A.rows >= A.cols` 且满列秩）。
  - `dot(a, b): f64 ! LinalgError`（亦提供 `Matrix.dot`）— 向量点积。
  - `vecNorm1(v): f64` / `vecNorm2(v): f64` / `vecNormInf(v): f64` — 向量 $L_1$、$L_2$、$L_\infty$ 范数。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { Matrix, LinalgError, leastSquares, vecNormInf } from "cap:ml";

pub fn demoLinalg(): void ! LinalgError {
    const alloc = std.heap.page_allocator;

    // 1. 求解含 0 主元的线性方程组 Ax = b（自动列主元行交换）
    let a = Matrix.fromSlice(alloc, 3, 3, &[
        0.0, 2.0, 1.0,
        1.0, -2.0, -3.0,
        -1.0, 1.0, 2.0,
    ])?;
    defer a.deinit();

    const b = [1.0, -2.0, 1.0];
    const x = a.solve(alloc, &b)?;
    defer alloc.free(x);

    // 2. 求逆验证 A * A^-1 ≈ I
    let inv = a.inverse(alloc)?;
    defer inv.deinit();
    let prod = a.mul(alloc, inv)?;
    defer prod.deinit();

    // 3. 超定方程组最小二乘拟合 y = 2x + 3
    let design = Matrix.fromSlice(alloc, 4, 2, &[
        1.0, 1.0,
        2.0, 1.0,
        3.0, 1.0,
        4.0, 1.0,
    ])?;
    defer design.deinit();
    const ys = [5.0, 7.0, 9.0, 11.0];
    const beta = leastSquares(alloc, design, &ys)?;
    defer alloc.free(beta);

    // 4. 动态懒加载挂载等价用法
    const norm = cap.linalg.vecNormInf(x);
    * = norm;
}
```
