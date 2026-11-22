// Source: cp/doc/lib/pqueue.md
# `cap:pqueue` — 索引优先队列、有界 Top-K 与流式双堆中位数

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

纯 CP、零 C 依赖的高性能优先队列与流式统计库，提供带稳定 `u32` 句柄与代际校验的 $O(\log N)$ 索引二叉最小堆（`IndexedMinHeap<V>`）、仅占 $O(K)$ 内存的流式有界 Top-K 保留器（`BoundedTopK<V>`）以及基于大顶堆 + 小顶堆的实时精确中位数追踪器（`StreamingMedian`）。

## 导入方式

```cp
import {
    PQueueError,
    PriorityItem,
    TopKEntry,
    IndexedMinHeap,
    BoundedTopK,
    StreamingMedian,
} from "cap:time";

// 或动态命名空间挂载：
import cap from "cap";
// cap.pqueue.IndexedMinHeap / cap.pqueue.BoundedTopK / cap.pqueue.StreamingMedian / cap.pqueue.PQueueError
```

## 核心类型

### `PQueueError`
- `InvalidHandle`：传入无效、越界、已弹出、已删除或已 `clear()` 的过期句柄
- `InvalidCapacity`：`BoundedTopK.init(allocator, k)` 传入 `k == 0`
- `EmptyQueue`：在空 `StreamingMedian` 上调用 `median()`
- `OutOfMemory`：内存分配失败或槽位容量耗尽

### `PriorityItem<V>`
- `handle: u32`：分配的稳定句柄
- `priority: i64`：当前优先级（越小越优先）
- `value: V`：关联值

### `TopKEntry<V>`
- `score: f64`：评分
- `value: V`：关联值

## 核心 API

### `IndexedMinHeap<V>`
- `IndexedMinHeap<V>.init(allocator): IndexedMinHeap<V>`：初始化空的索引最小堆
- `deinit(*_)`：释放堆与槽位池内存
- `push(*_, priority: i64, value: V): u32 ! PQueueError`：$O(\log N)$ 插入并返回稳定句柄 `handle`（内置代际号防止槽位复用导致的 ABA 误命中）
- `peek(): PriorityItem<V>?`：$O(1)$ 查看堆顶最小优先级元素，空堆返回 `null`
- `pop(*_): PriorityItem<V>?`：$O(\log N)$ 弹出堆顶最小优先级元素，空堆返回 `null`
- `updatePriority(*_, handle: u32, new_priority: i64): void ! PQueueError`：$O(\log N)$ 原地 sift-up / sift-down 更新优先级，无效或已弹出句柄返回 `InvalidHandle`
- `removeByHandle(*_, handle: u32): PriorityItem<V> ! PQueueError`：$O(\log N)$ 原地摘除任意句柄元素并返回，无效句柄返回 `InvalidHandle`
- `containsHandle(handle: u32): bool`：$O(1)$ 检查句柄是否仍处于活跃状态
- `len(): usize` / `isEmpty(): bool`：查询当前元素数量与是否为空
- `clear(*_)`：清空所有元素并使全部已签发句柄立即失效

### `BoundedTopK<V>`
- `BoundedTopK<V>.init(allocator, k: usize): BoundedTopK<V> ! PQueueError`：初始化容量为 `k` 的流式 Top-K 保留器（`k == 0` 返回 `InvalidCapacity`）
- `deinit(*_)`：释放内部堆内存
- `push(*_, score: f64, value: V): void ! PQueueError`：流式写入 `(score, value)`，仅维护 $O(K)$ 最小堆内存
- `topSorted(allocator): ([]TopKEntry<V>) ! PQueueError`：按 `score` 降序返回前 K 项新分配切片（不破坏内部状态）
- `len(): usize`：返回当前保留的元素数量（`<= k`）
- `clear(*_)`：清空已保留元素并保留容量

### `StreamingMedian`
- `StreamingMedian.init(allocator): StreamingMedian`：初始化双堆精确中位数追踪器
- `deinit(*_)`：释放低半区大顶堆与高半区小顶堆内存
- `push(*_, val: f64): void ! PQueueError`：$O(\log N)$ 插入并保持两堆大小差 `<= 1`
- `median(): f64 ! PQueueError`：$O(1)$ 返回精确中位数，空追踪器返回 `EmptyQueue`
- `count(): usize`：返回累计插入元素总数
- `clear(*_)`：清空所有元素并保留容量
