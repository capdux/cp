# `cap:collections` — 工业级基础数据结构集合标准库

## 1. 概述
`cap:collections`（亦可通过 `cap.collections` 动态挂载访问）提供零额外运行时依赖、确定性内存管理的高性能基础数据结构，涵盖动态位图、有界/无界双端队列、栈优先小向量与通用二叉堆。

## 2. 核心类型
- `CollectionError`：集合操作错误集（`Full`、`Empty`、`OutOfBounds`）。
- `BitSet`：基于 64 位字数组的动态位图，支持位操作、集合代数（并/交/差）与硬件 `@ctz` 加速的 `nextSetBit`。
- `BoundedDeque(T, N)`：固定容量无堆双端队列，环形缓冲区实现，满时返回 `CollectionError.Full`。
- `RingDeque(T)`：按需动态扩容的环形双端队列，支持头尾 O(1) 均摊入队与出队。
- `SmallVec(T, INLINE_CAP)`：栈上内联小向量，元素数不超过 `INLINE_CAP` 时零堆分配，超出后平滑溢出至堆。
- `BinaryHeap(T, cmp)`：基于 `std.ArrayList(T).empty` 的通用二叉堆，通过 `cmp(a, b) == .lt` 决定优先级。

## 3. API 速查
- **`BitSet`**: `init(alloc, bit_len)` / `deinit()` / `set(idx)` / `unset(idx)` / `isSet(idx)` / `toggle(idx)` / `clearAll()` / `countSetBits()` / `unionWith(&other)` / `intersectWith(&other)` / `differenceWith(&other)` / `nextSetBit(from)`
- **`BoundedDeque(T, N)`**: `init()` / `pushFront(v)` / `pushBack(v)` / `popFront()` / `popBack()` / `peekFront()` / `peekBack()` / `len()` / `capacity()` / `isEmpty()` / `isFull()` / `clear()`
- **`RingDeque(T)`**: `init(alloc)` / `deinit()` / `pushFront(v)` / `pushBack(v)` / `popFront()` / `popBack()` / `peekFront()` / `peekBack()` / `len()` / `isEmpty()` / `clear()`
- **`SmallVec(T, INLINE_CAP)`**: `init()` / `deinit(alloc)` / `push(alloc, v)` / `pop()` / `slice()` / `sliceMut()` / `len()` / `isEmpty()` / `isHeap()` / `clear()`
- **`BinaryHeap(T, cmp)`**: `init(alloc)` / `deinit()` / `push(v)` / `pop()` / `peek()` / `len()` / `isEmpty()` / `clear()`

## 4. 示例
```cp
import { BitSet, SmallVec, BinaryHeap } from "cap:collections";
const std = @import("std");

fn cmpAsc(a: i32, b: i32): std.math.Order {
    return std.math.order(a, b);
}

pub fn example(alloc: std.mem.Allocator): !void {
    let bs = try BitSet.init(alloc, 128);
    defer bs.deinit();
    bs.set(5);
    bs.set(70);
    const next = bs.nextSetBit(6); // 70

    let sv = SmallVec(i32, 4).init();
    defer sv.deinit(alloc);
    try sv.push(alloc, 10);

    let heap = BinaryHeap(i32, cmpAsc).init(alloc);
    defer heap.deinit();
    try heap.push(42);
    _ = next;
}
```
