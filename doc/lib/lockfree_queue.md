# `cap:lockfree_queue` — 无锁并发环形队列、侵入式无界队列与有界通道

## 1. 概述

`cap:lockfree_queue`（同时支持 `cap.lockfree_queue` 动态挂载）提供工业级无锁并发队列与有界阻塞通道原语：
- **`SpscQueue(T, CAP)`**：Wait-Free 单生产者单消费者无锁环形队列，采用 `align(64)` 缓存行隔离 `head` 与 `tail` 消除伪共享，零锁零堆分配。
- **`MpmcQueue(T, CAP)`**：基于 Dmitry Vyukov 槽位序列号算法的无锁有界多生产者多消费者队列，支持高竞争并发入队出队且免疫 ABA 问题。
- **`BoundedChannel(T)`**：基于动态堆容量与 `Mutex` + `Condition` 的有界阻塞/非阻塞通道，支持 `close()` 后优雅排空剩余元素。
- **`UnboundedQueue(T, next_field)`**（吸收自 Bun `threading/unbounded_queue.cp`）：零堆分配、缓存行填充防伪共享的侵入式无锁无界并发队列，支持单元素 `push`/`pop`、批量 `pushBatch`/`popBatch` 与 `Batch.Iterator` 遍历。

## 2. 核心类型

| 类型 / 错误 | 说明 |
| :--- | :--- |
| `SpscQueue(comptime T: type, comptime CAP: usize)` | 编译期固定 2 的幂次容量的 Wait-Free SPSC 环形队列 |
| `MpmcQueue(comptime T: type, comptime CAP: usize)` | 编译期固定 2 的幂次容量（`>= 2`）的 Dmitry Vyukov MPMC 队列 |
| `BoundedChannel(comptime T: type)` | 运行期动态容量的线程安全有界阻塞/非阻塞通道 |
| `ChannelError` | `ChannelClosed`、`QueueFull`、`InvalidCapacity`、`OutOfMemory` |
| `UnboundedQueue(comptime T: type, comptime next_field)` | 侵入式无锁无界并发队列（支持 `push`/`pop`/`pushBatch`/`popBatch`） |
| `CACHE_LINE_LENGTH` / `QUEUE_PADDING_LENGTH` | 目标架构缓存行长度与防伪共享填充字节数 |

## 3. API 速查

### `SpscQueue(T, CAP)` & `MpmcQueue(T, CAP)`
- `init(): Self` — 初始化空队列（无堆分配）
- `tryPush(*_, item: T): bool` — 非阻塞入队，队满返回 `false`
- `tryPop(*_): T?` — 非阻塞出队，队空返回 `null`
- `len(): usize` / `isEmpty(): bool` / `isFull(): bool` / `capacity(): usize`

### `BoundedChannel(T)`
- `init(alloc: std.mem.Allocator, cap: usize)` — 分配容量为 `cap` 的有界通道
- `deinit(*_)` — 释放底层缓冲区
- `send(*_, item: T)` / `trySend(*_, item: T)` — 阻塞/非阻塞发送
- `recv(*_): T?` / `tryRecv(*_): T?` — 阻塞/非阻塞接收
- `close(*_)` — 关闭通道并唤醒等待线程（保留剩余元素供排空）

### `UnboundedQueue(T, next_field)`
- `init(): Self` — 初始化空侵入式队列（生产端 `back` 与消费端 `front` 缓存行隔离）
- `push(*_, item: *T!)` — 无锁单节点入队
- `pushBatch(*_, first: *T!, last: *T!)` — 无锁批量入队已链接节点段
- `pop(*_): (*T!)?` — 无锁单节点出队，队空返回 `null`
- `popBatch(*_): Batch` — 一次性原子摘取当前队列全部节点为 `Batch`
- `isEmpty(): bool` — 判断队列是否为空
- `Batch`：`front` / `last` / `count` / `isEmpty()` / `len()` / `iterator()` -> `Iterator.next(): (*T!)?`

## 4. 示例

```cp
import { SpscQueue, UnboundedQueue } from "cap:lockfree_queue";

struct TaskNode {
    id: u32,
    next: (*TaskNode!)? = null,
}

const TaskQueue = UnboundedQueue(TaskNode, .next);
let uq = TaskQueue.init();
let n1 = TaskNode{ id: 1 };
let n2 = TaskNode{ id: 2 };
uq.push(&n1);
uq.push(&n2);
const batch = uq.popBatch();
let it = batch.iterator();
while (node := it.next()) {
    // 依次消费 n1, n2
}
```
