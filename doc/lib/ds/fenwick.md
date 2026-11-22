# `cap:fenwick` — 纯 CP 树状数组 (Fenwick / Binary Indexed Tree)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 概述

`cap:fenwick`（亦支持 `cap.fenwick` 动态挂载）实现泛型 $O(\log N)$ 树状数组 `FenwickTree(T)`，支持单点增量 `add`、单点赋值 `set`、前缀和 `prefixSum`、闭区间和 `rangeSum` 及二进制提升二分查找 `lowerBound`。

## 核心类型与 API

- `FenwickError`: `IndexOutOfBounds`、`InvalidRange`
- `FenwickTree(T)`: `init(alloc, size)`、`initFromSlice(alloc, values)`、`deinit(&self)`、`add(&self, idx, delta)`、`set(&self, idx, val)`、`get(self, idx)`、`prefixSum(self, idx)`、`rangeSum(self, left, right)`、`lowerBound(self, target)`

## 示例

```cp
import { FenwickTree } from "cap:ds";

const vals: [5]i64 = [ 1, 2, 3, 4, 5 ];
let ft = try FenwickTree(i64).initFromSlice(alloc, &vals);
defer ft.deinit();
const s = try ft.rangeSum(1, 3); // 2 + 3 + 4 = 9
```
