// Type definitions for CP

import { YOCTO_QUEUE_CAP, PopU64Result, YoctoQueueU64 } from "../../yocto-queue/src/index.cp";
export class SubmitResult {
    started_immediately: bool,
    queued: bool,
    rejected: bool,
}

export class CompleteResult {
    has_next: bool,
    next_task_id: u64,
}

export fn plClampConcurrency(c: usize): usize;
export class PLimit {
    limit: usize,
    active: usize,
    queue: YoctoQueueU64,
    fn new(max_concurrency: usize): PLimit;
    fn activeCount(): usize;
    fn pendingCount(): usize;
    fn queueCapacity(): usize;
    fn concurrency(): usize;
    fn setConcurrency(_: auto, new_limit: usize): bool;
    fn submit(_: auto, task_id: u64): SubmitResult;
    fn complete(_: auto): CompleteResult;
    fn clearQueue(_: auto): usize;
    fn drainReady(_: auto, out: []u64): usize;
}

