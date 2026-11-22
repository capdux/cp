# `cap:segment_tree` — 纯 CP 懒传播线段树 (Lazy Segment Tree)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 概述

`cap:segment_tree`（亦支持 `cap.segment_tree` 动态挂载）实现带懒传播 (Lazy Propagation) 的泛型线段树 `SegmentTree(T)`，支持 $O(\log N)$ 区间统一增量 `rangeAdd` 与区间求和 `rangeSum`、区间最小值 `rangeMin`、区间最大值 `rangeMax` 查询。

## 核心类型与 API

- `SegError`: `EmptyTree`、`IndexOutOfBounds`、`InvalidRange`
- `SegmentTree(T)`: `init(alloc, values)`、`deinit(&self)`、`len(self)`、`rangeAdd(&self, left, right, delta)`、`rangeSum(&self, left, right)`、`rangeMin(&self, left, right)`、`rangeMax(&self, left, right)`

## 示例

```cp
import { SegmentTree } from "cap:ds";

const vals: [5]i64 = [ 1, 3, 5, 7, 9 ];
let st = try SegmentTree(i64).init(alloc, &vals);
defer st.deinit();
try st.rangeAdd(1, 3, 10);
const s = try st.rangeSum(0, 4);
```
