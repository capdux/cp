# CP 标准库：并发同步原语、无锁队列与多线程池使用手册 (`cap:sync` & `cap:threading`)

> 参考来源：`/data/dev/bun/src/threading/threading.cp`, `/data/dev/bun/src/threading/` | 吸收日期：2026-11-22

## 一、 架构总览与设计哲学

`cap:sync`（与底层 `cap:threading` 共享同步原语）是 CP 语言专为**多核低延迟并发、用户态快速锁（Futex）、无锁队列（Lock-Free MPMC / Unbounded Batch Queue）、分片并发字典（Striped ConcurrentMap）与零拷贝跨线程共享切片（ArcSlice）**设计的工业级并发标准库。完整吸收自 Bun（`/data/dev/bun/src/threading/`），在非竞争快路径下实现 **0 系统调用** 的纯原子寄存器操作。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                      cap:sync & cap:threading 多核并发同步体系                          │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│    内核/用户态混合锁原语   │    无锁与有界并发通信管道    │   分片并发容器与线程池调度    │
│  • Futex & Deadline       │  • MpmcQueue / RingBuffer   │  • ConcurrentMap<K, V> (32片)│
│  • Mutex (withLock RAII)  │    (Vyukov 序列号无锁环)    │  • ArcSlice<T> (原子零拷贝)  │
│  • Condition (条件变量)   │  • UnboundedQueue<T, field> │  • Guarded<T> (互斥保护容器) │
│  • WaitGroup (并发屏障)   │    (缓存行对齐无锁批次队列) │  • WorkPool & ThreadPool     │
│                           │  • Channel<T> (CSP 通道)    │    (Work-Stealing 工作窃取)  │
└───────────────────────────┴─────────────────────────────┴──────────────────────────────┘
```

---

## 二、 核心并发原语与数据结构详解

### 1. 用户态快速互斥锁、条件变量与屏障 (`Futex` / `Mutex` / `Condition` / `WaitGroup`)

源自 `/data/dev/bun/src/threading/Futex.cp`、`Mutex.cp`、`Condition.cp`、`WaitGroup.cp`：
- **`Futex` & `Deadline`**：直接封装 Linux `FUTEX_WAIT_PRIVATE` / `FUTEX_WAKE_PRIVATE` 系统调用，支持单调时钟纳秒级超时控制；
- **`Mutex`**：3 态（`unlocked = 0b00`、`locked = 0b01`、`contended = 0b11`）用户态自旋/Futex 混合锁。Debug 模式下自动跟踪持有线程 ID 并实时拦截自死锁（Deadlock Detection）；支持 `withLock(cb)` 闭包自动 RAII 加解锁；
- **`Condition`**：基于 32 位 `state`（低 16 位等待者计数 + 高 16 位信号计数）与 `epoch` 代次计数器的高性能条件变量，支持 `wait`、`timedWait`、`signal` 与 `broadcast`；
- **`WaitGroup`**：原子计数同步屏障（`add` / `done` / `finish` / `wait`），用于等待一组并发任务全部收敛完成。

```typescript
import {
    Futex,
    Mutex,
    Condition,
    WaitGroup,
    Guarded,
} from "cap:sync";

pub fn demoLocksAndWaitGroup() {
    // 1. Mutex 显式加锁与 tryLock
    let mu: Mutex = .{};
    defer mu.deinit();

    if (mu.tryLock()) {
        mu.unlock();
    }

    // 2. Guarded<T> 带锁保护的状态容器
    let counter = Guarded<i64>.init(100);
    counter.set(counter.get() + 50); // 150

    // 3. WaitGroup 并发完成屏障
    let wg = WaitGroup.init();
    wg.add(2);
    wg.done();
    wg.done();
    wg.wait(); // 当计数归零时立即放行
}
```

---

### 2. 无锁有界 MPMC 环形队列与侵入式无界批次队列 (`MpmcQueue` / `RingBuffer` & `UnboundedQueue`)

源自 `/data/dev/bun/src/threading/channel.cp` 与 `unboundedQueue.cp`：
- **`MpmcQueue(T, CAP)` / `RingBuffer(T, CAP)`**：基于 Dmitry Vyukov 的多生产者多消费者无锁有界队列，每个槽位维护独立的原子轮次号 `turn`，彻底消除伪共享与 ABA 问题；
- **`UnboundedQueue(T, .next)`**：按 CPU 缓存行（x86_64/ARM64 下 64~128 字节 `align`）物理隔离 `front` 与 `back` 指针的侵入式无界无锁队列，支持 `push`、`pushBatch`、`pop` 以及单次原子 `swap` 整链摘除的 `popBatch()`。

```typescript
import { MpmcQueue, RingBuffer, UnboundedQueue } from "cap:sync";

pub struct JobNode {
    id: i32 = 0,
    next: (*JobNode)? = null,
}

pub fn demoLockFreeQueues() {
    // 1. Vyukov 无锁有界 MPMC 环形队列
    let ring = RingBuffer(i32, 4).init();
    ring.tryPush(10);
    ring.tryPush(20);
    const v1 = ring.tryPop().?; // 10

    // 2. 侵入式无界无锁批次队列 UnboundedQueue
    const JobQueue = UnboundedQueue(JobNode, .next);
    let uq: JobQueue = .{};
    let n1 = JobNode{ id: 1, next: null };
    let n2 = JobNode{ id: 2, next: null };

    uq.push(&n1);
    uq.push(&n2);

    if (batch := uq.popBatch()) {
        // batch.count == 2，可无锁遍历整批节点
        let iter = batch.iterator();
        while (node := iter.next()) {
            // 处理 node.id
        }
    }
}
```

---

### 3. CSP 有界阻塞/非阻塞通道 (`Channel<T>`)

源自 `/data/dev/bun/src/threading/channel.cp`。结合 `Mutex` 与双条件变量（`not_empty` / `not_full`），提供阻塞式 `send` / `recv`、非阻塞式 `trySend` / `tryRecv` 以及优雅关闭广播 `close()`：

```typescript
import std from "std";
import { Channel } from "cap:sync";

