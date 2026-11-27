// @cap/p-map — Zero-heap bounded-concurrency mapper, batch planner & error collector.
// Clean-room MIT utility modeled on Sindre Sorhus's `p-map` / `p-all` concurrency patterns.

pub const PMAP_STATUS_OK: u8 = 0;
pub const PMAP_STATUS_SKIP: u8 = 1;
pub const PMAP_STATUS_ERR: u8 = 2;

pub struct BatchSlice {
    pub start: usize,
    pub end: usize,
}

/// Options modeled on npm `p-map`: `concurrency` and `stopOnError`.
pub struct PMapOptions {
    pub concurrency: usize,
    pub stop_on_error: bool,
}

pub fn defaultPMapOptions(): PMapOptions {
    return PMapOptions {
        concurrency: 8,
        stop_on_error: true,
    };
}

/// Sentinel marker mirroring npm `pMapSkip`.
pub struct PMapSkip {
    pub marker: u8,
}

pub fn pMapSkip(): PMapSkip {
    return PMapSkip { marker: PMAP_STATUS_SKIP };
}

/// Step result returned by a `pMap` mapper callback (value, skip, or error code).
pub struct MapOutcome {
    pub status: u8,
    pub value: i64,
    pub err_code: i32,
}

pub fn mapOk(value: i64): MapOutcome {
    return MapOutcome { status: PMAP_STATUS_OK, value: value, err_code: 0 };
}

pub fn mapSkip(): MapOutcome {
    return MapOutcome { status: PMAP_STATUS_SKIP, value: 0, err_code: 0 };
}

pub fn mapErr(err_code: i32): MapOutcome {
    return MapOutcome { status: PMAP_STATUS_ERR, value: 0, err_code: err_code };
}

/// Recorded error entry for `stop_on_error = false` aggregate error collection.
pub struct PMapErrorEntry {
    pub index: usize,
    pub err_code: i32,
}

/// Execution summary returned by `pMap` and `pMapCollectErrors`.
pub struct PMapSummary {
    pub total: usize,
    pub produced: usize,
    pub skipped: usize,
    pub failed: usize,
    pub batches_run: usize,
    pub halted: bool,
    pub first_error_index: usize?,
    pub first_error_code: i32,
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
    pub skipped: usize = 0,

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
            skipped: 0,
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

    pub fn reportSkip(*_): bool {
        if (_.in_flight == 0) { return false; }
        _.in_flight = _.in_flight - 1;
        _.skipped = _.skipped + 1;
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
    pub fn skippedCount(): usize { return _.skipped; }
    pub fn failedCount(): usize { return _.failed; }

    pub fn remainingCount(): usize {
        if (_.halted) { return _.in_flight; }
        return (_.total - _.next_index) + _.in_flight;
    }

    pub fn reset(*_) {
        _.next_index = 0;
        _.in_flight = 0;
        _.completed = 0;
        _.failed = 0;
        _.skipped = 0;
        _.halted = false;
    }
}

/// Runs `mapper` over `items` in bounded `opts.concurrency` waves, writing non-skipped
/// results into `out` and collecting errors into `errors_out` when `stop_on_error` is false.
pub fn pMapCollectErrors(
    items: []const i64,
    out: []i64,
    errors_out: []PMapErrorEntry,
    opts: PMapOptions,
    mapper: fn(i64, usize): MapOutcome,
): PMapSummary {
    let conc = if (opts.concurrency == 0) { 1 } else { opts.concurrency };
    let pool = ConcurrencyPool.new(items.len, conc, opts.stop_on_error);
    let produced: usize = 0;
    let err_written: usize = 0;
    let batches_run: usize = 0;
    let first_err_idx: usize? = null;
    let first_err_code: i32 = 0;

    while (!pool.isDone()) {
        // Fill a bounded concurrency wave
        let wave_start = pool.next_index;
        let wave_count: usize = 0;
        while (wave_count < conc) {
            if (pool.acquireNext() == null) { break; }
            wave_count = wave_count + 1;
        }
        if (wave_count == 0) { break; }
        batches_run = batches_run + 1;

        let k: usize = 0;
        while (k < wave_count) {
            let idx = wave_start + k;
            let res = mapper(items[idx], idx);
            if (res.status == PMAP_STATUS_OK) {
                * = pool.reportOk();
                if (produced < out.len) {
                    out[produced] = res.value;
                    produced = produced + 1;
                }
            } else if (res.status == PMAP_STATUS_SKIP) {
                * = pool.reportSkip();
            } else {
                * = pool.reportErr();
                if (first_err_idx == null) {
                    first_err_idx = idx;
                    first_err_code = res.err_code;
                }
                if (err_written < errors_out.len) {
                    errors_out[err_written] = PMapErrorEntry { index: idx, err_code: res.err_code };
                    err_written = err_written + 1;
                }
            }
            k = k + 1;
        }
    }

    return PMapSummary {
        total: items.len,
        produced: produced,
        skipped: pool.skippedCount(),
        failed: pool.failedCount(),
        batches_run: batches_run,
        halted: pool.halted,
        first_error_index: first_err_idx,
        first_error_code: first_err_code,
    };
}

/// Maps `items` with bounded concurrency, `pMapSkip` support, and `stop_on_error` semantics.
pub fn pMap(
    items: []const i64,
    out: []i64,
    opts: PMapOptions,
    mapper: fn(i64, usize): MapOutcome,
): PMapSummary {
    let dummy_errs: [1]PMapErrorEntry = [PMapErrorEntry { index: 0, err_code: 0 }];
    return pMapCollectErrors(items, out, dummy_errs[0..0], opts, mapper);
}
