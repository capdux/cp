# CP 标准库：后台任务队列与死信重试调度引擎手册 (`cap:queue` / `cap.queue`)

## 一、 架构总览与设计准则

`cap:queue` 是 CP 语言官方内置的纯原生后台任务队列与死信重试调度引擎，专为百万级 AI Agent 异步任务编排、优先级调度、精确延迟执行、Worker 租约超时回收与死信队列（DLQ）容错打造。

核心特性：
1. **四级优先级 + 同级 FIFO 调度**（`Critical > High > Normal > Low`，同优先级按单调递增 `id` 升序出队），基于二叉最小堆实现 $O(\log N)$ 就绪出队。
2. **确定性延迟队列**（`enqueueDelayed`）：注入确定性 `now_ms` 时钟驱动，无需系统线程或睡眠即可精确测试与调度延迟任务。
3. **指数退避重试与死信队列（DLQ）**：`RetryPolicy.nextDelayMs(attempt)` 提供防溢出封顶指数退避；`nack` 在达到 `max_attempts` 后自动移入 `DeadLetter`，支持 `deadLetters(alloc)` 观测与 `requeueDeadLetter` 重置 `attempts = 0` 手工重放。
4. **僵尸任务租约超时回收（Lease-Timeout Recovery）**：`dequeue(now_ms)` 自动扫描并回迁超租约 `Running` 僵尸任务（`now_ms >= lease_until_ms`），防止 Worker 失联导致任务永久丢失，并拒绝失联 Worker 的迟到 `ack`/`nack`。
5. **所有权安全**：`topic`、`payload` 与 `last_error` 字符串在队列内部独立深拷贝托管，`deinit()` 确定性全量释放，绝不悬垂。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:queue"` 与 `import cap from "cap"; cap.queue.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/queue/job.cp` | `// Source: cp/lib/queue/job.cp (Job Types, Priority Levels & Exponential Retry Policy)` | `QueueError`、`JobPriority`、`JobState`、`RetryPolicy`、`Job` |
| `lib/queue/scheduler.cp` | `// Source: cp/lib/queue/scheduler.cp (Priority, Delayed, Lease-Timeout & Dead-Letter Job Queue)` | `QueueStats`、`JobQueue` |
| `lib/queue/index.cp` | `// Source: cp/lib/queue/index.cp (Unified Exports for cap:queue)` | `cap:queue` 与 `cap.queue` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误、优先级、状态与重试策略（`job.cp`）

- **`QueueError`**：`JobNotFound`、`InvalidState`、`DuplicateId`、`QueueFull`、`OutOfMemory`
- **`JobPriority: u8`**：`Low = 0`、`Normal = 1`、`High = 2`、`Critical = 3`
  - 方法：`rank(): u8`、`static fromRank(val: u8): JobPriority?`
- **`JobState: u8`**：`Pending = 0`、`Delayed = 1`、`Running = 2`、`Completed = 3`、`DeadLetter = 4`
  - 方法：`isTerminal(): bool`
- **`RetryPolicy`**：
  - 字段：`max_attempts: u32 = 3`、`base_delay_ms: i64 = 1000`、`max_delay_ms: i64 = 60000`
  - 方法：`static default(): RetryPolicy`、`static init(max_attempts, base_delay_ms, max_delay_ms): RetryPolicy`、`nextDelayMs(attempt: u32): i64`
- **`Job`**：
  - 字段：`id: u64`、`topic: string`、`payload: string`、`priority: JobPriority`、`state: JobState`、`attempts: u32`、`max_attempts: u32`、`run_at_ms: i64`、`created_at_ms: i64`、`lease_until_ms: i64`、`last_error: string`

### 2. 调度引擎与状态统计（`scheduler.cp`）

- **`QueueStats`**：
  - 字段：`pending: usize`、`delayed: usize`、`running: usize`、`completed: usize`、`dead_letter: usize`、`total_enqueued: u64`、`total_retried: u64`
- **`JobQueue`**：
  - 构造与销毁：
    - `init(alloc: std.mem.Allocator, default_retry: RetryPolicy, lease_timeout_ms: i64): JobQueue!`
    - `initWithCapacity(alloc, default_retry, lease_timeout_ms, max_active_jobs): JobQueue!`
    - `deinit(*_)`
  - 入队：
    - `enqueue(*_, topic: string, payload: string, priority: JobPriority, now_ms: i64): u64!`
    - `enqueueDelayed(*_, topic: string, payload: string, priority: JobPriority, delay_ms: i64, now_ms: i64): u64!`
  - 调度与确认：
    - `dequeue(*_, now_ms: i64): Job?`（先回迁到期 `Delayed` 任务与超租约 `Running` 僵尸任务，再按 `priority` 降序 + `id` 升序出队并刷新 `lease_until_ms = now_ms + lease_timeout_ms`）
    - `ack(*_, job_id: u64): !`
    - `nack(*_, job_id: u64, err_msg: string, now_ms: i64): !`
  - 死信与观测：
    - `deadLetters(alloc: std.mem.Allocator): ([]Job)!`（调用方负责 `alloc.free(...)`）
    - `requeueDeadLetter(*_, job_id: u64, now_ms: i64): !`
    - `getJob(job_id: u64): Job?`
    - `stats(): QueueStats`

---

## 三、 使用示例

```typescript
import cap from "cap";
import { JobQueue, RetryPolicy, JobPriority } from "cap:queue";

const std = @import("std");

pub fn demoQueue(): ! {
    const alloc = std.heap.page_allocator;
    let q = JobQueue.init(alloc, RetryPolicy.init(3, 1000, 30000), 5000)?;
    defer q.deinit();

    const low_id = q.enqueue("email", "digest", .Low, 100)?;
    const crit_id = q.enqueue("agent.tool", "call_1", .Critical, 100)?;

    // 优先出队 Critical 任务
    if (job := q.dequeue(100)) {
        if (job.id == crit_id) {
            q.ack(job.id)?;
        }
    }

    // 失败重试与死信队列
    if (job2 := q.dequeue(110)) {
        q.nack(job2.id, "upstream timeout", 110)?;
    }

    // cap.queue 动态挂载等价用法
    let q2 = cap.queue.JobQueue.init(alloc, cap.queue.RetryPolicy.default(), 10000)?;
    defer q2.deinit();
    * = low_id;
}
```
