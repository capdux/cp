# CP 标准库：树状数组与惰性传播线段树区间查询引擎手册 (`cap:segtree`)

## 一、 架构总览与设计准则

`cap:segtree` 是 CP 语言官方内置的纯 CP **树状数组（`FenwickTree`）与惰性传播线段树（`LazySegmentTree`）** 区间查询引擎，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。既可通过 `import { ... } from "cap:segtree"` 导入，也支持 `import cap from "cap"; cap.segtree.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/segtree/segtree.cp` | `// Source: cp/lib/segtree/segtree.cp (...)` | `SegTreeError`、`RangeStats`、`FenwickTree`、`LazySegmentTree` |
| `lib/segtree/index.cp` | `// Source: cp/lib/segtree/index.cp (...)` | `cap:segtree` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `SegTreeError`

- `IndexOutOfBounds`：单点下标 `idx >= len()` 或区间右端点 `right_excl > len()`。
- `InvalidRange`：区间非法（如修改操作 `left > right_excl` 或查询操作 `left >= right_excl`）。
- `EmptyTree`：在空树（`len() == 0`）上执行更新、单点读取或非空区间查询。
- `OutOfMemory`：底层树节点或缓冲切片分配失败。

### 2. 聚合统计结构 `RangeStats`

- `sum: i64`：区间元素总和。
- `min: i64`：区间最小值。
- `max: i64`：区间最大值。

### 3. `FenwickTree`（树状数组 / Binary Indexed Tree）

- **构建与释放**：
  - `FenwickTree.init(alloc, n): FenwickTree ! SegTreeError`（初始化长度为 `n` 的全零树状数组）
  - `FenwickTree.fromSlice(alloc, values): FenwickTree ! SegTreeError`（$O(N)$ 线性建树）
  - `deinit(*_)`
  - `len(): usize`
- **单点更新与读取**：
  - `add(*_, idx, delta): void ! SegTreeError`（$O(\log N)$ 单点增量）
  - `set(*_, idx, val): void ! SegTreeError`（$O(\log N)$ 单点覆写）
  - `get(idx): i64 ! SegTreeError`（$O(1)$ 读取当前值）
- **前缀 / 区间查询与二分**：
  - `prefixSum(idx_inclusive): i64 ! SegTreeError`（$O(\log N)$ 计算闭区间 `[0, idx_inclusive]` 前缀和）
  - `rangeSum(left, right_excl): i64 ! SegTreeError`（$O(\log N)$ 计算半开区间 `[left, right_excl)` 区间和）
  - `lowerBoundPrefix(target): usize?`（在非负数组上 $O(\log N)$ 树上倍增二分搜索首个满足 `prefixSum(idx) >= target` 的 0-based 下标；空树或总和小于 `target` 返回 `null`）

### 4. `LazySegmentTree`（支持区间加与区间赋值的惰性传播线段树）

- **构建与释放**：
  - `LazySegmentTree.init(alloc, values): LazySegmentTree ! SegTreeError`（$O(N)$ 建树）
  - `deinit(*_)`
  - `len(): usize`
- **区间更新（半开区间 `[left, right_excl)`）**：
  - `rangeAdd(*_, left, right_excl, delta): void ! SegTreeError`（$O(\log N)$ 区间加 `delta`）
  - `rangeAssign(*_, left, right_excl, val): void ! SegTreeError`（$O(\log N)$ 区间覆盖赋值为 `val`，自动覆盖旧加标记并正确合成后续 `rangeAdd` 标记）
- **区间与单点查询**：
  - `query(*_, left, right_excl): RangeStats ! SegTreeError`（$O(\log N)$ 同时返回 `{ sum, min, max }`）
  - `querySum(*_, left, right_excl): i64 ! SegTreeError`
  - `queryMin(*_, left, right_excl): i64 ! SegTreeError`
  - `queryMax(*_, left, right_excl): i64 ! SegTreeError`
  - `get(*_, idx): i64 ! SegTreeError`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { FenwickTree, LazySegmentTree } from "cap:segtree";
import cap from "cap";

pub fn demoSegTree(): ! {
    const alloc = std.heap.page_allocator;

    const weights: [5]i64 = [ 3, 1, 4, 2, 5 ];
    let bit = FenwickTree.fromSlice(alloc, weights[0..])?;
    defer bit.deinit();

    bit.add(1, 2)?; // [3, 3, 4, 2, 5]
    const total = bit.rangeSum(0, 5)?; // 17
    const idx = bit.lowerBoundPrefix(10); // Some(2)
    * = total;
    * = idx;

    let seg = cap.segtree.LazySegmentTree.init(alloc, weights[0..])?;
    defer seg.deinit();

    seg.rangeAdd(1, 4, 10)?;   // [3, 11, 14, 12, 5]
    seg.rangeAssign(0, 3, -5)?; // [-5, -5, -5, 12, 5]
    const stats = seg.query(0, 5)?;
    * = stats;
}
```
