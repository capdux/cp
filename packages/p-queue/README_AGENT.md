# p-queue — AI Agent Quick Ref

Zero-heap fixed-capacity (`PQ_MAX_TASKS = 64`) priority task queue with concurrency limit, pause/resume, and batch dispatch.

## Import
```typescript
import { TaskQueue, QueueOptions, QueueTask, PQ_MAX_TASKS } from "p-queue";
```

## API Signatures
```typescript
export const PQ_MAX_TASKS: usize = 64;
export class QueueTask { id: u64, priority: i32, seq: u64 }
export class QueueOptions {
    concurrency: usize, auto_start: bool, interval_cap: usize, interval_ms: u64,
    fn default(): QueueOptions;
}
export class TaskQueue {
    items: [64]QueueTask, count: usize, max_concurrency: usize, active: usize,
    paused: bool, next_seq: u64, completed_total: u64,
    fn new(max_concurrency: usize): TaskQueue;
    fn withOptions(opts: QueueOptions): TaskQueue;
    fn size(): usize;
    fn activeCount(): usize;
    fn pending(): usize;
    fn concurrency(): usize;
    fn setConcurrency(*_, max_concurrency: usize);
    fn isPaused(): bool;
    fn isIdle(): bool;
    fn onEmpty(): bool;
    fn onIdle(): bool;
    fn onSizeLessThan(limit: usize): bool;
    fn completedCount(): u64;
    fn canDispatch(): bool;
    fn pause(*_);
    fn start(*_);
    fn enqueue(*_, id: u64, priority: i32): bool;
    fn add(*_, id: u64, priority: i32): bool;
    fn addBatch(*_, ids: &[]u64, priority: i32): usize;
    fn peek(): QueueTask?;
    fn remove(*_, id: u64): bool;
    fn setPriority(*_, id: u64, new_priority: i32): bool;
    fn dispatchNext(*_): QueueTask?;
    fn drainBatch(*_, out: []QueueTask): usize;
    fn markCompleted(*_): bool;
    fn sizeByPriority(min_priority: i32): usize;
    fn sizeByExactPriority(priority: i32): usize;
    fn clear(*_);
    fn resetStats(*_);
}
```

## Minimal Example
```typescript
import { TaskQueue } from "p-queue";

fn run(): u64 {
    let mut q = TaskQueue.new(2);
    q.add(1, 5);
    q.add(2, 10);
    if (task := q.dispatchNext()) {
        q.markCompleted();
        return task.id; // 2
    }
    return 0;
}
```

## Pitfalls & Rules
- Fixed capacity is `PQ_MAX_TASKS` (64); `enqueue`/`add` returns `false` when full.
- Higher `priority` (`i32`) dispatches first; ties break by ascending `seq` (FIFO).
- Every `dispatchNext()` / `drainBatch()` increments `active`; call `markCompleted()` when a task finishes to free concurrency slots.
- `addBatch` takes a read-only slice `&[]u64`; `drainBatch` writes into a mutable slice `[]QueueTask`.
- Mutating methods require `let mut q`.
