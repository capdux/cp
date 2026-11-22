# CP 标准库：事件发布/订阅与高性能异步事件循环统一子系统 (`cap:event` / `cap:events` / `cap:event_loop`)

> 参考来源：`/data/dev/bun/src/event_loop/MiniEventLoop.cp`, `/data/dev/bun/src/event_loop/`, `/data/dev/bun/src/js/node/events.ts` | 吸收日期：2026-11-22

## 一、 架构总览与设计哲学

`cap:event`（别名 `cap:events`、`cap:event_loop`）统一整合 CP 语言的**应用层事件发布/订阅模型**（`EventBus`、`EventEmitter`、协程信号 `Event`）与**底层高性能异步事件循环内核**（侵入式配对最小堆 `TimerHeap`、自旋锁保护跨线程并发队列 `ConcurrentTaskQueue`/`ConcurrentQueue`、去重延迟微任务队列 `DeferredTaskQueue`、I/O 自动刷盘合并器 `AutoFlusher`、跨平台 FD 轮询器 `PollEngine`、类型擦除与可取消受管任务 `AnyTask`/`AnyTaskWithExtraContext`/`ManagedTask` 与统一事件循环引擎 `EventLoop`）。完整吸收自 Bun 的 `/data/dev/bun/src/js/node/events.ts` 与 `/data/dev/bun/src/event_loop/`。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                     cap:event (cap:events / cap:event_loop) 统一事件架构               │
│                                                                                        │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐  │
│  │ 应用发布/订阅与同步原语 (emitter.cp)                                              │  │
│  │  • EventBus      — 64 槽零堆分配确定性回调总线 (on / once / off / emit)           │  │
│  │  • EventEmitter  — Node.js 风格事件发射器 (返回退订闭包支持 defer)                │  │
│  │  • Event         — 协程/任务就绪屏障 (set / reset / isSet / wait)                 │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           ▼                                            │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐  │
│  │ 统一事件循环四阶段调度引擎 (loop.cp)                                              │  │
│  │  Phase 1: ConcurrentTaskQueue.drainAll — SpinLock O(1) 整链摘除，无锁执行跨线程回调│  │
│  │  Phase 2: DeferredTaskQueue.run & AutoFlusher — 按 ctx 去重，批量刷盘/微任务      │  │
│  │  Phase 3: PollEngine.poll — 按需懒初始化 POSIX poll 文件描述符 I/O 多路复用       │  │
│  │  Phase 4: TimerHeap.fireExpired — 侵入式配对最小堆 O(1) 插入 / O(log N) 取消      │  │
│  └──────────────────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、 核心组件详解与代码示例

### 1. 确定性零堆分配发布/订阅总线 (`EventBus`) 与 `EventEmitter` / `Event`

```typescript
import { EventBus, EventEmitter, Event } from "cap:event";

struct Metrics {
    pub received: i32 = 0;
}

fn onData(payload: []const u8, ctx: (*anyopaque)?) {
    if (c := ctx) {
        let m: *Metrics = @alignCast(@ptrCast(c));
        m.received += payload.len as i32;
    }
}

pub fn demoPubSub() {
    let bus = EventBus.init();
    let metrics = Metrics{ received: 0 };

    * = bus.on("chunk", onData, @ptrCast(&metrics));
    * = bus.once("ready", onData, @ptrCast(&metrics));

    const n1 = bus.emit("chunk", "hello"); // 1
    const n2 = bus.emit("ready", "ok");    // 1 (触发后自动移除)
    * = bus.off("chunk", onData);

    // Node.js 风格 EventEmitter + 协程信号 Event
    let ee = new EventEmitter();
    const unsub = ee.on("connected", null);
    defer unsub();
    * = ee.emit("connected");

    let ready = new Event();
    ready.set();
    const ok = ready.isSet(); // true
    ready.reset();
}
```

### 2. 侵入式配对最小堆定时器 (`Timer` & `TimerHeap`)

源自 `/data/dev/bun/src/event_loop/EventLoopTimer.cp`。`Timer` 在结构体内直接嵌入 `child`/`prev`/`next` 侵入式配对堆（Pairing Heap）指针，零堆分配：
- `insert(&timer)` / `peek()` / `nextTimeoutMs(now_ms)`：严格 $O(1)$；
- `deleteMin()` / `remove(&timer)`：均摊 $O(\log N)$ 任意位置取消；
- `fireExpired(now_ms)`：批量触发所有到期定时器，并对 `interval_ms > 0` 的周期定时器自动重入堆。

```typescript
import { Timer, TimerHeap } from "cap:event";

fn onTimeout(t: *Timer) {}

pub fn demoTimerHeap() {
    let heap = TimerHeap.init();
    let t1 = Timer{ expires_at_ms: 200, callback: onTimeout };
    let t2 = Timer{ expires_at_ms: 50,  callback: onTimeout };
    heap.insert(&t1);
    heap.insert(&t2);
    * = heap.remove(&t1);            // O(log N) 取消
    const fired = heap.fireExpired(100); // 1
}
```

### 3. 跨线程并发任务队列 (`ConcurrentTask` & `ConcurrentTaskQueue` / `ConcurrentQueue`)

源自 `/data/dev/bun/src/event_loop/ConcurrentTask.cp`。使用原子 `SpinLock` 保护 O(1) `drain()` 整链摘除，随后在无锁状态下通过 `drainAll()` 或 `drainMax(limit)` 执行。

### 4. 去重延迟微任务队列与自动刷盘器 (`DeferredTaskQueue` & `AutoFlusher`)

