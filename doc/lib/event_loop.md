# `cap:event_loop` — 高性能原生事件循环与并发调度标准库

`cap:event_loop` 模块是 CP 语言官方一等公民标准库中的事件驱动核心支柱，全量重构吸收自 Bun 生产级事件循环架构（`/data/dev/bun/src/event_loop/`）。本模块提供零内存分配侵入式二叉/配对最小堆定时器、无锁跨线程任务调度队列、I/O 延迟微任务队列、通用回调包装以及统一的事件循环驱动引擎。

---

## 模块设计理念

1. **侵入式零分配最小堆定时器 (`TimerHeap` & `Timer`)**：
   - 采用配对堆（Pairing Heap）拓扑结构，定时器节点（`Timer`）自包含堆内部指针（`child` / `prev` / `next`）；
   - 定时器的插入（`insert`）具备均摊 $O(1)$ 复杂度，最小项探测（`peek`）具备 $O(1)$ 复杂度，移除（`remove`）与弹出最小项（`deleteMin`）具备均摊 $O(\log N)$ 复杂度；
   - 插入与删除过程完全**零堆内存分配**，杜绝高频 `setTimeout` / `setInterval` 引起的内存碎片与垃圾。
2. **轻量无锁跨线程任务调度 (`ConcurrentTaskQueue` & `SpinLock`)**：
   - 针对多写者/单读者（MPSC）跨线程任务投递场景，提供原子锁保护的高速侵入式队列；
   - 支持批量排空（`drainAll`），一次性将外部工作线程投递的所有就绪任务转移至事件循环主线程批量执行，消除线程上下文频繁切换损耗。
3. **延迟微任务队列与自动刷新防御 (`DeferredTaskQueue` & `AutoFlusher`)**：
   - 针对流式套接字与文件 I/O 的频繁写入场景，在微任务阶段合并刷新，避免单次小报文多次系统调用；
   - 自动任务去重机制，防止同一个套接字在单次事件循环迭代中重复注册刷新回调。
4. **统一事件循环抽象 (`EventLoop`)**：
   - 融合任务调度、微任务排空、定时器触发三阶段经典事件驱动流水线；
   - 内置零分配预置槽位（`MAX_LOOP_TIMERS = 64`, `MAX_LOOP_TASKS = 64`），开箱即用无需手动管理任务结构体生命周期。

---

## 核心组件一览

| 组件 | 说明 | 适用场景 |
| :--- | :--- | :--- |
| `Timer` | 侵入式定时器节点，支持一次性与周期性重复触发 | `setTimeout`、`setInterval`、超时检测、心跳探活 |
| `TimerHeap` | 配对最小堆定时器管理器，支持 $O(1)$ 探测与 $O(\log N)$ 维护 | 大量高并发定时器聚合调度与顺序触发 |
| `TimeSpec` | 纳秒/毫秒精度的时间戳与比较结构体 | 跨平台时间间隔与超时比较 |
| `ConcurrentTask` | 16 字节轻量跨线程任务描述符 | 线程池与工作线程向主事件循环派发回调 |
| `ConcurrentTaskQueue` | 线程安全的侵入式任务就绪队列 | 跨线程就绪队列、Actor 消息信箱调度 |
| `DeferredTaskQueue` | 基于上下文去重的延迟微任务队列 | 套接字写入自动合并（AutoFlush）、批量微任务排空 |
| `AutoFlusher` | 自动刷新标记器与状态守卫 | 网络连接缓冲区防重复刷新调度 |
| `AnyTask` | 类型擦除的单指针回调通用封装 | 泛型/多态异步任务传递 |
| `AnyTaskWithExtraContext` | 携带附加环境上下文的双指针回调封装 | 带环境句柄或父级引用的任务派发 |
| `ManagedTask` | 支持中途取消（`cancel`）的一次性动态任务 | 可撤回任务调度、延迟执行拦截 |
| `EventLoop` | 统一事件循环驱动核，协调定时器、任务队列与驱动生命周期 | 异步服务主循环、CLI 工具等待循环、Actor 调度循环 |

