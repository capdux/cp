# p-map — AI Agent Quick Ref

Zero-heap bounded-concurrency mapper, batch/worker partitioner, and `ConcurrencyPool` state machine (npm `p-map` parity).

## Import

```cp
import {
    BatchSlice, PMapOptions, PMapSkip, MapOutcome, PMapErrorEntry, PMapSummary,
    ConcurrencyPool, defaultPMapOptions, pMapSkip, mapOk, mapSkip, mapErr,
    batchCount, batchRange, workerPartition, pMap, pMapCollectErrors,
} from "p-map";
```

## Core API Signatures

```cp
// Status constants: PMAP_STATUS_OK=0, PMAP_STATUS_SKIP=1, PMAP_STATUS_ERR=2
class BatchSlice { start: usize, end: usize }
class PMapOptions { concurrency: usize, stop_on_error: bool }
class MapOutcome { status: u8, value: i64, err_code: i32 }
class PMapErrorEntry { index: usize, err_code: i32 }
class PMapSummary {
    total, produced, skipped, failed, batches_run: usize,
    halted: bool, first_error_index: usize?, first_error_code: i32,
}

defaultPMapOptions(): PMapOptions // concurrency=8, stop_on_error=true
pMapSkip(): PMapSkip
mapOk(value: i64): MapOutcome; mapSkip(): MapOutcome; mapErr(err_code: i32): MapOutcome

batchCount(total_items: usize, concurrency: usize): usize
batchRange(total_items: usize, batch_size: usize, batch_idx: usize): BatchSlice?
workerPartition(total_items: usize, worker_count: usize, worker_idx: usize): BatchSlice?

pMap(items: &[]i64, out: []i64, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary
pMapCollectErrors(items: &[]i64, out: []i64, errors_out: []PMapErrorEntry, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary

class ConcurrencyPool { /* state fields */ }
ConcurrencyPool.new(total_items: usize, max_concurrency: usize, stop_on_error: bool): ConcurrencyPool
pool.acquireNext(): usize?; pool.reportOk(): bool; pool.reportSkip(): bool; pool.reportErr(): bool
pool.isDone(): bool; pool.inFlight(): usize; pool.completedCount(): usize
pool.skippedCount(): usize; pool.failedCount(): usize; pool.remainingCount(): usize; pool.reset()
```

## Minimal Example

```cp
import { defaultPMapOptions, mapOk, mapSkip, pMap, MapOutcome } from "p-map";

fn mapper(v: i64, idx: usize): MapOutcome {
    return if (v > 0) { mapOk(v * 2) } else { mapSkip() };
}

fn demo(): usize {
    let items: [3]i64 = [1, 0, 5];
    let out: [3]i64 = undefined;
    let sum = pMap(&items, out[0..3], defaultPMapOptions(), mapper);
    return sum.produced; // 2 -> out[0..2] == [2, 10]
}
```

## Gotchas & Borrow Rules

- **Output compaction**: Skipped (`mapSkip()`) and failed (`mapErr()`) items do not write to `out`; valid results occupy `out[0..summary.produced]`.
- **Collect all errors**: Set `opts.stop_on_error = false` and use `pMapCollectErrors` with a writable `errors_out: []PMapErrorEntry` slice.
- **Read-only input**: Pass `&items` (`&[]i64`); `out` and `errors_out` must be writable slices.
- **Mutable pool**: Declare `let mut pool = ConcurrencyPool.new(...);` since `acquireNext`, `report*`, and `reset` mutate pool state.
