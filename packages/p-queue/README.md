[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/p-queue

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-queue` ([https://github.com/sindresorhus/p-queue](https://github.com/sindresorhus/p-queue))

Clean-room MIT, zero-heap concurrency-limited priority task queue scheduler for CP — modeled on Sindre Sorhus's `p-queue` v8.

## API
- `TaskQueue.new(max_concurrency)` (up to `PQ_MAX_TASKS = 64`)
- `enqueue(id, priority)`, `dispatchNext()`, `markCompleted()`
- `pause()`, `start()`, `clear()`, `size()`, `activeCount()`, `isPaused()`, `isIdle()`, `completedCount()`, `sizeByPriority(min_priority)`
