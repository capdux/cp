# CP 标准库：有界环形缓冲区、字节流管道与微批聚合器手册 (`cap:ringbuf`)

## 一、 架构总览与设计准则

`cap:ringbuf` 是 CP 语言官方内置的纯 CP **有界环形缓冲区（`RingBuffer<T>`）、高性能字节环形流管道（`ByteRing`）与条数/时间双触发微批聚合器（`MicroBatcher<T>`）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为流式 I/O、LLM Token 流缓冲、音频帧队列与遥测微批上报提供确定性内存安全的高性能原语。既可通过 `import { ... } from "cap:ringbuf"` 导入，也支持 `import cap from "cap"; cap.ringbuf.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ringbuf/ring.cp` | `// Source: cp/lib/ringbuf/ring.cp (...)` | `RingError`、`OverflowPolicy`、`RingBuffer<T>`、`ByteRing`、`MicroBatcher<T>` |
| `lib/ringbuf/index.cp` | `// Source: cp/lib/ringbuf/index.cp (...)` | `cap:ringbuf` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与溢出策略

- **`RingError`**：
  - `BufferFull`：`RingBuffer` 在 `OverflowPolicy.Reject` 模式下满载写入，或 `MicroBatcher` 已达 `max_batch_size` 未 `drain` 时继续 `add`。
  - `InvalidCapacity`：初始化容量 `capacity == 0`。
  - `InvalidConfig`：`MicroBatcher` 配置 `max_batch_size == 0` 或 `max_wait_ms < 0`。
  - `OutOfMemory`：底层缓冲区或切片分配失败。
- **`OverflowPolicy`**：
  - `Reject`：缓冲区满时拒绝写入并返回 `RingError.BufferFull`。
  - `OverwriteOldest`：缓冲区满时覆盖最旧元素，推进读游标并累加 `overwrittenCount()`。

### 2. `RingBuffer<T>`（泛型动态容量环形缓冲区）

- **`RingBuffer(T).init(alloc, capacity, policy): Self ! RingError` / `deinit()`**：初始化定容环形缓冲区与确定性释放。
- **`push(item): void ! RingError`**：写入元素；满载时按 `OverflowPolicy` 拒绝或覆盖最旧元素。
- **`pop(): T?` / `peek(): T?` / `peekAt(logical_idx): T?`**：弹出队首、只读查看队首，或按逻辑索引 `0..len()-1` 零拷贝随机查看（跨环回边界自动取模）。
- **`popBatch(out_buf: []T): usize`**：一次弹出最多 `out_buf.len` 个元素，跨环回边界采用两段 `@memcpy` 拷贝。
- **`len(): usize` / `capacity(): usize` / `isEmpty(): bool` / `isFull(): bool` / `overwrittenCount(): u64` / `clear()`**。

### 3. `ByteRing`（高性能字节环形流管道）

- **`ByteRing.init(alloc, capacity): ByteRing ! RingError` / `deinit()`**：初始化与释放字节环形流管道。
- **`write(src: []const u8): usize`**：写入尽可能多字节，支持跨尾回绕（wrap-around）两段 `@memcpy`。
- **`read(dst: []u8): usize` / `peek(dst: []u8): usize` / `skip(n: usize): usize`**：消费读取、只读探测（不推进游标）与快速跳过。
- **`readableLen(): usize` / `writableLen(): usize` / `capacity(): usize` / `isEmpty(): bool` / `isFull(): bool` / `clear()`**。
- **`readUntilByte(alloc, delim: u8): ([]u8)?!`**: 扫描当前可读字节流，若包含分隔符 `delim` 则消费并返回包含该分隔符的完整帧切片（适合按 `\n` 增量切分流式行协议，调用方用 `alloc.free` 释放）。

### 4. `MicroBatcher<T>`（条数 + 超时双阈值微批聚合器）

- **`MicroBatcher(T).init(alloc, max_batch_size, max_wait_ms): Self ! RingError` / `deinit()`**：初始化最大批大小与最大等待时间窗口（毫秒）。
- **`add(item: T, now_ms: i64): bool ! RingError`**：追加元素并记录首批首元素时间戳；当达到 `max_batch_size` 时返回 `true` 提示可立即 flush。
- **`shouldFlush(now_ms: i64): bool`**：非空且满足 `len() >= max_batch_size` 或 `now_ms - first_item_ms >= max_wait_ms` 时返回 `true`。
- **`drain(alloc): []T ! RingError`**：取出当前全部批次切片（调用方用 `alloc.free` 释放）并重置计数与计时起点。
- **`len(): usize` / `isEmpty(): bool`**。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { RingBuffer, ByteRing, MicroBatcher, OverflowPolicy } from "cap:ringbuf";
import cap from "cap";

pub fn demoRingBuf(): ! {
    const alloc = std.heap.page_allocator;

    let rb = RingBuffer(i64).init(alloc, 4, OverflowPolicy.OverwriteOldest)?;
    defer rb.deinit();
    rb.push(10)?;
    rb.push(20)?;

    let pipe = cap.ringbuf.ByteRing.init(alloc, 16)?;
    defer pipe.deinit();
    * = pipe.write("tok1\ntok2");
    if (frame := pipe.readUntilByte(alloc, '\n')?) {
        defer alloc.free(frame);
        // frame == "tok1\n"
    }

    let batcher = MicroBatcher(i32).init(alloc, 8, 50)?;
    defer batcher.deinit();
    let ready = batcher.add(1, 1000)?;
    if (ready || batcher.shouldFlush(1050)) {
        const batch = batcher.drain(alloc)?;
        defer alloc.free(batch);
    }
}
```
