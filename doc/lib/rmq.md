# `cap:rmq` — O(1) Sparse Table 区间最值/GCD、欧拉序 LCA 与滑动窗口零分配库

零堆分配（Zero-Alloc）的纯 CP 静态区间查询与滑动窗口算法库，提供 O(n log n) 预处理 + 严格 O(1) 查询的 Sparse Table（区间最小值 `RmqMin`、区间最大值 `RmqMax`、首次最小下标 `RmqArgMin`、区间最大公约数 `RangeGcd`）、基于欧拉序的 O(1) 树上最近公共祖先 `EulerLca`，以及 O(n) 单调队列一维滑动窗口最值（`slidingWindowMin` / `slidingWindowMax`）。

## 核心特性

- **零堆分配（Zero-Alloc）**：全部 Sparse Table 槽位与单调队列工作区均由调用方扁平切片提供，可通过 `sparseTableCells(n)` 精确计算所需容量。
- **严格 O(1) 静态区间查询**：利用 `min`、`max`、`gcd` 的幂等性，任意闭区间 `[l, r]` 仅需两次表查与一次合并。
- **平局确定性（Tie-Breaking）**：`RmqArgMin.queryIndex` 保证返回 `[l, r]` 内最小值**首次出现（最小）的下标**。
- **O(1) 树上 LCA**：`EulerLca` 复用 `RmqArgMin` 对欧拉序深度数组建表，单次 `query(u, v)` 严格 O(1)。
- **O(n) 滑动窗口**：`slidingWindowMin` / `slidingWindowMax` 基于单调双端队列，线性时间输出全部 `n - k + 1` 个窗口最值。
- **完备边界校验**：自动拦截空数组、`l > r`、`r >= n`、`k == 0`、`k > n` 及工作区缓冲不足，安全返回 `null`。

## 导出 API

### 辅助函数

- `floorLog2(n: usize): usize` — 返回 $\lfloor \log_2(n) \rfloor$（`n <= 1` 时返回 `0`）
- `sparseTableCells(n: usize): usize` — 返回构建 `n` 元素稀疏表所需的总槽位数 `n * (floorLog2(n) + 1)`
- `gcdU64(a: u64, b: u64): u64` — 迭代欧几里得最大公约数

### O(1) Sparse Table 与欧拉序 LCA

- `RmqMin`
  - `RmqMin.build(values: []const i64, table_buf: []i64): RmqMin?`
  - `query(l: usize, r: usize): i64?` — 闭区间 `[l, r]` 最小值
- `RmqMax`
  - `RmqMax.build(values: []const i64, table_buf: []i64): RmqMax?`
  - `query(l: usize, r: usize): i64?` — 闭区间 `[l, r]` 最大值
- `RmqArgMin`
  - `RmqArgMin.build(values: []const i64, idx_buf: []usize): RmqArgMin?`
  - `queryIndex(values: []const i64, l: usize, r: usize): usize?` — 闭区间 `[l, r]` 最小值首次出现下标
- `RangeGcd`
  - `RangeGcd.build(values: []const u64, table_buf: []u64): RangeGcd?`
  - `query(l: usize, r: usize): u64?` — 闭区间 `[l, r]` 最大公约数
- `EulerLca`
  - `EulerLca.build(num_nodes: usize, euler_nodes: []const usize, euler_depth: []const i64, first_occ: []const usize, idx_buf: []usize): EulerLca?`
  - `query(u: usize, v: usize): usize?` — 树上节点 `u` 与 `v` 的最近公共祖先

### O(n) 一维滑动窗口

- `slidingWindowMin(values: []const i64, k: usize, out: []i64, deque_scratch: []usize): usize?`
- `slidingWindowMax(values: []const i64, k: usize, out: []i64, deque_scratch: []usize): usize?`

## 使用示例

```cp
import std from "std";
import {
    sparseTableCells,
    RmqMin,
    RmqArgMin,
    RangeGcd,
    slidingWindowMin,
} from "cap:rmq";

fn example() {
    const vals: [8]i64 = [5, -2, 7, -2, 9, 1, -8, 3];
    let min_buf: [32]i64 = std.mem.zeroes([32]i64); // sparseTableCells(8) == 32
    let idx_buf: [32]usize = std.mem.zeroes([32]usize);

    if (rmq := RmqMin.build(vals[0..], min_buf[0..])) {
        * = rmq.query(1, 5); // -2
    }
    if (arg := RmqArgMin.build(vals[0..], idx_buf[0..])) {
        * = arg.queryIndex(vals[0..], 1, 5); // 1 (first occurrence of -2)
    }

    const nums: [4]u64 = [24, 36, 18, 12];
    let gcd_buf: [12]u64 = std.mem.zeroes([12]u64);
    if (rg := RangeGcd.build(nums[0..], gcd_buf[0..])) {
        * = rg.query(0, 3); // 6
    }

    let out: [6]i64 = std.mem.zeroes([6]i64);
    let dq: [8]usize = std.mem.zeroes([8]usize);
    * = slidingWindowMin(vals[0..], 3, out[0..], dq[0..]); // returns 6
}
```
