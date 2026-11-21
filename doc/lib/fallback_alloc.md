# `cap:fallback_alloc` — 栈内联缓冲优先 + 堆回退分配器

源自 Bun `bun_alloc/BufferFallbackAllocator.cp`，优先从调用方提供的固定栈/静态缓冲（`FixedBufferAllocator`）零系统调用分配内存，缓冲耗尽后无缝透明回退到后备 `std.mem.Allocator`（如 GPA/Arena），并在 `resize`/`remap`/`free` 时自动按指针归属路由。

## 导出符号

- `BufferFallbackAllocator`:
  - `init(buffer: []u8, fallback_allocator: std.mem.Allocator): BufferFallbackAllocator`
  - `allocator(*_): std.mem.Allocator`
  - `ownsBufferPtr(ptr: [*]const u8): bool`
  - `usedBytes(): usize` / `capacity(): usize` / `remainingBytes(): usize`
  - `reset(*_)`
