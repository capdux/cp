// Type definitions for CP

export class FastQueue {
    buf: [64]u64,
    head: usize,
    tail: usize,
    queued: usize,
    active: usize,
    concurrency: usize,
    paused: bool,
    fn new(concurrency: usize): FastQueue;
    fn length(): usize;
    fn running(): usize;
    fn idle(): bool;
    fn isPaused(): bool;
    fn setConcurrency(_: auto, c: usize);
    fn pause(_: auto);
    fn unpause(_: auto);
    fn push(_: auto, task_id: u64): bool;
    fn unshift(_: auto, task_id: u64): bool;
    fn nextTask(_: auto): u64?;
    fn completeTask(_: auto): bool;
    fn kill(_: auto): usize;
}

