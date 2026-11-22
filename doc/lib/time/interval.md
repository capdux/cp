# CP 标准库：不相交区间集合代数与增强区间树 (`cap:interval`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

## 一、 架构总览与设计准则

`cap:interval` 是 CP 语言官方内置的纯 CP **半开区间 `[start, end)` 代数、规范化不相交区间集合（`IntervalSet`）与 AVL 平衡增强区间树（`IntervalTree<V>`）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。

既可通过 `import { ... } from "cap:time"` 导入，也支持 `import cap from "cap"; cap.interval.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/interval/interval.cp` | `// Source: cp/lib/interval/interval.cp (...)` | `IntervalError`、`Interval`、`IntervalSet`、`IntervalEntry<V>`、`IntervalTree<V>` |
| `lib/interval/index.cp` | `// Source: cp/lib/interval/index.cp (...)` | `cap:interval` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `IntervalError`

- `InvalidInterval`：传入非法区间 `start > end`。
- `EmptyInterval`：向 `IntervalTree<V>.insert` 传入空区间 `start == end`。
- `OutOfMemory`：底层动态数组、切片或节点池内存分配失败。

### 2. 半开区间值类型 `Interval`

表示整数半开区间 `[start, end)`（校验 `start <= end`）：

- `Interval.init(start: i64, end: i64): Interval ! IntervalError`
- `len(): u64`：无符号溢出安全地返回区间长度（空区间返回 `0`）。
- `isEmpty(): bool`：判断 `start >= end`。
- `containsPoint(p: i64): bool`：判断 `start <= p < end`。
- `overlaps(other: Interval): bool`：判断两非空区间是否存在交集。
- `touches(other: Interval): bool`：判断两非空区间是否首尾严格相邻（`a.end == b.start` 或 `b.end == a.start`）。
- `intersect(other: Interval): Interval?`：返回交集区间，不相交返回 `null`。
- `hull(other: Interval): Interval`：返回包含两者的最小凸包区间 `[min(start), max(end))`。

### 3. 规范化不相交区间集合 `IntervalSet`

内部维护严格升序、互不重叠且互不相邻（`items[i].end < items[i+1].start`）的规范化区间序列：

- `IntervalSet.init(allocator)` / `deinit()` / `clear()` / `clone(allocator)` / `items() -> []const Interval`
- `add(start, end): void ! IntervalError`：二分定位并自动合并重叠及首尾相邻区间（`[a, b) + [b, c) -> [a, c)`）；空区间 `[x, x)` 为空操作。
- `subtract(start, end): void ! IntervalError`：精确处理完全覆盖删除、左/右截断与中间挖洞一分为二分裂；空区间 `[x, x)` 为空操作。
- `contains(p: i64): bool`：$O(\log N)$ 点查。
- `containsRange(start, end): bool ! IntervalError`：$O(\log N)$ 判断整个 `[start, end)` 是否被完全覆盖（空区间返回 `true`）。
- `totalLength(): u64`：所有区间覆盖总长度。
- `gaps(allocator): []Interval ! IntervalError`：提取相邻区间之间的空隙切片（调用方通过 `allocator.free(...)` 释放）。
- `unionWith(allocator, other)` / `intersectWith(allocator, other)` / `differenceWith(allocator, other)`：双指针线性 $O(M + N)$ 集合并、交、差代数运算。

### 4. 泛型增强区间树 `IntervalTree<V>`

基于 AVL 自平衡二叉搜索树（按 `(start, end)` 排序）与子树 `max_end` 增强字段，支持可重叠区间高效剪枝检索：

- `IntervalTree<V>.init(allocator)` / `deinit()` / `count(): usize`
- `insert(start: i64, end: i64, value: V): void ! IntervalError`
- `queryPoint(allocator, p: i64): []IntervalEntry<V> ! IntervalError`：利用 `max_end <= p` 与 `start > p` 剪枝查询所有包含 `p` 的区间条目。
- `queryOverlap(allocator, start: i64, end: i64): []IntervalEntry<V> ! IntervalError`：利用 `max_end <= start` 与 `node.start >= end` 剪枝查询所有与 `[start, end)` 重叠的区间条目。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { IntervalSet, IntervalTree } from "cap:time";
import cap from "cap";

pub fn demoInterval(): ! {
    const alloc = std.heap.page_allocator;

    let set = IntervalSet.init(alloc);
    defer set.deinit();
    * = set.add(0, 10)?;
    * = set.add(10, 20)?; // 首尾相邻自动合并为 [0, 20)
    * = set.subtract(5, 12)?; // 分裂为 [0, 5) 与 [12, 20)

    let tree = cap.interval.IntervalTree<i32>.init(alloc);
    defer tree.deinit();
    * = tree.insert(1, 8, 101)?;
    * = tree.insert(4, 12, 102)?;

    const hits = tree.queryPoint(alloc, 5)?;
    defer alloc.free(hits);
}
```
