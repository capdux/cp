# p-map@1.0.0 (Agent Quickref)

> Zero-heap bounded-concurrency batch planner, chunk partitioner, and token-bucket rate limiter (clean-room port of npm p-map / p-all) | License: MIT
> Import: `import { ... } from "p-map";` | Full types: `src/index.d.cp`

## Functions
- `export fn defaultPMapOptions(): PMapOptions;`
- `export fn pMapSkip(): PMapSkip;`
- `export fn mapOk(value: i64): MapOutcome;`
- `export fn mapSkip(): MapOutcome;`
- `export fn mapErr(err_code: i32): MapOutcome;`
- `export fn batchCount(total_items: usize, concurrency: usize): usize;`
- `export fn batchRange(total_items: usize, batch_size: usize, batch_idx: usize): BatchSlice?;`
- `export fn workerPartition(total_items: usize, worker_count: usize, worker_idx: usize): BatchSlice?;`
- `export fn pMapCollectErrors(items: &[]i64, out: []i64, errors_out: []PMapErrorEntry, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary;`
- `export fn pMap(items: &[]i64, out: []i64, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary;`
