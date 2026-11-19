# CP 标准库：事件发布/订阅与事件循环驱动子系统使用手册 (`cap:event` / `cap:events`)

## 一、 架构总览与设计哲学

`cap:event`（别名 `cap:events`）统一提供 CP 语言的**事件发布/订阅模型**（`EventBus`、`EventEmitter`、协程信号 `Event`）与**底层异步事件调度原语**（`EventLoop`、`TimerHeap`、`DeferredTaskQueue`、`ConcurrentQueue`、`PollEngine`、`AnyTask`/`ManagedTask`），吸收自 Bun 的 `/data/dev/bun/src/js/node/events.ts` 与 `/data/dev/bun/src/event_loop/`。

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│                       cap:event 分层事件驱动模型                              │
│                                                                              │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │ 应用发布/订阅与同步原语 (emitter.cp)                                    │  │
│  │  • EventBus      — 64 槽零堆分配确定性回调总线 (on / once / off / emit)  │  │
│  │  • EventEmitter  — Node.js 风格事件发射器 (返回退订闭包支持 defer)      │  │
│  │  • Event         — 协程/任务就绪屏障 (set / reset / isSet / wait)       │  │
│  └───────────────────────────────────┬────────────────────────────────────┘  │
│                                      ▼                                       │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │ 统一事件循环与多源调度引擎 (loop.cp / timer.cp / file_poll.cp / ...)    │  │
│  │  • ConcurrentQueue  — 自旋锁保护的跨线程任务投递队列                    │  │
│  │  • DeferredTaskQueue— 按上下文指针自动去重的延迟微任务队列               │  │
│  │  • PollEngine       — 基于 std.posix.poll 的跨平台 FD 多路复用器        │  │
│  │  • TimerHeap        — 侵入式二叉最小堆单调时钟定时器 (setTimeout/Interval)│  │
│  │  • AnyTask / ManagedTask — 类型擦除与可取消受管任务封装                 │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、 核心 API 详解与代码示例

### 1. 确定性零堆分配发布/订阅总线 (`EventBus`)

`EventBus` 面向系统级高吞吐场景，在栈内维护 64 个监听槽位，直接按事件名精确分发强类型回调 `EventListenerFn = fn(payload: []const u8, ctx: (*anyopaque)?)`，支持 `once` 自动注销与 `off`/`removeAllListeners`：

```typescript
import { EventBus } from "cap:event";

struct Metrics {
    pub received: i32 = 0;
}

fn onData(payload: []const u8, ctx: (*anyopaque)?) {
    if (c := ctx) {
        let m: *Metrics = @alignCast(@ptrCast(c));
        m.received += payload.len as i32;
    }
}

pub fn demoEventBus() {
    let bus = EventBus.init();
    let metrics = Metrics{ received: 0 };

    * = bus.on("chunk", onData, @ptrCast(&metrics));
    * = bus.once("ready", onData, @ptrCast(&metrics));

    const n1 = bus.emit("chunk", "hello"); // 1
    const n2 = bus.emit("ready", "ok");    // 1 (触发后自动移除)
    const n3 = bus.emit("ready", "again"); // 0

    * = bus.off("chunk", onData);
    * = bus.removeAllListeners("chunk");
}
```

### 2. Node.js 风格 `EventEmitter` 与协程屏障 `Event`

```typescript
import { EventEmitter, Event } from "cap:event";

pub fn demoEmitterAndSignal() {
    let ee = new EventEmitter();
    const unsub = ee.on("connected", null);
    defer unsub(); // 离开作用域自动退订，杜绝监听器泄漏

    const has_listeners = ee.emit("connected"); // true
    const count = ee.listenerCount("connected"); // 1

    // 协程同步信号原语
    let ready = new Event();
    ready.set();
    const ok = ready.isSet(); // true
    ready.reset();
}
```

### 3. 统一事件循环与任务调度 (`EventLoop` / `TimerHeap` / `DeferredTaskQueue` / `ConcurrentQueue`)

`EventLoop.tick()` 按固定 6 阶段顺序调度：`ConcurrentQueue.drain` → `DeferredTaskQueue.run` → 计算最近定时器超时 → `PollEngine.poll` / 休眠 → `TimerHeap.drainExpired` → 尾随微任务排空：

```typescript
import { EventLoop, ManagedTask } from "cap:event";

struct LoopState {
    pub steps: i32 = 0;
}

fn onTick(ctx: *!?) {
    if (c := ctx) {
        let st: *LoopState = @alignCast(@ptrCast(c));
        st.steps += 1;
    }
}

pub fn demoEventLoop() {
    let loop = EventLoop.init();
    let state = LoopState{ steps: 0 };

    // 1. 注册 0ms 定时器、去重微任务与跨线程任务
    const t = loop.setTimeout(onTick, 0, @ptrCast(&state));
    * = loop.deferTask(onTick, @ptrCast(&state));
    loop.postTask(onTick, @ptrCast(&state));

    // 2. 驱动单次事件循环 Tick
    const handled = loop.tick(0); // >= 3, state.steps == 3
    * = loop.clearTimeout(t);

    // 3. 受管可取消任务
    let task = ManagedTask.create(@ptrCast(&state), onTick);
    task.cancel();
    task.run(); // 已取消，跳过回调并自动释放
}
```

---

## 三、 与 Node.js `events` / Bun EventLoop 对照表

| 能力 | Node.js / Bun | CP (`cap:event`) | 核心差异与优势 |
|---|---|---|---|
| 事件订阅/分发 | `new EventEmitter()` (`on`/`once`/`off`/`emit`) | `EventBus`（真实回调零堆分配）& `EventEmitter`（带退订闭包） | `EventBus` 零堆分配、无闭包 GC 压力；`EventEmitter.on` 返回退订函数适配 `defer` |
| 异步就绪屏障 | 手动封装 `Promise` + `resolve` | 内置 `Event` (`set`/`reset`/`isSet`/`wait`) | 直接集成 CP 原生任务句柄，零 Promise 对象分配 |
| 延迟微任务去重 | `queueMicrotask`（不去重） | `DeferredTaskQueue.postTask(ctx, fn)` | 按 `ctx` 指针自动去重，适配高频 I/O 批量刷盘 |
| 跨线程回调投递 | libuv `uv_async_send` | `ConcurrentQueue`（自旋锁保护侵入式链表） | 轻量无系统调用唤醒延迟，批量 `drain(limit)` |
| 定时器与 I/O 调度 | `setTimeout` / `setInterval` + libuv/epoll | `EventLoop` + `TimerHeap` + `PollEngine` | 纯 CP 实现，可独立嵌入任意自定义协议引擎与测试夹具 |
