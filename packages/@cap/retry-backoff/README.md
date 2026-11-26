# @cap/retry-backoff

Clean-room MIT, zero-heap exponential backoff & retry policy state machine for CP — modeled on npm's `retry`/`p-retry` and Rust's `backoff`.

## API
- `RetryPolicy` (`exponential`, `constant`)
- `computeBackoffMs(policy, attempt): u64`
- `applyJitterMs(base_delay_ms, jitter_bps, seed): u64`
- `BackoffState` (`new`, `nextDelayMs`, `attempts`, `totalWaitedMs`, `reset`)
