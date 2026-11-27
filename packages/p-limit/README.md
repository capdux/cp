[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`p-limit`](https://github.com/sindresorhus/p-limit)（`MIT`）（包名 `p-limit`）。

# `p-limit`

Zero-heap-allocation concurrency limiter and FIFO task slot scheduler in pure CP (clean-room MIT implementation inspired by npm `p-limit`).

Reuses [`yocto-queue`](../yocto-queue) (`YoctoQueueU64`) for O(1) fixed-capacity FIFO task queuing.

## Features

- **`PLimit.new(concurrency)`** — Creates a concurrency limiter (`0` is safely clamped to `1`).
- **`submit(task_id)`** — Immediately acquires a slot if `activeCount() < concurrency()`, otherwise queues in FIFO order or returns `rejected: true` if full.
- **`complete()`** — Releases an active slot and automatically dequeues the next pending task if capacity permits.
- **`setConcurrency(new_limit)` + `drainReady(out)`** — Dynamic scale-up and scale-down of concurrency limits with batch task wake-up.
- **`clearQueue()`** — Drops all pending tasks without disturbing active tasks.

## Usage

```cp
import { PLimit, SubmitResult, CompleteResult } from "p-limit";
```

## License

MIT
