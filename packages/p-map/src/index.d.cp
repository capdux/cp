// Type definitions for CP

export class BatchSlice {
    start: usize,
    end: usize,
}

export class PMapOptions {
    concurrency: usize,
    stop_on_error: bool,
}

export fn defaultPMapOptions(): PMapOptions;
export class PMapSkip {
    marker: u8,
}

export fn pMapSkip(): PMapSkip;
export class MapOutcome {
    status: u8,
    value: i64,
    err_code: i32,
}

export fn mapOk(value: i64): MapOutcome;
export fn mapSkip(): MapOutcome;
export fn mapErr(err_code: i32): MapOutcome;
export class PMapErrorEntry {
    index: usize,
    err_code: i32,
}

export class PMapSummary {
    total: usize,
    produced: usize,
    skipped: usize,
    failed: usize,
    batches_run: usize,
    halted: bool,
    first_error_index: usize?,
    first_error_code: i32,
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
    skipped: usize,
    fn new(total_items: usize, max_concurrency: usize, stop_on_error: bool): ConcurrencyPool;
    fn acquireNext(_: auto): usize?;
    fn reportOk(_: auto): bool;
    fn reportSkip(_: auto): bool;
    fn reportErr(_: auto): bool;
    fn isDone(): bool;
    fn inFlight(): usize;
    fn completedCount(): usize;
    fn skippedCount(): usize;
    fn failedCount(): usize;
    fn remainingCount(): usize;
    fn reset(_: auto);
}

export fn pMapCollectErrors(items: &[]i64, out: []i64, errors_out: []PMapErrorEntry, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary;
export fn pMap(items: &[]i64, out: []i64, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary;
