// Type definitions for CP

export class QueueTask {
    id: u64,
    priority: i32,
    seq: u64,
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
    fn size(): usize;
    fn activeCount(): usize;
    fn isPaused(): bool;
    fn isIdle(): bool;
    fn completedCount(): u64;
    fn pause(_: auto);
    fn start(_: auto);
    fn enqueue(_: auto, id: u64, priority: i32): bool;
    fn dispatchNext(_: auto): QueueTask?;
    fn markCompleted(_: auto): bool;
    fn sizeByPriority(min_priority: i32): usize;
    fn clear(_: auto);
}

