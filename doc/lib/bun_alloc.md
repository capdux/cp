# `cap:bun_alloc` (`cap.bun_alloc`) — 高性能自定义内存分配器套件

## 1. 概述

`lib/bun_alloc` 是纯 CP 实现的工业级自定义内存分配器套件，完全基于标准 `std.mem.Allocator` 接口构建，零 `mimalloc` / `std.posix` 耦合，适用于高频临时分配、固定对象池复用、栈缓冲零堆分配以及内存泄漏审计。

---

## 2. 核心类型

| 类型 | 职责 | 适用场景 |
| :--- | :--- | :--- |
| `BumpAllocator` | 固定栈缓冲零堆 Bump 分配器 | 已知上限的小缓冲、零堆分配热路径 |
| `PoolAllocator(T)` | 固定尺寸 O(1) 空闲链表对象池 | AST 节点、连接会话、高频创建/销毁对象 |
| `TrackingAllocator` | 审计追踪与泄漏检测包装分配器 | 单元测试泄漏审计、峰值内存分析 |
| `ScratchArena` | 支持 `mark()` / `rewind()` 的作用域暂存 Arena | 编译器/解析器阶段性临时内存、局部回滚 |
| `Checkpoint` | `ScratchArena` 状态快照句柄 | 记录并恢复 Arena 分配偏移 |
| `HeapBreakdown` | 外部切片驱动命名内存分区分析器（`Bun__<Zone>`） | 跨平台按子系统统计实时/峰值内存、泄漏审计与排序报表 |

---

## 3. API 速查

### `BumpAllocator`
- `BumpAllocator.init(buffer: []u8): BumpAllocator`
- `alloc(*_, len: usize, alignment: std.mem.Alignment): [*]u8?`
- `allocBytes(*_, len: usize): []u8?`
- `reset(*_)`, `usedBytes(): usize`, `remainingBytes(): usize`, `capacity(): usize`, `ownsPtr(ptr: [*]const u8): bool`
- `allocator(*_): std.mem.Allocator`

### `PoolAllocator(T)`
- `PoolAllocator(T).init(allocator: std.mem.Allocator): Self`
- `PoolAllocator(T).initWithOptions(allocator: std.mem.Allocator, chunk_size: usize, max_objects: usize): Self`
- `prewarm(*_, count: usize): !void`
- `create(*_): !*T` / `recycle(*_, ptr: *T)`
- `activeCount(): usize` / `capacity(): usize` / `deinit(*_)`

### `TrackingAllocator`
- `TrackingAllocator.init(child_allocator: std.mem.Allocator): TrackingAllocator`
- `allocator(*_): std.mem.Allocator`
- `hasLeaks(): bool` / `resetStats(*_)`
- `formatStats(alloc: std.mem.Allocator): ![]u8`
- 统计字段：`alloc_count`, `free_count`, `realloc_count`, `live_bytes`, `peak_bytes`, `total_allocated_bytes`

### `ScratchArena`
- `ScratchArena.init(allocator: std.mem.Allocator): ScratchArena`
- `ScratchArena.initWithChunkSize(allocator: std.mem.Allocator, chunk_size: usize): ScratchArena`
- `mark(): Checkpoint` / `rewind(*_, cp: Checkpoint)`
- `resetRetainingCapacity(*_)` / `usedBytes(): usize` / `capacityBytes(): usize` / `chunkCount(): usize`
- `allocator(*_): std.mem.Allocator` / `deinit(*_)`

### `HeapBreakdown`（吸收自 `/data/dev/bun/src/bun_alloc/heap_breakdown.cp`）
- 常量与辅助：`HEAP_ZONE_PREFIX` (`"Bun__"`), `heapZoneBaseName(qualified: []const u8): []const u8`, `formatHeapZoneLabel(name: []const u8, out: []u8): HeapBreakdownError!usize`
- `HeapBreakdown.init(storage: []ZoneStats): HeapBreakdown`（外部切片驱动，无硬编码分区上限）
- `registerZone(*_, name)` / `getOrRegisterZone(*_, name)` / `findZone(name)` / `stats(handle)`
- `recordAlloc(*_, handle, bytes)` / `recordFree(*_, handle, bytes)` / `recordResize(*_, handle, old_bytes, new_bytes)`
- `resetZone(*_, handle)` / `resetAll(*_)` / `totalLiveBytes()` / `totalPeakBytes()` / `totalAllocatedBytes()` / `totalActiveAllocations()` / `hasLeaks()` / `heaviestLiveZone()`
- `sortByLiveBytesDesc(*_)` / `formatReport(out: []u8): HeapBreakdownError!usize`

---

## 4. 示例

```cp
import std from "std";
import { BumpAllocator, ScratchArena, TrackingAllocator } from "cap:bun_alloc";

fn example(): !void {
    let tracker = TrackingAllocator.init(std.heap.page_allocator);
    let arena = ScratchArena.init(tracker.allocator());
    defer arena.deinit();

    const cp = arena.mark();
    const tmp = try arena.allocator().alloc(u8, 256);
    _ = tmp;
    arena.rewind(cp);
}
```