---

## API 使用示例

### 1. 高精度侵入式定时器堆 (`TimerHeap` & `Timer`)

```typescript
import { Timer, TimerHeap, TimerState } from "cap:event_loop";

// 初始化配对最小堆
let heap = TimerHeap.init();

// 创建侵入式定时器
let timer1 = Timer.init(100, (t) => {
    // 100ms 后触发
}, null);

let timer2 = Timer.init(50, (t) => {
    // 50ms 后触发（优先到期）
}, null);

heap.insert(&timer1);
heap.insert(&timer2);

// O(1) 查看最近到期定时器（timer2）
if (earliest := heap.peek()) {
    // earliest.expires_at_ms 为 50ms 后
}

// 触发当前时间之前已到期的所有定时器
let now_ms = heap.nextTimeoutMs(0);
let fired_count = heap.fireExpired(1000); // 触发所有到期定时器
```

---

### 2. 延迟微任务去重队列 (`DeferredTaskQueue`)

常用于网络服务端在一次事件循环周期内合并发送多个小包：

```typescript
import { DeferredTaskQueue } from "cap:event_loop";

let queue = DeferredTaskQueue.init();

fn flushSocket(ctx: *anyopaque): bool {
    // 执行批量 I/O 刷新
    // 返回 false 表示本轮处理完毕，从队列中移出
    // 返回 true 表示缓冲区未发完，下一轮继续排队
    return false;
}

// 注册延迟任务（如果 ctx 已存在则忽略，实现天然去重）
let registered = queue.postTask(socket_ptr, flushSocket);

// 在微任务周期执行所有待刷新的套接字
let executed = queue.run();
```

---

### 3. 跨线程并发任务队列 (`ConcurrentTaskQueue`)

工作线程通过原子无锁/自旋锁通道向事件循环安全投递任务：

```typescript
import { ConcurrentTask, ConcurrentTaskQueue } from "cap:event_loop";

let queue = ConcurrentTaskQueue.init();

// 外部工作线程投递任务
let task = ConcurrentTask.init((ctx) => {
    // 在主事件循环中执行回调
}, ctx_ptr, false);

queue.push(&task);

// 事件循环主线程批量排空并执行
let processed = queue.drainAll();
```

---

### 4. 完整事件循环 (`EventLoop`)

```typescript
import { EventLoop } from "cap:event_loop";

let loop = EventLoop.init();

// 1. 注册一次性延时回调
let timeout = loop.setTimeout(100, (t) => {
    // 100ms 超时回调
}, null);

// 2. 注册周期性心跳定时器
let interval = loop.setInterval(1000, (t) => {
    // 每秒执行一次
}, null);

// 3. 投递并发任务与延迟微任务
* = loop.postTask((ctx) => {
    // 异步任务
}, null);

// 4. 单步执行一次完整流水线（排空并发任务 -> 微任务 -> 触发定时器）
let processed = loop.tick();

// 5. 持续驱动直至完成条件满足
fn isComplete(ctx: (*anyopaque)?): bool {
    return false; // 自定义结束判断
}
* = loop.runUntilDone(isComplete, null);
```

---

## 性能与架构考量

- **无内存分配设计**：`Timer` 与 `ConcurrentTask` 均采用链表/树形侵入式指针，避免每次定时任务和跨线程投递都经历分配器（Allocator）交互；
- **自旋与原子保护**：`SpinLock` 内部使用 `std.atomic.Value(u32)` 配合 `std.atomic.spinLoopHint()`，在 Linux/x86/ARM/macOS 上均实现零额外系统调用损耗；
- **状态机与重入安全**：延迟任务队列在 `run()` 迭代过程中支持任务就地取消（`unregisterTask`）与按需重入重复排队。
