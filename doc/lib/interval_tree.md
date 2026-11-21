# `cap:interval_tree` — 纯 CP 增强区间树 (Interval Tree)

## 概述

`cap:interval_tree`（亦支持 `cap.interval_tree` 动态挂载）实现带子树最大右端点 (`max_high`) 剪枝的泛型区间树 `IntervalTree(T, V)`，支持区间插入、闭区间重叠检索 `queryOverlap`、单点命中查询 `queryPoint` 与快速冲突判定 `hasOverlap`。

## 核心类型与 API

- `IntervalError`: `InvalidInterval`
- `Interval(T, V)`: `{ low: T, high: T, value: V }`
- `IntervalTree(T, V)`: `init(alloc)`、`deinit(&self)`、`insert(&self, low, high, value)`、`queryOverlap(self, alloc, low, high)`、`queryPoint(self, alloc, point)`、`hasOverlap(self, low, high)`、`len(self)`

## 示例

```cp
import { IntervalTree } from "cap:interval_tree";

let tree = IntervalTree(i64, []const u8).init(alloc);
defer tree.deinit();
try tree.insert(10, 20, "meeting-a");
const hits = try tree.queryPoint(alloc, 15);
defer alloc.free(hits);
```
