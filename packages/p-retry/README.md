[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# p-retry

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-retry` ([https://github.com/sindresorhus/p-retry](https://github.com/sindresorhus/p-retry))（原库名 `p-retry` → CP 包名 `p-retry`）

Clean-room MIT, zero-heap exponential backoff & retry policy state machine for CP — modeled on npm's `retry`/`p-retry` and Rust's `backoff`.

## API
- `RetryPolicy` (`exponential`, `constant`)
- `computeBackoffMs(policy, attempt): u64`
- `applyJitterMs(base_delay_ms, jitter_bps, seed): u64`
- `BackoffState` (`new`, `nextDelayMs`, `attempts`, `totalWaitedMs`, `reset`)
