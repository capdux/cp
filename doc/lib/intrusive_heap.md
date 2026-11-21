# `cap:intrusive_heap` — 零堆分配侵入式配对堆 (Intrusive Pairing Heap)

> 吸收自 `/data/dev/bun/src/io/heap.cp`，提供零堆分配、缓存友好的侵入式配对最小堆，适用于定时器调度、异步事件循环与任务优先级队列。

## 核心导出 API

| 符号 | 说明 |
| :--- | :--- |
| `IntrusiveField(T)` | 嵌入节点结构体的三指针链接字段（`child` / `prev` / `next`） |
| `IntrusiveHeap(T, Context, less)` | 侵入式最小配对堆泛型容器（`less(ctx, a, b)` 返回 `a < b`） |
| `init(ctx)` / `isEmpty()` / `peek()` / `count()` / `findMax()` | 构造、判空、查看最小元素、统计节点数、查找最大元素 |
| `insert(*_, v)` / `deleteMin(*_)` / `remove(*_, v)` | 均摊 $O(1)$ 插入、均摊 $O(\log N)$ 弹出最小或任意节点删除 |

## 示例

```cp
import { IntrusiveField, IntrusiveHeap } from "cap:intrusive_heap";

struct TimerNode {
    deadline: u64,
    heap: IntrusiveField(TimerNode) = IntrusiveField(TimerNode){},
}
```
