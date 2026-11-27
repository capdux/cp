// @cap/p-map — Zero-heap bounded-concurrency batch planner, work partitioner & token bucket.
// Clean-room MIT utility modeled on Sindre Sorhus's `p-map` / `p-all` concurrency patterns.

pub struct BatchSlice {
    pub start: usize,
    pub end: usize,
}

/// Computes the number of batches needed to process `total_items` with `concurrency` slots.
pub fn batchCount(total_items: usize, concurrency: usize): usize {
    if (total_items == 0) { return 0; }
    let c = if (concurrency == 0) { 1 } else { concurrency };
    return (total_items + c - 1) / c;
}

/// Returns the `(start, end)` range for `batch_idx` when splitting `total_items` by `batch_size`.
pub fn batchRange(total_items: usize, batch_size: usize, batch_idx: usize): BatchSlice? {
    if (total_items == 0) { return null; }
    let sz = if (batch_size == 0) { 1 } else { batch_size };
    let start = batch_idx * sz;
    if (start >= total_items) { return null; }
    let end = if (start + sz > total_items) { total_items } else { start + sz };
    return BatchSlice { start: start, end: end };
}

/// Partitions `total_items` evenly across `worker_count` workers (at most +/-1 item difference).
pub fn workerPartition(total_items: usize, worker_count: usize, worker_idx: usize): BatchSlice? {
    if (worker_count == 0 || worker_idx >= worker_count) { return null; }
    let base = total_items / worker_count;
    let rem = total_items % worker_count;
    let start = worker_idx * base + (if (worker_idx < rem) { worker_idx } else { rem });
    let extra: usize = if (worker_idx < rem) { 1 } else { 0 };
    let count = base + extra;
    return BatchSlice { start: start, end: start + count };
}

/// Zero-heap bounded-concurrency work-item dispatcher (`ConcurrencyPool`).
pub struct ConcurrencyPool {
    pub total: usize,
    pub concurrency: usize,
    pub next_index: usize,
    pub in_flight: usize,
    pub completed: usize,
    pub failed: usize,
    pub stop_on_error: bool,
    pub halted: bool,

    pub static fn new(total_items: usize, max_concurrency: usize, stop_on_error: bool): ConcurrencyPool {
        let c = if (max_concurrency == 0) { 1 } else { max_concurrency };
        return ConcurrencyPool {
            total: total_items,
            concurrency: c,
            next_index: 0,
            in_flight: 0,
            completed: 0,
            failed: 0,
            stop_on_error: stop_on_error,
            halted: false,
        };
    }

    pub fn acquireNext(*_): usize? {
        if (_.halted || _.in_flight >= _.concurrency || _.next_index >= _.total) {
            return null;
        }
        let idx = _.next_index;
        _.next_index = _.next_index + 1;
        _.in_flight = _.in_flight + 1;
        return idx;
    }

    pub fn reportOk(*_): bool {
        if (_.in_flight == 0) { return false; }
        _.in_flight = _.in_flight - 1;
        _.completed = _.completed + 1;
        return true;
    }

    pub fn reportErr(*_): bool {
        if (_.in_flight == 0) { return false; }
        _.in_flight = _.in_flight - 1;
        _.failed = _.failed + 1;
        if (_.stop_on_error) { _.halted = true; }
        return true;
    }

    pub fn isDone(): bool {
        if (_.halted) { return _.in_flight == 0; }
        return _.next_index >= _.total && _.in_flight == 0;
    }

    pub fn inFlight(): usize { return _.in_flight; }
    pub fn completedCount(): usize { return _.completed; }
    pub fn failedCount(): usize { return _.failed; }
}
