[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`p-settle`](https://github.com/sindresorhus/p-settle)（`MIT`）（包名 `p-settle`）。

# `p-settle`

Zero-heap-allocation bounded-concurrency `Promise.allSettled` task planner in pure CP (clean-room MIT implementation inspired by npm `p-settle` v5).

Reuses [`p-limit`](../p-limit) (`PLimit`) for bounded concurrency and FIFO task dispatch without ever short-circuiting on errors.

## Features

- **`SettleStatus` & `SettledEntry`** — Structured per-task settlement state (`Pending`, `Fulfilled`, `Rejected`) with result `value` and `err_code`.
- **`PSettlePlanner.new(concurrency)`** — Creates a 32-slot zero-heap planner (`concurrency == 0` returns `null`).
- **`addTask(id)` / `dispatchNext()`** — Enqueues tasks and dispatches up to `concurrency` tasks concurrently via `PLimit.startNext()`.
- **`reportFulfilled(id, value)` / `reportRejected(id, err_code)`** — Settles tasks independently; rejections never cancel queued tasks.
- **`isAllSettled()` / `fulfilledCount()` / `rejectedCount()` / `entryAt(idx)`** — Inspect aggregate counts and indexed settlement entries in registration order.

## Usage

```cp
import { P_SETTLE_CAP, SettleStatus, SettledEntry, PSettlePlanner } from "p-settle";
```

## License

MIT
