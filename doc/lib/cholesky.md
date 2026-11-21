# `cap:cholesky` — SPD Cholesky ($L L^T$) 与无开方 $L D L^T$ 分解、求解与求逆库

纯 CP、零堆分配的数值线性代数标准库，采用行主序扁平 `[]f64` 矩阵，提供对称正定矩阵（SPD）的 Cholesky $A = L L^T$ 分解、前向/后向三角回代、线性方程组求解、防溢出对数行列式 `choleskyLogDet`、显式求逆 `choleskyInvert` 以及支持对称不定矩阵的无开方 $L D L^T$ 分解与求解。

## 导入

```cp
import {
    choleskyDecompose,
    forwardSubstitute,
    backwardSubstituteT,
    choleskySolve,
    choleskyLogDet,
    choleskyDet,
    choleskyInvert,
    ldltDecompose,
    ldltSolve,
} from "cap:cholesky";
```

## 核心 API

- **Cholesky $A = L L^T$**：
  - `choleskyDecompose(a: []const f64, n: usize, l_out: []f64): bool`
  - `forwardSubstitute(l, n, b, y_out): bool`、`backwardSubstituteT(l, n, y, x_out): bool`
  - `choleskySolve(l, n, b, x_out, scratch_y): bool`
  - `choleskyLogDet(l, n): f64?`、`choleskyDet(l, n): f64?`
  - `choleskyInvert(l, n, inv_out, scratch_col, scratch_y): bool`
- **无开方 $L D L^T$**：
  - `ldltDecompose(a: []const f64, n: usize, l_out: []f64, d_out: []f64): bool`
  - `ldltSolve(l, d, n, b, x_out, scratch_y): bool`

## 示例

```cp
import std from "std";
import { choleskyDecompose, choleskySolve } from "cap:cholesky";

fn solve3x3(a: []const f64, b: []const f64, x_out: []f64): bool {
    let l: [9]f64 = std.mem.zeroes([9]f64);
    let y: [3]f64 = std.mem.zeroes([3]f64);
    if (!choleskyDecompose(a, 3, l[0..])) {
        return false;
    }
    return choleskySolve(l[0..], 3, b, x_out, y[0..]);
}
```
