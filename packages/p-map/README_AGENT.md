# p-map@1.0.0 (Agent Quickref)

> Zero-heap bounded-concurrency batch planner, chunk partitioner, and token-bucket rate limiter (clean-room port of npm p-map / p-all) | License: MIT
> Import: `import { ... } from "p-map";` | Full types: `src/index.d.cp`

## Functions
- `export fn batchCount(total_items: usize, concurrency: usize): usize;`
- `export fn batchRange(total_items: usize, batch_size: usize, batch_idx: usize): BatchSlice?;`
- `export fn workerPartition(total_items: usize, worker_count: usize, worker_idx: usize): BatchSlice?;`
