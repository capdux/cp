// Type definitions for CP

export class QueueTask {
    id: u64,
    priority: i32,
    seq: u64,
}

export class QueueOptions {
    concurrency: usize,
    auto_start: bool,
    interval_cap: usize,
    interval_ms: u64,
    fn default(): QueueOptions;
}

export class TaskQueue {
    items: [64]QueueTask,
    count: usize,
    max_concurrency: usize,
    active: usize,
    paused: bool,
    next_seq: u64,
    completed_total: u64,
    fn new(max_concurrency: usize): TaskQueue;
    fn withOptions(opts: QueueOptions): TaskQueue;
    fn size(): usize;
    fn activeCount(): usize;
    fn pending(): usize;
    fn concurrency(): usize;
    fn setConcurrency(_: auto, max_concurrency: usize);
    fn isPaused(): bool;
    fn isIdle(): bool;
    fn onEmpty(): bool;
    fn onIdle(): bool;
    fn onSizeLessThan(limit: usize): bool;
    fn completedCount(): u64;
    fn canDispatch(): bool;
    fn pause(_: auto);
    fn start(_: auto);
    fn enqueue(_: auto, id: u64, priority: i32): bool;
    fn add(_: auto, id: u64, priority: i32): bool;
    fn addBatch(_: auto, ids: &[]u64, priority: i32): usize;
    fn peek(): QueueTask?;
    fn remove(_: auto, id: u64): bool;
    fn setPriority(_: auto, id: u64, new_priority: i32): bool;
    fn dispatchNext(_: auto): QueueTask?;
    fn drainBatch(_: auto, out: []QueueTask): usize;
    fn markCompleted(_: auto): bool;
    fn sizeByPriority(min_priority: i32): usize;
    fn sizeByExactPriority(priority: i32): usize;
    fn clear(_: auto);
    fn resetStats(_: auto);
}

