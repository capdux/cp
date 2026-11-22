# `cap:simplex` — 纯 CP 零堆分配两阶段线性规划单纯形求解器库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:math`，导入请使用 `import { ... } from "cap:math"`。

`cap:simplex` 提供纯 CP、零堆分配的两阶段单纯形法（Two-Phase Simplex）线性规划求解器，内置 **Bland 防退化循环规则**，支持任意混合 `<= / >= / =` 约束、负右端项自动 Phase-I 初始化、最大化/最小化目标以及原问题解 `xOut` 与对偶影子价格 `dualOut` 提取。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部单纯形表（tableau）、基变量索引及输出切片均由调用方提供栈/切片内存。
- **两阶段法 + Bland 防循环**：自动对负右端项与 `>=`/`=` 约束引入人工变量执行 Phase-I 寻找初始基本可行解；采用最小下标入基与最小基变量号破平局的 Bland 规则，保证退化顶点绝不循环。
- **原对偶双解输出**：在 `Optimal` 终止时自动从初始单位列提取各约束的精确对偶影子价格 `dualOut`，严格满足线性规划强对偶定理 $c^T x^* = b^T y^*$。
- **完备边界防御**：自动拦截零维度、缓冲不足、非有限输入（`NaN`/`Inf`）及超迭代上限。

## API 参考

### 枚举与数据结构

```cp
pub enum ConstraintKind { Le, Ge, Eq }

pub enum SimplexStatus {
    Optimal,
    Unbounded,
    Infeasible,
    MaxIterations,
    InvalidInput,
}

pub struct SimplexResult {
    status: SimplexStatus,
    objective: f64,
    iterations: u32,
}

pub struct SimplexWorkspace {
    tableau: []f64, // 安全容量: simplexRequiredTableauLen(num_vars, num_constraints)
    basis: []usize, // len >= num_constraints
}
```

### 辅助与核心求解函数

- `simplexRequiredTableauLen(num_vars: usize, num_constraints: usize): usize`
  返回最坏情况单纯形表所需长度 `(m + 1) * (n + 2 * m + 1)`。
- `solveGeneral(c, a, b, kinds, num_vars, num_constraints, is_max, max_iter, x_out: *[]f64, dual_out: *[]f64, ws: *SimplexWorkspace): SimplexResult`
  求解一般混合 `Le`/`Ge`/`Eq` 约束线性规划（`x >= 0`）。若无需对偶解，`dual_out` 可传空切片。
- `solveMaxLe(c, a, b, num_vars, num_constraints, max_iter, x_out: *[]f64, dual_out: *[]f64, ws: *SimplexWorkspace): SimplexResult`
  便捷求解 $\max c^T x \text{ s.t. } Ax \le b, x \ge 0$（支持负 $b_i$ 自动触发 Phase-I）。
- `solveMinLe(c, a, b, num_vars, num_constraints, max_iter, x_out: *[]f64, dual_out: *[]f64, ws: *SimplexWorkspace): SimplexResult`
  便捷求解 $\min c^T x \text{ s.t. } Ax \le b, x \ge 0$（支持负 $b_i$ 自动触发 Phase-I）。

## 使用示例

```cp
import std from "std";
import { SimplexWorkspace, SimplexStatus, solveMaxLe } from "cap:math";

// max 3*x1 + 5*x2 s.t. x1 <= 4, 2*x2 <= 12, 3*x1 + 2*x2 <= 18, x >= 0
const c: [2]f64 = [3.0, 5.0];
const a: [6]f64 = [
    1.0, 0.0,
    0.0, 2.0,
    3.0, 2.0,
];
const b: [3]f64 = [4.0, 12.0, 18.0];

let tab_buf: [36]f64 = std.mem.zeroes([36]f64); // (3+1) * (2+2*3+1) = 36
let basis_buf: [3]usize = std.mem.zeroes([3]usize);
let ws = SimplexWorkspace { tableau: tab_buf[0..], basis: basis_buf[0..] };

let x_buf: [2]f64 = std.mem.zeroes([2]f64);
let y_buf: [3]f64 = std.mem.zeroes([3]f64);
let x: []f64 = x_buf[0..];
let y: []f64 = y_buf[0..];

const res = solveMaxLe(c[0..], a[0..], b[0..], 2, 3, 100, *x, *y, *ws);
// res.status == SimplexStatus.Optimal, res.objective == 36.0, x == [2.0, 6.0]
* = res;
```
