[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# p-map

Zero-heap bounded-concurrency mapper, batch/worker partitioner, and error collector for CP, modeled on Sindre Sorhus's npm [`p-map`](https://github.com/sindresorhus/p-map) (MIT, CP v1.0.0).

Provides `pMap` and `pMapCollectErrors` with `stop_on_error` control and skip sentinels (`mapSkip` / `pMapSkip`), a deterministic `ConcurrencyPool` state machine for driving bounded in-flight Actor/async work, and zero-allocation range partitioning helpers (`batchCount`, `batchRange`, `workerPartition`).

## Installation & Import

```bash
cap add p-map
```

```cp
import {
    PMapOptions,
    MapOutcome,
    PMapErrorEntry,
    PMapSummary,
    ConcurrencyPool,
    defaultPMapOptions,
    mapOk,
    mapSkip,
    mapErr,
    pMap,
    pMapCollectErrors,
    batchCount,
    batchRange,
    workerPartition,
} from "p-map";
```

## Core API Reference

### Constants & Data Structures

| Symbol | Fields / Values | Description |
| :--- | :--- | :--- |
| `PMAP_STATUS_OK` / `SKIP` / `ERR` | `0` / `1` / `2` | Discriminant codes stored in `MapOutcome.status`. |
| `BatchSlice` | `start: usize`, `end: usize` | Half-open `[start, end)` index range for a batch or worker slice. |
| `PMapOptions` | `concurrency: usize`, `stop_on_error: bool` | Concurrency limit and fail-fast policy (`defaultPMapOptions()` -> `8`, `true`). |
| `PMapSkip` | `marker: u8` | Sentinel struct mirroring npm `pMapSkip` (constructed via `pMapSkip()`). |
| `MapOutcome` | `status: u8`, `value: i64`, `err_code: i32` | Callback result built with `mapOk(v)`, `mapSkip()`, or `mapErr(code)`. |
| `PMapErrorEntry` | `index: usize`, `err_code: i32` | Per-item failure record populated by `pMapCollectErrors`. |
| `PMapSummary` | `total`, `produced`, `skipped`, `failed`, `batches_run`, `halted`, `first_error_index: usize?`, `first_error_code: i32` | Complete execution summary returned by `pMap` and `pMapCollectErrors`. |

### Functions & `ConcurrencyPool`

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `pMap` | `(items: &[]i64, out: []i64, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary` | Maps `items` in bounded-concurrency batches into `out`, skipping `mapSkip()` entries. |
| `pMapCollectErrors` | `(items: &[]i64, out: []i64, errors_out: []PMapErrorEntry, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary` | Like `pMap`, additionally recording failed item indices and error codes into `errors_out`. |
| `batchCount` | `(total_items: usize, concurrency: usize): usize` | Computes number of batches needed for `total_items` at `concurrency`. |
| `batchRange` | `(total_items: usize, batch_size: usize, batch_idx: usize): BatchSlice?` | Returns `[start, end)` for batch `batch_idx`, or `null` if out of range. |
| `workerPartition` | `(total_items: usize, worker_count: usize, worker_idx: usize): BatchSlice?` | Balances `total_items` evenly across `worker_count` workers. |
| `ConcurrencyPool.new` | `(total_items: usize, max_concurrency: usize, stop_on_error: bool): ConcurrencyPool` | Creates a stack-allocated bounded-concurrency scheduler state machine. |
| `ConcurrencyPool` methods | `acquireNext(): usize?`, `reportOk(): bool`, `reportSkip(): bool`, `reportErr(): bool`, `isDone(): bool`, `inFlight()`, `completedCount()`, `skippedCount()`, `failedCount()`, `remainingCount()`, `reset()` | Drives in-flight slot acquisition, completion accounting, and halt-on-error semantics. |

## Usage Examples

### 1. Mapping with Skip & Fail-Fast (`pMap`)

```cp
import { defaultPMapOptions, mapOk, mapSkip, pMap, MapOutcome } from "p-map";

fn doublePositive(v: i64, idx: usize): MapOutcome {
    if (v <= 0) {
        return mapSkip();
    }
    return mapOk(v * 2);
}

fn runMap(): usize {
    let items: [4]i64 = [10, -1, 20, 30];
    let out: [4]i64 = undefined;
    let summary = pMap(&items, out[0..4], defaultPMapOptions(), doublePositive);
    // summary.produced == 3, summary.skipped == 1 -> out[0..3] == [20, 40, 60]
    return summary.produced;
}
```

### 2. Collect All Errors Without Halting (`pMapCollectErrors`)

```cp
import { PMapOptions, PMapErrorEntry, MapOutcome, mapOk, mapErr, pMapCollectErrors } from "p-map";

fn validateItem(v: i64, idx: usize): MapOutcome {
    if (v < 0) { return mapErr(-100); }
    return mapOk(v);
}

fn collectAll() {
    let items: [4]i64 = [1, -2, 3, -4];
    let out: [4]i64 = undefined;
    let errs: [4]PMapErrorEntry = undefined;
    let opts = PMapOptions { concurrency: 2, stop_on_error: false };
    let sum = pMapCollectErrors(&items, out[0..4], errs[0..4], opts, validateItem);
    // sum.produced == 2, sum.failed == 2, errs[0].index == 1, errs[1].index == 3
}
```

### 3. Driving Custom Concurrency with `ConcurrencyPool`

```cp
import { ConcurrencyPool } from "p-map";

fn scheduleLoop() {
    let mut pool = ConcurrencyPool.new(10, 3, true);
    while (!pool.isDone()) {
        let next = pool.acquireNext();
        if (next != null) {
            // Dispatch work item `next.?`, then report completion:
            pool.reportOk();
        }
    }
}
```

## Differences from Upstream & Zero-Copy Highlights

- **Zero Heap Allocation**: Writes mapped values and error entries directly into caller-owned slices (`out: []i64`, `errors_out: []PMapErrorEntry`) instead of allocating arrays or `AggregateError` objects.
- **Explicit State Machine**: `ConcurrencyPool` separates concurrency bookkeeping from the underlying execution runtime so it integrates cleanly with CP Actors, event loops, or worker threads.
- **Deterministic Summaries**: `PMapSummary` reports exact counts (`produced`, `skipped`, `failed`, `batches_run`, `halted`) and `first_error_index: usize?`.
