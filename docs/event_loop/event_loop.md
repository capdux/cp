# CP 标准库：高性能异步事件循环与定时器调度子系统使用手册 (`cap:event_loop`)

## 一、 架构总览与设计哲学

`cap:event_loop` 是 CP 语言官方内置的**侵入式配对最小堆定时器（`TimerHeap`）、无锁/自旋跨线程并发任务队列（`ConcurrentTaskQueue`）、去重延迟微任务队列（`DeferredTaskQueue`）、I/O 自动刷盘器（`AutoFlusher`）与统一事件循环调度器（`EventLoop`）**标准库。完整吸收自 Bun（`/data/dev/bun/src/event_loop/`），作为 CP 异步运行时与高性能网络服务的驱动心脏。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                            cap:event_loop 事件循环三阶段调度引擎                        │
│                                                                                        │
│   ┌────────────────────────────────────────────────────────────────────────────────┐   │
│   │ Phase 1: 跨线程并发任务排空 (ConcurrentTaskQueue.drainAll)                     │   │
│   │   • 原子 SpinLock 保护 O(1) 整链摘除 (drain)，零锁持有执行工作线程投递的回调   │   │
│   └───────────────────────────────────────┬────────────────────────────────────────┘   │
│                                           ▼                                            │
│   ┌────────────────────────────────────────────────────────────────────────────────┐   │
│   │ Phase 2: 延迟微任务与自动刷盘 (DeferredTaskQueue.run & AutoFlusher)            │   │
│   │   • 按上下文指针自动去重，批量聚合 Socket / 文件缓冲区的尾端 Flush 操作        │   │
│   └───────────────────────────────────────┬────────────────────────────────────────┘   │
│                                           ▼                                            │
│   ┌────────────────────────────────────────────────────────────────────────────────┐   │
│   │ Phase 3: 侵入式最小堆定时器触发 (TimerHeap.fireExpired)                        │   │
│   │   • O(1) peek / insert，O(log N) deleteMin / remove，支持 Interval 自动重调度  │   │
│   └────────────────────────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、 五大核心组件详解与代码示例

### 1. 侵入式配对最小堆定时器 (`Timer` & `TimerHeap`)

源自 `/data/dev/bun/src/event_loop/EventLoopTimer.cp`。传统的数组二叉堆在任意取消一个中间定时器时需要维护额外的索引映射或堆重排；`Timer` 直接在结构体内嵌入 `child`、`prev`、`next` 三个侵入式配对堆（Pairing Heap）指针：
- **`insert(&timer)`**：严格 $O(1)$ 时间复杂度将定时器合并入堆根；
- **`peek()` / `nextTimeoutMs(now_ms)`**：严格 $O(1)$ 获取最近到期时间，供底层 `epoll_wait` / `kqueue` 计算阻塞超时；
- **`deleteMin()` / `remove(&timer)`**：均摊 $O(\log N)$ 弹出最小元或直接通过侵入式指针摘除任意已取消的定时器，零堆内存分配；
- **`fireExpired(now_ms)`**：批量触发所有 `expires_at_ms <= now_ms` 的到期定时器，对周期性定时器（`interval_ms > 0`）自动重新计算下次到期时间并重入堆。

```typescript
import { Timer, TimerHeap, TimerState } from "cap:event_loop";

fn onTimeout(t: *Timer) {
    // 处理定时器到期回调
}

pub fn demoTimerHeap() {
    let heap = TimerHeap.init();

    let t1 = Timer{ expires_at_ms: 200, callback: onTimeout };
    let t2 = Timer{ expires_at_ms: 50,  callback: onTimeout };
    let t3 = Timer{ expires_at_ms: 120, callback: onTimeout };

    heap.insert(&t1);
    heap.insert(&t2);
    heap.insert(&t3);

    // 堆顶恒为最早到期的 t2 (50ms)
    const earliest = heap.peek().?.expires_at_ms; // 50

    // 随时 O(log N) 取消尚未到期的 t1
    heap.remove(&t1);

    // 推进时钟至 150ms，一次性触发 t2 (50ms) 与 t3 (120ms)
    const fired_count = heap.fireExpired(150); // 2
}
```

---

### 2. 跨线程并发任务队列 (`ConcurrentTask` & `ConcurrentTaskQueue`)

源自 `/data/dev/bun/src/event_loop/ConcurrentTask.cp`。用于线程池工作线程（如异步 DNS、文件 I/O、密码学重计算）向主事件循环线程安全投递完成通知：
- `push(&task)` 与 `drain()` 由极轻量原子自旋锁 `SpinLock` 保护；
- 主循环调用 `drainAll()` 时，仅需一次指针交换即可将整条单向任务链表在 $O(1)$ 时间内原子摘除至局部变量，随后在**完全无锁**状态下顺序执行所有回调，彻底消除锁竞争阻塞。

