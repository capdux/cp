# @cap/p-limit-core

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-limit` ([https://github.com/sindresorhus/p-limit](https://github.com/sindresorhus/p-limit))（原库名 `p-limit` → CP 包名 `@cap/p-limit-core`）

Zero-allocation, Clean-room MIT implementation of `p-limit` v6 core concurrency limiting semantics for CP, reusing `@cap/p-queue`.

## Features

- **`PLimit.new(max_concurrency: usize): PLimit?`**: Strictly validates `max_concurrency > 0` (returns `null` on `0`).
- **`submit(*_, task_id: u32): bool`**: Enqueues a task ID into the underlying `@cap/p-queue` FIFO queue.
- **`startNext(*_): u32?`**: Dispatches the next pending task if `activeCount() < concurrency`.
- **`finishOne(*_): bool`**: Completes one active task and releases a concurrency slot.
- **`activeCount(): usize` / `pendingCount(): usize` / `concurrency(): usize`**: Zero-cost state inspection.
- **`clearQueue(*_): usize`**: Drops all pending tasks without affecting active tasks and returns the dropped count.
- **`setConcurrency(*_, new_limit: usize): bool`**: Dynamically adjusts the concurrency cap (rejects `0`).

## Dependencies

- `@cap/p-queue` (`1.0.0`)

## License

MIT
