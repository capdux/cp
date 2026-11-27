[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# smawk

纯 CP 实现的零堆分配 SMAWK $O(m + n)$ 完全单调/Monge 矩阵行最小值与列最小值算法库，内置 Monge/完全单调性检验、`reduce`/`interpolate` 原语及在线列最小值 DP 求解器。

- **包名**：`smawk`（`v0.3.2`）
- **上游对齐**：洁净室移植自 `crates.io/smawk`（`v0.3.2`，`MIT`，`mgeisler/smawk`），适用于 Knuth-Plass 断行排版、最优文本布局与一维动态规划加速。

## 安装与导入

```bash
cap add smawk
```

```typescript
import {
    rowMinima, colMinima,
    isMonge, isTotallyMonotone,
    reduce, interpolate,
    mongeRowMinima, smawkRowMinima, smawkColMinima,
    onlineColumnMinima,
} from "smawk";
```

## 核心 API

### 高层矩阵极值函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `smawkRowMinima` / `mongeRowMinima` | `(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize` | 面向完全单调/Monge 矩阵的 $O(m + n)$ SMAWK 行最小值，向 `out_min_cols` 写入每行最左最小值列索引，返回写入行数。 |
| `smawkColMinima` | `(matrix: &[]f64, rows: usize, cols: usize, out_min_rows: []usize): usize` | $O(m + n)$ SMAWK 列最小值，向 `out_min_rows` 写入每列最小值行索引。 |
| `rowMinima` | `(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize` | 通用 $O(mn)$ 精确行最小值，不要求矩阵具备单调性。 |
| `colMinima` | `(matrix: &[]f64, rows: usize, cols: usize, out_min_rows: []usize): usize` | 通用 $O(mn)$ 精确列最小值，不要求矩阵具备单调性。 |
| `onlineColumnMinima` | `(matrix: &[]f64, n: usize, out_min_rows: []usize): usize` | 严格上三角在线 SMAWK（仅访问 `i < j` 的元素，绝不读取 `i >= j`），适用于 1D DP 递推。 |

### 性质检验与底层 SMAWK 原语

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `isMonge` | `(matrix: &[]f64, rows: usize, cols: usize): bool` | 检验 Monge 性质：$M[i,j] + M[i+1,j+1] \le M[i,j+1] + M[i+1,j]$。 |
| `isTotallyMonotone` | `(matrix: &[]f64, rows: usize, cols: usize): bool` | 检验所有 $2 \times 2$ 子矩阵的完全单调性。 |
| `reduce` | `(matrix: &[]f64, cols: usize, rows_idx: &[]usize, cols_idx: &[]usize, out_cols: []usize): usize` | SMAWK REDUCE 步骤：将候选列裁剪至不超过 `rows_idx.len` 列，写入 `out_cols`。 |
| `interpolate` | `(matrix: &[]f64, cols: usize, rows_idx: &[]usize, cols_idx: &[]usize, minima: []usize)` | SMAWK INTERPOLATE 步骤：在已求出的偶数行极值之间插值填充奇数行极值。 |

## 使用示例

### 1. Monge 矩阵的 SMAWK 行最小值

```typescript
import { isMonge, smawkRowMinima } from "smawk";

fn main() {
    // 3x4 行主序 Monge 矩阵
    let mat: [12]f64 = [
        10.0, 17.0, 13.0, 28.0,
        17.0, 22.0, 16.0, 29.0,
        24.0, 28.0, 22.0, 34.0,
    ];
    let ok = isMonge(mat[0..], 3, 4); // true
    let mins: [3]usize = undefined;
    let count = smawkRowMinima(mat[0..], 3, 4, mins[0..]);
    // count == 3, mins == [0, 2, 2]
}
```

### 2. 完全单调性检查与列最小值

```typescript
import { isTotallyMonotone, smawkColMinima } from "smawk";

fn main() {
    let mat: [9]f64 = [
        1.0, 2.0, 4.0,
        2.0, 1.0, 2.0,
        4.0, 2.0, 1.0,
    ];
    if (isTotallyMonotone(mat[0..], 3, 3)) {
        let col_mins: [3]usize = undefined;
        let n = smawkColMinima(mat[0..], 3, 3, col_mins[0..]); // [0, 1, 2]
    }
}
```

### 3. 在线严格上三角列最小值（1D DP）

```typescript
import { onlineColumnMinima } from "smawk";

fn main() {
    // 4x4 严格上三角代价矩阵（绝不读取 i >= j 的位置）
    let cost: [16]f64 = [
        0.0, 5.0, 9.0, 14.0,
        0.0, 0.0, 3.0,  7.0,
        0.0, 0.0, 0.0,  2.0,
        0.0, 0.0, 0.0,  0.0,
    ];
    let best_row: [4]usize = undefined;
    let n = onlineColumnMinima(cost[0..], 4, best_row[0..]);
}
```

## 与源包差异及零拷贝安全亮点

- **行主序扁平零堆布局**：直接接受连续 `&[]f64` 切片（`matrix[r * cols + c]`）并将结果写入调用方 `[]usize` 缓冲区，消除 `Vec<Vec<T>>` 堆分配与 Trait 虚调用开销。
- **开放底层原语**：同时导出高层 `smawkRowMinima` / `smawkColMinima` / `onlineColumnMinima` 与底层 `reduce` / `interpolate` 供自定义 DP 组合。
- **防御式边界检查**：当维度为 `0`、`matrix.len < rows * cols` 或输出切片不足时安全返回 `0` 而不越界。
