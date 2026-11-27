# p-map — AI Agent Quick Ref

- **Import**: `import { pMap, pMapCollectErrors, defaultPMapOptions, mapOk, mapSkip, mapErr, ConcurrencyPool, ... } from "p-map";`
- **Origin**: npm `p-map` (v1.0.0, MIT, zero-heap)
- **Full Types**: `src/index.d.cp`

## Core Types
```cp
class BatchSlice { start: usize, end: usize }
class PMapOptions { concurrency: usize, stop_on_error: bool }
class MapOutcome { status: u8, value: i64, err_code: i32 }
class PMapErrorEntry { index: usize, err_code: i32 }
class PMapSummary {
    total: usize, produced: usize, skipped: usize, failed: usize,
    batches_run: usize, halted: bool, first_error_index: usize?, first_error_code: i32,
}
```

## Functions
- `defaultPMapOptions(): PMapOptions` | `pMapSkip(): PMapSkip`
- `mapOk(value: i64): MapOutcome` | `mapSkip(): MapOutcome` | `mapErr(err_code: i32): MapOutcome`
- `pMap(items: &[]i64, out: []i64, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary`
- `pMapCollectErrors(items: &[]i64, out: []i64, errors_out: []PMapErrorEntry, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary`
- `batchCount(total_items: usize, concurrency: usize): usize`
- `batchRange(total_items: usize, batch_size: usize, batch_idx: usize): BatchSlice?`
- `workerPartition(total_items: usize, worker_count: usize, worker_idx: usize): BatchSlice?`

## `ConcurrencyPool` Methods
- `ConcurrencyPool.new(total_items: usize, max_concurrency: usize, stop_on_error: bool): ConcurrencyPool`
- `acquireNext(): usize?` | `reportOk(): bool` | `reportSkip(): bool` | `reportErr(): bool`
- `isDone(): bool` | `inFlight(): usize` | `completedCount(): usize` | `skippedCount(): usize` | `failedCount(): usize` | `remainingCount(): usize`
- `reset()`

## Minimal Pattern
```cp
let opts = PMapOptions { concurrency: 2, stop_on_error: false };
let sum = pMapCollectErrors(&items, &mut out, &mut errs, opts, mapper);
// produced values are in out[0..sum.produced]
```

## Gotchas
1. `out` receives only **produced** (non-skipped, non-errored) values densely packed into `out[0..summary.produced]`.
2. In `pMapCollectErrors`, if `errors_out.len < summary.failed`, extra errors are counted in `summary.failed` but not written past `errors_out.len`.
3. `ConcurrencyPool.acquireNext()` returns `null` when either `inFlight() == concurrency`, `halted == true`, or all items are dispatched; call `reportOk()`/`reportSkip()`/`reportErr()` to release slots.