pub fn demoChannel()! {
    const allocator = std.heap.page_allocator;
    let ch = try Channel(i32).init(allocator, 4);
    defer ch.deinit();

    try ch.send(101);
    _ = try ch.trySend(202);

    const a = ch.tryRecv().?; // 101
    const b = (try ch.recv()).?; // 202
    ch.close();
}
```

---

### 4. 32 分片并发哈希表与原子零拷贝共享切片 (`ConcurrentMap` & `ArcSlice`)

- **`ConcurrentMap(K, V)`**：内置 32 个独立加锁的分片桶（`StripeCount = 32`），通过 `Wyhash`（种子 `0x12345678`）将键均匀路由至对应分片，使多线程并发读写锁竞争降低为单锁的 $1/32$；
- **`ArcSlice(T)`**：带原子引用计数头（`ref_count: std.atomic.Value(usize)`）的不可变共享切片，`clone()` 仅执行一次原子 `fetchAdd(1, .monotonic)`，最后一个持有者调用 `release()` 时自动释放底层内存。

```typescript
import std from "std";
import { ConcurrentMap, ArcSlice } from "cap:sync";

pub fn demoConcurrentMapAndArcSlice()! {
    const allocator = std.heap.page_allocator;

    // 1. 32 分片并发字典
    let cmap = ConcurrentMap(i32, i32).init(allocator);
    defer cmap.deinit();
    try cmap.put(1, 1000);
    try cmap.put(2, 2000);
    const val = cmap.get(1).?; // 1000

    // 2. 跨线程零拷贝原子引用计数切片
    let shared_bytes = try ArcSlice(u8).init(allocator, "zero-copy payload");
    let worker_view = shared_bytes.clone(); // refCount == 2
    worker_view.release();                  // refCount == 1
    shared_bytes.release();                 // refCount == 0 -> 自动释放堆内存
}
```

---

## 三、 与 Bun (`src/threading/`) 全面对照表

| 并发组件 | Bun (`src/threading/*.cp`) | CP 标准库 (`cap:sync` / `cap:threading`) | 核心并发特性 |
| :--- | :--- | :--- | :--- |
| **用户态系统锁原语** | `bun.threading.Futex` | `Futex`, `Deadline` | Linux `futex` 私有快路径 + 单调时钟超时 |
| **跨平台 3 态互斥锁** | `bun.Mutex` | `Mutex` (`lock`, `tryLock`, `withLock`) | Debug 死锁检测 + Release 零开销位操作 |
| **条件变量** | `bun.threading.Condition` | `Condition` (`wait`, `timedWait`, `signal`) | 双 16 位状态字 + `epoch` 唤醒代次 |
| **并发计数屏障** | `bun.threading.WaitGroup` | `WaitGroup` (`add`, `done`, `wait`) | 原子递减 + 零竞争快路径返回 |
| **无界侵入式批次队列**| `UnboundedQueue` | `UnboundedQueue(T, .next)` | 缓存行隔离 `front`/`back` + $O(1)$ `popBatch` |
| **无锁有界环形队列** | `Channel` Ring | `MpmcQueue(T, CAP)` / `RingBuffer(T, CAP)` | Vyukov 轮次号无锁 MPMC |
| **32 分片并发哈希表** | `ConcurrentMap` | `ConcurrentMap(K, V)` | Wyhash 分片路由，支持整型与字符串键 |
| **原子引用计数切片** | `ArcSlice` | `ArcSlice(T)` | 零拷贝跨 Actor/线程只读载荷共享 |
| **工作窃取线程池** | `ThreadPool` / `WorkPool` | `WorkPool`, `TaskDesc`, `Guarded<T>` | 线程本地缓冲 + 全局队列 + 远程窃取 |

---

## 四、 上游 Bun `threading` 并发契约与安全边界

1. **`Mutex` 不可重入与死锁检测**：
   - `Mutex` 为非递归 3 态用户态 Futex 锁（非竞争路径 0 系统调用）；同一线程重复 `lock()` 会触发死锁（Debug 模式下自动检测并拦截），优先使用 `withLock` 或 `Guarded<T>` 保证异常路径下确定性解锁。
2. **`Condition.wait` 虚假唤醒防护**：
   - 遵循 POSIX / Futex 条件变量契约：`Condition.wait` / `timedWait` 必须置于 `while (!predicate)` 循环中调用，不可使用单次 `if`，以防系统虚假唤醒或多线程竞争消耗信号。
3. **`MpmcQueue` 2 的幂容量与 `ArcSlice` 生命周期**：
   - `MpmcQueue` / `RingBuffer` 依赖 2 的幂容量与 Vyukov 序列号实现无锁多生产者多消费者通信；`ArcSlice<T>` 跨线程传递只读大数据块时每次 `clone()` 必须配对一次 `release()`，引用计数归零时自动释放底层堆内存。
4. **`WaitGroup` 计数守卫**：
   - 务必在派生工作线程之前调用 `wg.add(1)`（而非在子线程内部调用），避免主线程 `wg.wait()` 在子线程尚未执行 `add` 前提前返回。
