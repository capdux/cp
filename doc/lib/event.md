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
| **事件循环延迟监控器** | `runtime/timer/EventLoopDelayMonitor.cp` | `EventLoopDelayMonitor`, `DelayHistogram`, `DelayMonitorError` | 64 桶 2^k 纳秒对数直方图，零分配统计 P50/P95/P99/Max 与调度抖动 |

### 6. 事件循环延迟监控器 (`EventLoopDelayMonitor` & `DelayHistogram`)

吸收自 `/data/dev/bun/src/runtime/timer/EventLoopDelayMonitor.cp`（对应 `node:perf_hooks` 的 `monitorEventLoopDelay` 能力，参考 `/data/dev/bun/docs/runtime/nodejs-compat.mdx` 与 `/data/dev/bun/docs/runtime/workers.mdx`）：

- **导出符号**：`DELAY_HISTOGRAM_BUCKETS` (`64`)、`DELAY_DEFAULT_RESOLUTION_MS` (`10`)、`DELAY_NS_PER_MS` (`1_000_000`)、`DelayMonitorError`（`InvalidResolution`/`InvalidPercentile`）、`DelayHistogram`（`init`/`initWithMax`/`bucketIndex`/`bucketUpperBoundNs`/`record`/`reset`/`meanNs`/`percentile`）、`EventLoopDelayMonitor`（`init`/`initWithMax`/`expectedIntervalNs`/`enable`/`disable`/`isEnabled`/`shouldFire`/`onFire`/`reset`）；
- **实战最佳实践**：
  - 默认采样分辨率 `10ms`（`DELAY_DEFAULT_RESOLUTION_MS`），过小的分辨率（如 1ms）会增加定时器唤醒频率；首次 `onFire` 仅建立基准时间戳并返回 `null`，从第二次触发开始计算 `actual_ns - expected_ns` 延迟；
  - 结合 `EventLoop` 或工作线程心跳，在每次事件循环推进时检查 `shouldFire(now_ns)` 并调用 `onFire(now_ns)`，通过 `histogram.percentile(99.0)` 与 `histogram.max_ns` 定位阻塞主线程的长同步任务；
  - 若需要有界告警统计，使用 `initWithMax(resolution_ms, max_recordable_ns)` 自动截断极端离群值并通过 `histogram.exceeds_count` 追踪超限次数。

```cp
import { EventLoopDelayMonitor } from "cap:event";

let mon = EventLoopDelayMonitor.init(10)?; // 10ms 分辨率
* = mon.enable(1_000_000_000);             // t0 = 1.000s，计划在 1.010s 触发
* = mon.onFire(1_010_000_000);             // 首次触发建立基线，返回 null
const d1 = mon.onFire(1_025_000_000);      // 间隔 15ms -> 记录 5ms (5_000_000ns) 延迟
const p99 = mon.histogram.percentile(99.0)?;
* = d1; * = p99;
```

### 3.8 同步阻塞子事件循环与超时隔离状态机 (`spawnSyncLoop.cp`)

吸收自 `/data/dev/bun/src/event_loop/SpawnSyncEventLoop.cp`：

- **导出符号**：`SpawnSyncTickState`（`Running`/`Completed`/`Timeout`/`Cancelled`）、`SpawnSyncLoopError`（`NotPrepared`/`AlreadyPrepared`/`InvalidSubHandle`）、`SpawnSyncDeadline`（`infinite`/`fromTimeout`/`isInfinite`/`deadlineMs`/`remainingMs`/`isExpired`/`elapsedMs`）、`SpawnSyncLoop`（`init`/`prepare`/`markCompleted`/`cancel`/`state`/`tick`/`remainingTimeoutMs`/`elapsedMs`/`cleanup`）；
- **核心设计**：在执行同步阻塞等待（如 `spawnSync`）时，`prepare(&active_handle, start_ms, timeout_ms)` 暂存并替换父事件循环句柄以防止事件重入污染，通过确定性时间戳 `tick(now_ms)` 推进超时/完成/取消状态机，结束后由 `cleanup(&active_handle)` 严格还原父事件循环句柄。

```cp
import { SpawnSyncLoop, SpawnSyncTickState } from "cap:event";

let active_loop_id: u64 = 1; // 父事件循环句柄
let sub = SpawnSyncLoop.init(42);
const saved = sub.prepare(&active_loop_id, 1000, 500)?; // active_loop_id 切换为 42
* = sub.tick(1200); // SpawnSyncTickState.Running，剩余 300ms
sub.markCompleted();
const st = sub.tick(1250); // SpawnSyncTickState.Completed
const restored = sub.cleanup(&active_loop_id)?; // active_loop_id 还原为 1
* = saved; * = st; * = restored;
```

### 3.9 Node/Bun `TimeoutObject` 生命周期与 `ref`/`unref`/`refresh` 调度表 (`timeoutObject.cp`)

吸收自 `/data/dev/bun/src/runtime/timer/TimeoutObject.cp`：

- **导出符号**：`TIMEOUT_MAX_INTERVAL_MS` (`2_147_483_647`)、`TIMEOUT_REGISTRY_MAX` (`64`)、`TimeoutKind`（`SetTimeout` / `SetInterval`）、`TimeoutState`（`Active` / `Fired` / `Cancelled` / `Destroyed`）、`TimeoutObjectError`（`AlreadyDestroyed` / `RegistryFull` / `NotFound`）、`clampTimeoutIntervalMs`、`TimeoutObject`（`init` / `toPrimitive` / `doRef` / `doUnref` / `hasRef` / `ref` / `deref` / `doRefresh` / `isDue` / `fire` / `cancel` / `finalize`）、`TimeoutRegistry`（`init` / `scheduleTimeout` / `scheduleInterval` / `get` / `clear` / `refresh` / `setRef` / `activeRefCount` / `tickDue`）；
- **核心语义**：严格遵循 Node.js / Bun `u31` 超时钳位规范（`<= 0` 或 `> 2^31 - 1` 钳位至 `1ms`），支持事件循环保活 `ref()` / `unref()`、一次性定时器触发后 `refresh()` 重新激活、以及 64 槽位零分配批量 `tickDue(now_ms)` 触发。

