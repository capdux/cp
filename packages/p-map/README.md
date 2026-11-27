[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/p-map

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-map` ([https://github.com/sindresorhus/p-map](https://github.com/sindresorhus/p-map))

Clean-room MIT, zero-heap bounded-concurrency batch planner, balanced worker partitioner, and `ConcurrencyPool` dispatcher for CP — modeled on `p-map` / `p-all`.

## API
- `batchCount(total_items, concurrency): usize`
- `batchRange(total_items, batch_size, batch_idx): BatchSlice?`
- `workerPartition(total_items, worker_count, worker_idx): BatchSlice?`
- `ConcurrencyPool` (`new`, `acquireNext`, `reportOk`, `reportErr`, `isDone`, `inFlight`, `completedCount`, `failedCount`)
