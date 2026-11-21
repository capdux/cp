# `cap:lockfree_queue` — 无锁并发环形队列与有界通道

## 1. 概述

`cap:lockfree_queue`（同时支持 `cap.lockfree_queue` 动态挂载）提供工业级无锁并发队列与有界阻塞通道原语：
- **`SpscQueue(T, CAP)`**：Wait-Free 单生产者单消费者无锁环形队列，采用 `align(64)` 缓存行隔离 `head` 与 `tail` 消除伪共享，零锁零堆分配。
- **`MpmcQueue(T, CAP)`**：基于 Dmitry Vyukov 槽位序列号算法的无锁有界多生产者多消费者队列，支持高竞争并发入队出队且免疫 ABA 问题。
- **`BoundedChannel(T)`**：基于动态堆容量与 `Mutex` + `Condition` 的有界阻塞/非阻塞通道，支持 `close()` 后优雅排空剩余元素。

## 2. 核心类型

| 类型 / 错误 | 说明 |
| :--- | :--- |
| `SpscQueue(comptime T: type, comptime CAP: usize)` | 编译期固定 2 的幂次容量的 Wait-Free SPSC 环形队列 |
| `MpmcQueue(comptime T: type, comptime CAP: usize)` | 编译期固定 2 的幂次容量（`>= 2`）的 Dmitry Vyukov MPMC 队列 |
| `BoundedChannel(comptime T: type)` | 运行期动态容量的线程安全有界阻塞/非阻塞通道 |
| `ChannelError` | `ChannelClosed`、`QueueFull`、`InvalidCapacity`、`OutOfMemory` |

## 3. API 速查

### `SpscQueue(T, CAP)` & `MpmcQueue(T, CAP)`
- `init(): Self` — 初始化空队列（无堆分配）
- `tryPush(self: *Self, item: T): bool` — 非阻塞入队，队满返回 `false`
- `tryPop(self: *Self): T?` — 非阻塞出队，队空返回 `null`
- `len(self: *const Self): usize` — 当前元素数量
- `isEmpty(self: *const Self): bool` — 是否为空
- `isFull(self: *const Self): bool` — 是否已满
- `capacity(self: *const Self): usize` — 队列容量 `CAP`

### `BoundedChannel(T)`
- `init(alloc: std.mem.Allocator, cap: usize): ChannelError!Self` — 分配容量为 `cap` 的有界通道
- `deinit(self: *Self)` — 释放底层缓冲区
- `send(self: *Self, item: T): ChannelError!void` — 阻塞发送，已关闭返回 `ChannelError.ChannelClosed`
- `trySend(self: *Self, item: T): ChannelError!void` — 非阻塞发送，队满返回 `ChannelError.QueueFull`
- `recv(self: *Self): T?` — 阻塞接收，关闭且排空后返回 `null`
- `tryRecv(self: *Self): T?` — 非阻塞接收，队空返回 `null`
- `close(self: *Self)` — 关闭通道并唤醒所有等待线程（保留已入队元素供排空）
- `len(self: *Self): usize` / `isClosed(self: *Self): bool` / `capacity(self: *const Self): usize`

## 4. 示例

```cp
import { SpscQueue, MpmcQueue, BoundedChannel } from "cap:lockfree_queue";

fn main() {
    var q = SpscQueue(u64, 1024).init();
    _ = q.tryPush(42);
    const val = q.tryPop(); // 42
    _ = val;
}
```
