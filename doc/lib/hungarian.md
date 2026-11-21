# `cap:hungarian` — 纯 CP 零堆分配线性指派与二分图最优匹配库

`cap:hungarian` 实现经典 $O(n^3)$ Jonker-Volgenant / Kuhn-Munkres 势能对偶（`u`/`v` potential）匈牙利算法，支持 `i64` 精确整数与 `f64` 双精度浮点代价矩阵的最小权与最大权线性指派，全程零堆分配并自动零拷贝支持任意矩形矩阵（`rows <= cols` 与 `rows > cols`）。

## 特性概览

- **零堆分配（Zero-Alloc）**：全部势能数组、增广路径与输出映射均由调用方栈/切片内存提供。
- **任意矩形矩阵零拷贝支持**：当 `rows > cols` 时自动采用逻辑转置，无需额外矩阵缓冲区；未匹配行在 `rowToCol` 中标记为 `-1`，`matchedCount == min(rows, cols)`。
- **强对偶最优性保证**：求解结束后最终对偶势能保留在 `ws.u[1..=r]` 与 `ws.v[1..=c]` 中（满足 `u[i] + v[j] <= cost_eff(i, j)` 且匹配边取等号）。
- **完备输入校验**：自动拒收空矩阵、切片长度不足、工作区缓冲不足、非有限浮点（`NaN`/`Inf`）及可能引发整数溢出的极端 `i64` 代价。

## API 参考

### 结果与工作区类型

```cp
pub struct AssignmentResultF64 {
    totalCost: f64,
    matchedCount: usize,
}
pub type AssignmentResult = AssignmentResultF64;

pub struct AssignmentResultI64 {
    totalCost: i64,
    matchedCount: usize,
}

pub struct HungarianWorkspaceF64 {
    u: []f64,      // len >= min(rows, cols) + 1
    v: []f64,      // len >= max(rows, cols) + 1
    minv: []f64,   // len >= max(rows, cols) + 1
    p: []usize,    // len >= max(rows, cols) + 1
    way: []usize,  // len >= max(rows, cols) + 1
    used: []u8,    // len >= max(rows, cols) + 1
}
pub type HungarianWorkspace = HungarianWorkspaceF64;

pub struct HungarianWorkspaceI64 {
    u: []i64,      // len >= min(rows, cols) + 1
    v: []i64,      // len >= max(rows, cols) + 1
    minv: []i64,   // len >= max(rows, cols) + 1
    p: []usize,    // len >= max(rows, cols) + 1
    way: []usize,  // len >= max(rows, cols) + 1
    used: []u8,    // len >= max(rows, cols) + 1
}
```

### 核心求解函数

- `solveMinF64(cost: []const f64, rows: usize, cols: usize, rowToCol: *[]i32, ws: *HungarianWorkspaceF64): AssignmentResultF64?`
- `solveMaxF64(weight: []const f64, rows: usize, cols: usize, rowToCol: *[]i32, ws: *HungarianWorkspaceF64): AssignmentResultF64?`
- `solveMinI64(cost: []const i64, rows: usize, cols: usize, rowToCol: *[]i32, ws: *HungarianWorkspaceI64): AssignmentResultI64?`
- `solveMaxI64(weight: []const i64, rows: usize, cols: usize, rowToCol: *[]i32, ws: *HungarianWorkspaceI64): AssignmentResultI64?`

## 使用示例

```cp
import std from "std";
import { HungarianWorkspaceF64, solveMinF64 } from "cap:hungarian";

const cost: [9]f64 = [
    4.0, 1.0, 3.0,
    2.0, 0.0, 5.0,
    3.0, 2.0, 2.0,
];

let u: [4]f64 = std.mem.zeroes([4]f64);
let v: [4]f64 = std.mem.zeroes([4]f64);
let minv: [4]f64 = std.mem.zeroes([4]f64);
let p: [4]usize = std.mem.zeroes([4]usize);
let way: [4]usize = std.mem.zeroes([4]usize);
let used: [4]u8 = std.mem.zeroes([4]u8);
let ws = HungarianWorkspaceF64 {
    u: u[0..], v: v[0..], minv: minv[0..],
    p: p[0..], way: way[0..], used: used[0..],
};

let assign_buf: [3]i32 = std.mem.zeroes([3]i32);
let assign: []i32 = assign_buf[0..];

if (res := solveMinF64(cost[0..], 3, 3, *assign, *ws)) {
    // res.totalCost == 5.0, res.matchedCount == 3
    * = res;
}
```
