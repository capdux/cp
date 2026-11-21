# `cap:union_interval` — 纯 CP 不相交区间合并集合 (`IntervalSet`)

## 概述

`cap:union_interval`（亦支持 `cap.union_interval` 动态挂载）实现泛型半开区间合并集合 `IntervalSet(T)`，自动维护有序不相交 `[start, end)` 区间：插入时自动合并重叠或相邻区间，删除时自动分裂或裁剪区间，并支持 $O(\log N)$ 二分点查询 `containsPoint` 与总覆盖长度 `totalCovered`。

## 核心类型与 API

- `RangeError`: `InvalidRange`
- `Range(T)`: `{ start: T, end: T }`
- `IntervalSet(T)`: `init(alloc)`、`deinit(&self)`、`addRange(&self, start, end)`、`removeRange(&self, start, end)`、`containsPoint(self, point)`、`totalCovered(self)`、`count(self)`、`items(self)`、`clear(&self)`

## 示例

```cp
import { IntervalSet } from "cap:union_interval";

let set = IntervalSet(i64).init(alloc);
defer set.deinit();
try set.addRange(10, 20);
try set.addRange(20, 30); // merges into [10, 30)
```