```cp
import { TimeoutRegistry } from "cap:event";

let reg = TimeoutRegistry.init();
const id1 = reg.scheduleTimeout(1000, 100)?;
const id2 = reg.scheduleInterval(1000, 50)?;
const fired = reg.tickDue(1060); // 仅 id2 到期触发 -> fired == 1
* = id1; * = id2; * = fired;
```

### 3.10 Node/Bun `ImmediateObject` 生命周期与轮次隔离 FIFO 队列 (`immediateObject.cp`)

吸收自 `/data/dev/bun/src/runtime/timer/ImmediateObject.cp`：

- **导出符号**：`IMMEDIATE_QUEUE_MAX` (`64`)、`ImmediateState`（`Active` / `Fired` / `Cancelled` / `Destroyed`）、`ImmediateObjectError`（`QueueFull` / `NotFound` / `AlreadyDestroyed`）、`ImmediateObject`（`init` / `toPrimitive` / `doRef` / `doUnref` / `hasRef` / `ref` / `deref` / `runImmediateTask` / `dispose` / `finalize` / `getDestroyed` / `isActive`）、`ImmediateQueue`（`init` / `schedule` / `get` / `clear` / `setRef` / `activeCount` / `activeRefCount` / `drainTurn`）；
- **核心语义**：完整建模 `setImmediate` / `clearImmediate` 生命周期与事件循环保活引用（`ref`/`unref`），`drainTurn(out_ids)` 通过 `current_turn` 快照隔离本轮与回调内新投递的立即任务，避免递归 `setImmediate` 饿死事件循环。

```cp
import { ImmediateQueue } from "cap:event";

let q = ImmediateQueue.init();
const id1 = q.schedule()?;
const id2 = q.schedule()?;
* = q.clear(id1);
let out: [8]i32 = [0; 8];
const n = q.drainTurn(out[0..]); // n == 1, out[0] == id2
* = n;
```

### 3.11 高精度浮点秒级 `RunLoopTimer` 与 Imminent 零延迟快路径 (`runLoopTimer.cp`)

吸收自 `/data/dev/bun/src/runtime/timer/WTFTimer.cp`：

- **导出符号**：`RUN_LOOP_TIMER_MAX_SECONDS` (`2_147_483_647.0`)、`RUN_LOOP_TIMER_NS_PER_SEC` (`1_000_000_000`)、`RunLoopTimerState`（`Cancelled` / `Imminent` / `Active` / `Fired`）、`RunLoopTimeSpec`（`init` / `addSeconds` / `compare` / `isReached`）、`RunLoopTimer`（`init` / `hasValidContext` / `update` / `isImminent` / `isDue` / `runIfImminent` / `fire` / `cancel`）；
- **核心语义**：支持 `f64` 秒级调度与 `i32` 上限钳位、`timespec` 纳秒进位规格化、`seconds <= 0` 立即抢占 `Imminent` 零延迟快速通道（用于 GC/微任务紧急调度），以及重复触发与上下文受保护取消。

```cp
import { RunLoopTimeSpec, RunLoopTimer } from "cap:event";

let t = RunLoopTimer.init(1);
const now = RunLoopTimeSpec.init(100, 750_000_000);
t.update(now, 0.5, false); // next == (101s, 250_000_000ns)
const fired = t.fire(RunLoopTimeSpec.init(101, 250_000_000)); // true
* = fired;
```

### 3.12 定时器公共内核与 32-bit 紧凑标志位 (`timerInternals.cp`)

吸收自 `/data/dev/bun/src/runtime/timer/TimerObjectInternals.cp`：

- **导出符号**：`TIMER_INTERNALS_MAX_EPOCH` (`33_554_431`)、`TIMER_INTERNALS_MAX_INTERVAL_MS` (`2_147_483_647`)、`TimerInternalsKind`（`SetTimeout` / `SetInterval` / `SetImmediate`）、`TimerInternalsState`（`Active` / `Fired` / `Cancelled` / `Destroyed`）、`TimerObjectFlags`（`toU32` / `fromU32`）、`TimerObjectInternals`（`init` / `asyncID` / `toPrimitive` / `doRef` / `doUnref` / `hasRef` / `shouldFire` / `beginFire` / `endFire` / `reschedule` / `cancel` / `finalize` / `getDestroyed`）；
- **核心语义**：提供 `TimeoutObject` 与 `ImmediateObject` 共享的 32-bit 位域标志位编码（含 `u25 epoch`、`kind`、`has_cleared_timer`、`is_keeping_event_loop_alive`、`has_accessed_primitive`、`has_js_ref`、`in_callback`）、64-bit `asyncID()` 追踪标识、`generation` 测试隔离代数防护以及回调重入安全的 `beginFire` / `endFire` 生命周期状态机。

```cp
import { TimerObjectInternals, TimerInternalsKind } from "cap:event";

let t = TimerObjectInternals.init(42, TimerInternalsKind.SetInterval, 100, 1, 7);
if (t.beginFire(7, true)) {
    t.endFire(); // 自动重武装为 Active
}
const aid = t.asyncID();
* = aid;
```

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

---

## 收敛子模块深度文档

- [event_loop.md](event/event_loop.md)
