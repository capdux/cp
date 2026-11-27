[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# p-queue

Zero-heap priority task queue with concurrency limiting, pause/resume control, batch draining, and active/pending tracking in pure CP.

- **Upstream Alignment**: Clean-room port of npm [`p-queue`](https://github.com/sindresorhus/p-queue) (`MIT`).
- **Package Version**: `1.0.0`

## Installation & Import

```bash
cap add p-queue
```

In `cap.json`:

```json
{
  "dependencies": {
    "p-queue": "^1.0.0"
  }
}
```

```typescript
import { TaskQueue, QueueOptions, QueueTask, PQ_MAX_TASKS } from "p-queue";
```

## Core API Reference

### Constants & Types

| Symbol | Definition | Description |
| :--- | :--- | :--- |
| `PQ_MAX_TASKS` | `const PQ_MAX_TASKS: usize = 64` | Fixed inline ring/array capacity per `TaskQueue` |
| `QueueTask` | `class { id: u64, priority: i32, seq: u64 }` | Queued task entry (higher `priority` first; FIFO by `seq` on tie) |
| `QueueOptions` | `class { concurrency: usize, auto_start: bool, interval_cap: usize, interval_ms: u64 }` | Queue configuration; call `QueueOptions.default()` for defaults |

### `class TaskQueue`

| Method | Signature | Description |
| :--- | :--- | :--- |
| `new` | `TaskQueue.new(max_concurrency: usize): TaskQueue` | Create an active queue with the given concurrency limit |
| `withOptions` | `TaskQueue.withOptions(opts: QueueOptions): TaskQueue` | Create a queue from `QueueOptions` (`auto_start: false` starts paused) |
| `enqueue` / `add` | `enqueue(*_, id: u64, priority: i32): bool` | Enqueue task `id` with `priority`; returns `false` if full (`PQ_MAX_TASKS`) |
| `addBatch` | `addBatch(*_, ids: &[]u64, priority: i32): usize` | Enqueue multiple task IDs at `priority`; returns number enqueued |
| `dispatchNext` | `dispatchNext(*_): QueueTask?` | Pop highest-priority task and increment `active` if allowed, else `null` |
| `drainBatch` | `drainBatch(*_, out: []QueueTask): usize` | Dispatch up to `out.len` tasks respecting concurrency & pause state |
| `markCompleted` | `markCompleted(*_): bool` | Decrement `active` and increment `completed_total`; `false` if `active == 0` |
| `peek` | `peek(): QueueTask?` | Inspect next highest-priority task without removing it |
| `remove` | `remove(*_, id: u64): bool` | Remove queued task by `id` |
| `setPriority` | `setPriority(*_, id: u64, new_priority: i32): bool` | Update priority of an already queued task |
| `pause` / `start` | `pause(*_)` / `start(*_)` | Pause or resume dispatching |
| `setConcurrency` | `setConcurrency(*_, max_concurrency: usize)` | Update maximum concurrency limit |
| `clear` / `resetStats` | `clear(*_)` / `resetStats(*_)` | Clear all pending tasks / reset `completed_total` counter |
| `size` / `activeCount` / `pending` | `size(): usize` / `activeCount(): usize` / `pending(): usize` | Pending queued count / active running count |
| `concurrency` / `completedCount` | `concurrency(): usize` / `completedCount(): u64` | Current concurrency limit / total completed count |
| `isPaused` / `isIdle` / `canDispatch` | `isPaused(): bool` / `isIdle(): bool` / `canDispatch(): bool` | Queue state predicates |
| `onEmpty` / `onIdle` / `onSizeLessThan` | `onEmpty(): bool` / `onIdle(): bool` / `onSizeLessThan(limit: usize): bool` | Synchronous readiness checks (`size == 0`, `size == 0 && active == 0`, `size < limit`) |
| `sizeByPriority` / `sizeByExactPriority` | `sizeByPriority(min_priority: i32): usize` / `sizeByExactPriority(priority: i32): usize` | Count queued tasks with `priority >= min_priority` or exact `priority` |

## Examples

### 1. Priority Dispatch & Concurrency Limiting

```typescript
import { TaskQueue } from "p-queue";

fn runScheduler(): u64 {
    let mut q = TaskQueue.new(2);
    q.add(101, 1);
    q.add(102, 10);
    q.add(103, 5);

    if (first := q.dispatchNext()) {
        // first.id == 102 (highest priority)
        q.markCompleted();
    }
    return q.completedCount();
}
```

### 2. Paused Startup, Batch Enqueue & Batch Drain

```typescript
import { TaskQueue, QueueOptions, QueueTask } from "p-queue";

fn batchDispatch(): usize {
    let mut opts = QueueOptions.default();
    opts.concurrency = 4;
    opts.auto_start = false;

    let mut q = TaskQueue.withOptions(opts);
    let ids = [10 as u64, 20 as u64, 30 as u64];
    q.addBatch(&ids, 5);

    // Paused: nothing dispatches yet
    let mut batch: [4]QueueTask = undefined;
    let n0 = q.drainBatch(&mut batch); // 0

    q.start();
    let n1 = q.drainBatch(&mut batch); // 3
    return n0 + n1;
}
```

## Differences from Upstream & CP Highlights

- **Zero-Heap Inline Storage**: Stores up to `PQ_MAX_TASKS` (64) tasks inline on the stack (`items: [64]QueueTask`) with zero heap allocation.
- **Deterministic Scheduler Model**: Instead of JavaScript `Promise` closures, tasks are identified by `u64` IDs, making `TaskQueue` directly composable with CP actors, worker pools, and state machines.
- **Explicit State Predicates**: `onEmpty()`, `onIdle()`, and `onSizeLessThan(limit)` return `bool` immediately so callers can integrate them into custom polling or actor loops without hidden allocations.