```typescript
import { ConcurrentTask, ConcurrentTaskQueue } from "cap:event_loop";

fn workerCallback(ctx: (*anyopaque)?) {
    // 在主事件循环线程上安全执行
}

pub fn demoConcurrentQueue() {
    let queue = ConcurrentTaskQueue.init();
    let task = ConcurrentTask.init(workerCallback, null, false);

    queue.push(&task);
    const executed = queue.drainAll(); // 1
}
```

---

### 3. 去重延迟微任务队列与自动刷盘器 (`DeferredTaskQueue` & `AutoFlusher`)

源自 `/data/dev/bun/src/event_loop/DeferredTaskQueue.cp` 与 `AutoFlusher.cp`。在一次事件循环 Tick 内，同一个 Socket 可能会被业务代码连续调用数十次小块 `write()`。通过 `AutoFlusher.registerDeferred(&queue, ctx, flushFn)`：
- 同一 Tick 内对同一 `ctx` 的重复注册被自动幂等忽略；
- 当本轮并发任务执行完毕后，`DeferredTaskQueue.run()` 统一触发一次批量刷盘；若回调返回 `true` 则保留至下一轮 Tick 继续刷盘，返回 `false` 则自动移出队列。

```typescript
import { DeferredTaskQueue, AutoFlusher } from "cap:event_loop";

fn onFlush(ctx: *anyopaque): bool {
    return false; // 返回 false 表示本次已全部刷完，无需下一轮重复执行
}

pub fn demoAutoFlusher() {
    let queue = DeferredTaskQueue.init();
    let flusher = AutoFlusher.init();
    let dummy_socket: i32 = 1;

    // 注册自动刷盘任务（重复调用会被幂等去重）
    const ok1 = flusher.registerDeferred(&queue, @ptrCast(&dummy_socket), onFlush); // true
    const ok2 = flusher.registerDeferred(&queue, @ptrCast(&dummy_socket), onFlush); // false (已注册)

    const ran = queue.run(); // 1
    flusher.markUnregistered();
}
```

---

### 4. 通用任务封装 (`AnyTask` / `AnyTaskWithExtraContext` / `ManagedTask`)

源自 `/data/dev/bun/src/event_loop/AnyTask.cp`：
- **`AnyTask`**：16 字节类型擦除闭包（`ctx` + `callback`）；
- **`AnyTaskWithExtraContext`**：支持在执行 `run(extra)` 时动态注入第二个上下文指针，且自带侵入式 `next` 链表指针；
- **`ManagedTask`**：带 `cancel()` 取消标志位的受管任务，在调用 `run()` 前若被取消则自动跳过执行。

---

### 5. 统一事件循环调度器 (`EventLoop`)

源自 `/data/dev/bun/src/event_loop/MiniEventLoop.cp`。内置 64 槽位零堆分配定时器池与任务池，直接提供与 Web / Bun 标准一致的 `setTimeout`、`setInterval`、`clearTimeout`、`clearInterval`、`postTask`、`postDeferred`、`tick`、`run` 与 `runUntilDone`：

```typescript
import { EventLoop, Timer } from "cap:event_loop";

struct AppState {
    ticks: i32 = 0,
}

fn onTimerTick(t: *Timer) {
    if (c := t.ctx) {
        let state: *AppState = @alignCast(@ptrCast(c));
        state.ticks += 1;
    }
}

pub fn demoEventLoop() {
    let loop = EventLoop.init();
    let state = AppState{ ticks: 0 };

    // 注册 0ms 立即到期定时器
    const timer_ptr = loop.setTimeout(0, onTimerTick, @ptrCast(&state));

    // 执行单轮事件循环 Tick（依次处理 ConcurrentTasks -> DeferredTasks -> ExpiredTimers）
    const processed = loop.tick();
}
```

---

## 三、 与 Bun (`src/event_loop/`) 全面对照表

| 事件循环组件 | Bun (`src/event_loop/*.cp`) | CP 标准库 (`cap:event_loop`) | 关键性能特征 |
| :--- | :--- | :--- | :--- |
| **侵入式定时器与配对堆** | `EventLoopTimer` / `TimerHeap` | `Timer`, `TimerHeap`, `TimeSpec` | $O(1)$ 插入/窥视，$O(\log N)$ 删除，零堆分配 |
| **跨线程无锁/自旋任务** | `ConcurrentTask` / `Queue` | `ConcurrentTask`, `ConcurrentTaskQueue` | $O(1)$ 整链原子摘除（`drainAll`） |
| **延迟去重微任务队列** | `DeferredTaskQueue` | `DeferredTaskQueue` | 按 `ctx` 自动去重，支持返回 `bool` 续期 |
| **流式缓冲自动刷盘器** | `AutoFlusher` | `AutoFlusher` | 防止单 Tick 内多次冗余 `write` 系统调用 |
| **类型擦除与可取消任务** | `AnyTask` / `ManagedTask` | `AnyTask`, `AnyTaskWithExtraContext`, `ManagedTask` | 16~24 字节零虚表轻量回调封装 |
| **统一事件循环内核** | `MiniEventLoop` | `EventLoop` | 内联 64 定时器/任务槽池，零堆碎片 |
