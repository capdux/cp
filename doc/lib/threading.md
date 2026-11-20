# `cap:threading` 标准库文档

## 1. 概述

`cap:threading`（源码位于 `lib/threading/`）是纯 CP 实现的工业级并发同步原语标准库，完全基于 `std.atomic.Value` 与 `std.Thread.Futex` 构建，零堆分配、无 `std.posix` 或外部平台胶水耦合。支持通过 `import { ... } from "cap:threading";` 或 `import cap from "cap";`（`cap.threading.*`）调用。

## 2. 核心类型

| 类型 | 定位 | 适用场景 |
| :--- | :--- | :--- |
| `ResetEvent` | 手动重置事件标志 | 跨线程状态通知、一次性或可复用唤醒信号 |
| `WaitGroup` | 多任务完成汇聚器 | 扇出（Fan-out）并发任务等待全部完成，防负计数下溢 |
| `CountDownLatch` | 倒计时同步闩 | 固定数量事件/线程就绪后的单次放行 |
| `Semaphore` | 计数信号量 | 限制同时访问共享资源的最大并发线程数 |
| `SpinLock` | 自适应自旋锁 | 极短临界区低延迟互斥（先自旋后让出 CPU） |
| `OnceFlag` | 并发单次初始化器 | 多线程竞争下保证初始化回调仅执行一次且阻塞等待完成 |
| `Barrier` | 可复用栅栏 | N 个线程多轮阶段同步，全部到达后同时放行 |

## 3. API 速查

- **`ResetEvent`**：
  - `ResetEvent.init(): ResetEvent`
  - `set(*_)`：置位并唤醒所有等待线程
  - `reset(*_)`：重置为未触发状态
  - `isSet(*_): bool`：查询当前是否已置位
  - `wait(*_)`：阻塞直到事件置位
- **`WaitGroup`**：
  - `WaitGroup.init(): WaitGroup`
  - `add(*_, delta: usize)`：增加待完成计数
  - `done(*_)`：完成计数减 1（计数为 0 时安全忽略，防下溢），归零时唤醒等待者
  - `wait(*_)`：阻塞直到计数归零
  - `count(*_): usize`：读取当前剩余计数
- **`CountDownLatch`**：
  - `CountDownLatch.init(count: usize): CountDownLatch`
  - `countDown(*_)`：倒计时减 1（归零后安全幂等）
  - `wait(*_)`：阻塞直到计数归零
  - `remaining(*_): usize`：读取当前剩余计数
- **`Semaphore`**：
  - `Semaphore.init(permits: usize): Semaphore`
  - `acquire(*_)`：获取 1 个许可，无可用许可时阻塞
  - `tryAcquire(*_): bool`：非阻塞尝试获取 1 个许可
  - `post(*_)`：归还 1 个许可并唤醒等待线程
  - `availablePermits(*_): usize`：查询当前可用许可数
- **`SpinLock`**：
  - `SpinLock.init(): SpinLock`
  - `lock(*_)`：自适应自旋加锁
  - `tryLock(*_): bool`：非阻塞尝试加锁
  - `unlock(*_)`：释放锁
  - `isHeld(*_): bool`：查询当前是否处于锁定状态
- **`OnceFlag`**：
  - `OnceFlag.init(): OnceFlag`
  - `callOnce(*_, func: *const fn () void)`：保证 `func` 仅执行一次，其余线程等待其完成
  - `isDone(*_): bool`：查询初始化是否已完成
- **`Barrier`**：
  - `Barrier.init(parties: usize): Barrier`
  - `wait(*_)`：等待 `parties` 个线程全部到达后同时放行，支持多轮复用

## 4. 示例

```cp
import { WaitGroup, SpinLock } from "cap:threading";

const std = @import("std");

struct SharedState {
    lock: SpinLock,
    wg: WaitGroup,
    total: usize,
}

fn worker(state: *SharedState) {
    defer state.wg.done();
    state.lock.lock();
    state.total += 1;
    state.lock.unlock();
}

pub fn runExample() {
    let state = SharedState{
        .lock = SpinLock.init(),
        .wg = WaitGroup.init(),
        .total = 0,
    };
    state.wg.add(2);
    const t1 = std.Thread.spawn(.{}, worker, .{&state}) catch unreachable;
    const t2 = std.Thread.spawn(.{}, worker, .{&state}) catch unreachable;
    state.wg.wait();
    t1.join();
    t2.join();
}
```
