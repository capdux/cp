# `cap:fallback_alloc` — 栈内联缓冲优先 + 堆回退分配器

> 参考来源：/data/dev/bun/src/bun_alloc/BufferFallbackAllocator.cp | 吸收日期：2026-11-22

源自 Bun `bun_alloc/BufferFallbackAllocator.cp`，优先从调用方提供的固定栈/静态缓冲（`FixedBufferAllocator`）零系统调用分配内存，缓冲耗尽后无缝透明回退到后备 `std.mem.Allocator`（如 GPA/Arena），并在 `resize`/`remap`/`free` 时自动按指针归属路由。

## 导出符号

- `BufferFallbackAllocator`:
  - `init(buffer: []u8, fallback_allocator: std.mem.Allocator): BufferFallbackAllocator`
  - `allocator(*_): std.mem.Allocator`
  - `ownsBufferPtr(ptr: [*]const u8): bool`
  - `usedBytes(): usize` / `capacity(): usize` / `remainingBytes(): usize`
  - `reset(*_)`

## 上游行为契约与内存生命周期边界

1. **快路径零系统调用 + 透明堆回退**：
   - 99% 小对象请求（如 HTTP 头部拼接、短路径归一化、临时 AST 节点）直接命中栈内联 `buffer` 完成 O(1) bump 分配；仅当 `remainingBytes()` 不足时自动路由至 `fallback_allocator`，调用方代码无需感知分支差异。
2. **指针归属自动分流 (`ownsBufferPtr`)**：
   - `free` / `resize` / `remap` 通过地址区间判断指针是否属于内联 `buffer`：属于栈缓冲的释放走内部 `FixedBufferAllocator`，属于后备堆的释放精确转发给 `fallback_allocator`，绝不跨分配器误释放。
3. **结构体地址稳定与 `reset()` 契约**：
   - 通过 `allocator()` 导出的 `std.mem.Allocator` 句柄持有指向 `BufferFallbackAllocator` 实例的指针，使用期间严禁按值移动该实例；`reset()` 仅重置栈内联缓冲游标，不释放已溢出至 `fallback_allocator` 的堆分配。
