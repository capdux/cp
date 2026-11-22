# `cap:intrusive_heap` — 零堆分配侵入式配对堆 (Intrusive Pairing Heap)

> 参考来源：/data/dev/bun/src/io/heap.cp | 吸收日期：2026-11-22

提供零堆分配、缓存友好的侵入式配对最小堆，适用于定时器调度、异步事件循环与任务优先级队列。

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

## 上游行为契约与侵入式指针安全

1. **均摊复杂度与两遍合并（Two-Pass Pairing）**：
   - 继承自 Bun `io/heap.cp` 配对堆实现：`insert` 与 `peek` 均为 $O(1)$，`deleteMin` 与任意节点 `remove` 采用两遍配对合并策略，均摊复杂度 $O(\log N)$ 且常数项显著低于红黑树，极适合高吞吐定时器取消场景。
2. **节点生命周期与地址稳定约束**：
   - `IntrusiveHeap` 不拥有节点内存，入堆期间节点的内存地址不可移动（切勿将堆内节点存放在可能触发扩容搬迁的动态数组中，推荐配合 `cap:hive_array` 或 `cap:object_pool` 使用）；
   - 严禁将同一节点重复 `insert` 入堆，亦不可在未 `remove` / `deleteMin` 前提前释放节点内存。
3. **比较器严格弱序契约**：
   - `less(ctx, a, b)` 须满足严格弱序（`less(a, a) == false`）；在堆中就地修改节点排序键（如调整 `deadline`）前必须先 `remove` 再重新 `insert`。
