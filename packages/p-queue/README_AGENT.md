# p-queue — AI Agent Quick Ref

- **Import**: `import { TaskQueue, QueueOptions, QueueTask } from "p-queue";`
- **Origin**: npm `p-queue` (v1.0.0, MIT, zero-heap fixed capacity = 64)
- **Full Types**: `src/index.d.cp`

## Core Types
```cp
class QueueTask { id: u64, priority: i32, seq: u64 }
class QueueOptions {
    concurrency: usize, auto_start: bool, interval_cap: usize, interval_ms: u64,
    fn default(): QueueOptions;
}
```

## `TaskQueue` Methods
- **Construct**: `TaskQueue.new(max_concurrency: usize): TaskQueue` | `TaskQueue.withOptions(opts: QueueOptions): TaskQueue`
- **Enqueue**: `add(id: u64, priority: i32): bool` (alias: `enqueue`) | `addBatch(ids: &[]u64, priority: i32): usize`
- **Dispatch & Complete**:
  - `peek(): QueueTask?` | `canDispatch(): bool`
  - `dispatchNext(): QueueTask?` | `drainBatch(out: []QueueTask): usize`
  - `markCompleted(): bool`
- **Modify**: `remove(id: u64): bool` | `setPriority(id: u64, new_priority: i32): bool` | `setConcurrency(max_concurrency: usize)`
- **Control**: `pause()` | `start()` | `clear()` | `resetStats()`
- **Inspect**:
  - `size(): usize` (waiting count) | `activeCount(): usize` (alias: `pending()`)
  - `concurrency(): usize` | `completedCount(): u64`
  - `isPaused(): bool` | `isIdle(): bool` (alias: `onIdle()`)
  - `onEmpty(): bool` | `onSizeLessThan(limit: usize): bool`
  - `sizeByPriority(min_priority: i32): usize` | `sizeByExactPriority(priority: i32): usize`

## Minimal Pattern
```cp
mut q = TaskQueue.new(2);
let _ = q.add(101, 10);
if (q.dispatchNext()) |t| {
    // run task t.id
    let _ = q.markCompleted();
}
```

## Gotchas
1. Fixed capacity is **64** pending tasks (`items: [64]QueueTask`); `add`/`enqueue` return `false` when full.
2. Higher `priority` runs first; equal priorities break ties FIFO via `seq`.
3. `pending()` is an alias for `activeCount()` (in-flight count, matching npm `p-queue`), NOT waiting queue size (which is `size()`).
4. Always pair each `dispatchNext()` / `drainBatch()` task with `markCompleted()`, otherwise concurrency slots stay occupied.
