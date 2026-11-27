[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`p-all`](https://github.com/sindresorhus/p-all)（`MIT`）（包名 `@cap/p-all`）。

# `@cap/p-all`

Zero-heap-allocation bounded-concurrency batch task planner in pure CP (clean-room MIT implementation inspired by npm `p-all` v5).

Reuses [`@cap/p-limit`](../p-limit) (`PLimit`) for bounded concurrency and FIFO task dispatch.

## Features

- **`PAllPlanner.new(concurrency, stop_on_error)`** — Creates a 32-slot zero-heap batch planner (`concurrency == 0` returns `null`).
- **`addTask(id)` / `dispatchNext()`** — Registers tasks in FIFO order and dispatches up to `concurrency` tasks simultaneously via `PLimit.startNext()`.
- **`reportSuccess(id, value)` / `reportError(id, err_code)`** — Tracks per-task results and errors; when `stop_on_error` is `true`, the first error immediately aborts the batch and calls `limiter.clearQueue()`.
- **`isDone()` / `hasFailed()` / `firstErrorCode()` / `completedCount()` / `resultOf(id)`** — Query completion, failure state, and per-task results with zero heap allocation.

## Usage

```cp
import { P_ALL_CAP, PAllPlanner } from "@cap/p-all";
```

## License

MIT
