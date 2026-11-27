[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# p-map

Zero-heap bounded-concurrency mapper (`pMap` / `pMapCollectErrors`), skip/error outcome protocol (`mapOk`/`mapSkip`/`mapErr`), batch/worker range partitioner, and `ConcurrencyPool` state machine in pure CP.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [npmjs.com/package/p-map](https://www.npmjs.com/package/p-map) |
| **Upstream Package** | `p-map` (v1.0.0 in CP) |
| **Upstream Repository** | https://github.com/sindresorhus/p-map |
| **License** | MIT |

Keep the exact package name when migrating from TypeScript/npm to CP:

```ts
// TypeScript (npm)
import pMap, { pMapSkip } from "p-map";
const result = await pMap(items, mapper, { concurrency: 4, stopOnError: false });
```

```cp
// CP — same package name, zero-heap deterministic concurrency planner & mapper
import {
    pMap, pMapCollectErrors, defaultPMapOptions,
    mapOk, mapSkip, mapErr, ConcurrencyPool,
} from "p-map";
```

## Installation

```bash
cap add p-map
```

## Quick Start & Practical Examples

### 1. Bounded Mapping with Skip & Error Handling (`pMap` & `pMapCollectErrors`)

```cp
import {
    pMap, pMapCollectErrors, defaultPMapOptions,
    mapOk, mapSkip, mapErr,
    PMapOptions, PMapErrorEntry, MapOutcome,
} from "p-map";

fn transform(val: i64, _idx: usize): MapOutcome {
    if (val < 0) {
        return mapErr(-1);       // Record error code
    }
    if (val == 0) {
        return mapSkip();        // Equivalent to pMapSkip
    }
    return mapOk(val * 10);
}

fn demoPMap() {
    let input: [5]i64 = [2, 0, -3, 4, -5];
    mut out: [5]i64 = [0; 5];
    mut errs: [5]PMapErrorEntry = [PMapErrorEntry { index: 0, err_code: 0 }; 5];

    // Collect all errors without halting early (stop_on_error = false)
    let opts = PMapOptions { concurrency: 2, stop_on_error: false };
    let summary = pMapCollectErrors(&input, &mut out, &mut errs, opts, transform);
    // summary.produced == 2 (out[0..2] = [20, 40])
    // summary.skipped == 1
    // summary.failed == 2 (errs[0].index == 2, errs[1].index == 4)
    // summary.halted == false

    // Fast-fail mode (defaultPMapOptions has stop_on_error = true)
    let strict_opts = defaultPMapOptions();
    let strict_summary = pMap(&input, &mut out, strict_opts, transform);
    // strict_summary.halted == true, strict_summary.first_error_index == 2
}
```

### 2. Batch & Worker Partitioning (`batchCount` / `batchRange` / `workerPartition`)

```cp
import { batchCount, batchRange, workerPartition } from "p-map";

fn demoPartition() {
    let batches = batchCount(10, 3); // 4 batches: [0..3), [3..6), [6..9), [9..10)
    if (batchRange(10, 3, 3)) |last_batch| {
        // last_batch.start == 9, last_batch.end == 10
    }

    // Evenly distribute 10 items across 4 workers
    if (workerPartition(10, 4, 0)) |slice0| {
        // Worker 0 handles [slice0.start .. slice0.end)
    }
}
```

### 3. Stateful `ConcurrencyPool`

```cp
import { ConcurrencyPool } from "p-map";

fn demoPool() {
    mut pool = ConcurrencyPool.new(10, 3, true);
    while (!pool.isDone()) {
        if (pool.acquireNext()) |idx| {
            if (idx % 2 == 0) {
                let _ = pool.reportOk();
            } else {
                let _ = pool.reportSkip();
            }
        }
    }
    let done = pool.completedCount();
    let skipped = pool.skippedCount();
    pool.reset();
}
```

## Performance & Safety Highlights

- **Zero Heap Allocations**: All outputs and error entries are written into caller-provided stack/slice buffers (`out: []i64`, `errors_out: []PMapErrorEntry`).
- **Two Error Policies**: Supports both fail-fast (`stop_on_error: true`) and full error aggregation (`stop_on_error: false` with `pMapCollectErrors`).
- **Explicit Outcome Protocol**: `mapOk`, `mapSkip`, and `mapErr` avoid sentinel ambiguity while preserving `pMapSkip` ergonomics.
- **Bounds-Safe Partitioning**: `batchRange` and `workerPartition` return `null` on out-of-range indices or zero batch/worker counts.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