源自 `/data/dev/bun/src/event_loop/DeferredTaskQueue.cp` 与 `AutoFlusher.cp`。同一 Tick 内按 `ctx` 指针自动幂等去重；回调返回 `true` 续期至下一轮，返回 `false` 自动移出。

```typescript
import { DeferredTaskQueue, AutoFlusher } from "cap:event";

fn onFlush(ctx: *anyopaque): bool {
    return false;
}

pub fn demoAutoFlusher() {
    let queue = DeferredTaskQueue.init();
    let flusher = AutoFlusher.init();
    let fd_ctx: i32 = 1;
    * = flusher.registerDeferred(&queue, @ptrCast(&fd_ctx), onFlush);
    * = queue.run();
    flusher.markUnregistered();
}
```

### 5. 通用任务封装 (`AnyTask` / `AnyTaskWithExtraContext` / `ManagedTask`) 与统一事件循环 (`EventLoop`)

- `ManagedTask` 同时支持栈分配值语义 `ManagedTask.init(ctx, cb)` 与堆分配指针语义 `ManagedTask.create(ctx, cb)`，内置 `is_heap_allocated` 守卫杜绝栈误释放与重复释放。
- `EventLoop` 内置 64 槽位零堆分配定时器池与任务池，按需懒初始化 `PollEngine` 支持 `watch(fd, events, cb, ctx)` / `unwatch(fd)`：

```typescript
import { EventLoop, Timer } from "cap:event";

struct AppState { ticks: i32 = 0 }

fn onTimer(t: *Timer) {
    if (c := t.ctx) {
        let s: *AppState = @alignCast(@ptrCast(c));
        s.ticks += 1;
    }
}

pub fn demoEventLoop() {
    let loop = EventLoop.init();
    let state = AppState{ ticks: 0 };
    * = loop.setTimeout(0, onTimer, @ptrCast(&state));
    const processed = loop.tick();
}
```

---

## 三、 与 Node.js `events` / Bun `src/event_loop/` 对照表

| 组件 | Bun / Node.js 来源 | CP (`cap:event` / `cap:event_loop`) | 关键性能特征 |
| :--- | :--- | :--- | :--- |
| **发布/订阅总线** | `js/node/events.ts` | `EventBus`, `EventEmitter`, `Event` | 64 槽零堆分配 + 退订闭包适配 `defer` |
| **侵入式配对堆定时器** | `event_loop/EventLoopTimer.cp` | `Timer`, `TimerHeap`, `TimeSpec`, `TimerState` | $O(1)$ 插入/窥视，$O(\log N)$ 取消 |
| **跨线程并发任务队列** | `event_loop/ConcurrentTask.cp` | `SpinLock`, `ConcurrentTask`, `ConcurrentTaskQueue` (`ConcurrentQueue`) | $O(1)$ 整链原子摘除（`drainAll` / `drainMax`） |
| **去重延迟微任务队列** | `event_loop/DeferredTaskQueue.cp` | `DeferredTaskQueue`, `DeferredTaskEntry` | 128 槽零分配、按 `ctx` 幂等去重、支持 `bool` 续期 |
| **自动刷盘合并器** | `event_loop/AutoFlusher.cp` | `AutoFlusher` | 单 Tick 聚合多次小块写缓冲 Flush |
| **FD 多路复用器** | `event_loop/AnyEventLoop.cp` | `PollEngine`, `PollEvents`, `FilePollRegistration` | 懒初始化 POSIX `poll` 监听 |
| **类型擦除与受管任务** | `event_loop/AnyTask.cp`, `ManagedTask.cp` | `AnyTask`, `AnyTaskWithExtraContext`, `ManagedTask` | 栈/堆双模安全释放，支持 `cancel()` |
| **统一事件循环内核** | `event_loop/MiniEventLoop.cp` | `EventLoop` | 零堆碎片 64 槽定时/任务池 + 四阶段 `tick` |

---

## 四、 上游 Bun 事件循环行为契约与并发边界

1. **四阶段确定性调度顺序（`EventLoop.tick`）**：
   - 继承自 Bun `MiniEventLoop`：单次 `tick()` 严格依次执行（1）`ConcurrentTaskQueue.drainAll` 跨线程任务摘除、（2）`DeferredTaskQueue.run` 与 `AutoFlusher` 微任务/刷盘合并、（3）`PollEngine.poll` I/O 就绪派发、（4）`TimerHeap.fireExpired` 到期定时器触发；回调中重新投递的任务不会导致当前阶段无限饿死。
2. **跨线程无锁执行窗口（`ConcurrentTaskQueue`）**：
   - `drainAll` / `drainMax` 仅在摘除链表头尾指针期间持有 `SpinLock`，实际执行用户回调时**完全释放自旋锁**，防止回调内再次入队引发自旋死锁。
3. **侵入式定时器生命周期（`Timer` / `TimerHeap`）**：
   - `Timer` 节点采用侵入式指针嵌入 `TimerHeap`，在定时器仍处于活跃态（未触发且未取消）期间，调用方不得移动或释放该 `Timer` 所在内存地址；提前销毁前须显式取消。
4. **有界容量防洪泛**：
   - `EventLoop`（`MAX_LOOP_TIMERS` / `MAX_LOOP_TASKS`）、`DeferredTaskQueue`（`MAX_DEFERRED_TASKS`）与 `EventBus` 均为固定槽位零堆分配结构，满载时返回明确失败状态而非无限增长内存。
