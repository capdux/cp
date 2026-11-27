// Type definitions for CP

export class BatchSlice {
    start: usize,
    end: usize,
}

export fn batchCount(total_items: usize, concurrency: usize): usize;
export fn batchRange(total_items: usize, batch_size: usize, batch_idx: usize): BatchSlice?;
export fn workerPartition(total_items: usize, worker_count: usize, worker_idx: usize): BatchSlice?;
export class ConcurrencyPool {
    total: usize,
    concurrency: usize,
    next_index: usize,
    in_flight: usize,
    completed: usize,
    failed: usize,
    stop_on_error: bool,
    halted: bool,
    fn new(total_items: usize, max_concurrency: usize, stop_on_error: bool): ConcurrencyPool;
    fn acquireNext(_: auto): usize?;
    fn reportOk(_: auto): bool;
    fn reportErr(_: auto): bool;
    fn isDone(): bool;
    fn inFlight(): usize;
    fn completedCount(): usize;
    fn failedCount(): usize;
}

