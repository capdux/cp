[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# p-timeout

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-timeout` ([https://github.com/sindresorhus/p-timeout](https://github.com/sindresorhus/p-timeout))（原库名 `p-timeout` → CP 包名 `p-timeout`）

Zero-allocation, Clean-room MIT implementation of `p-timeout` v6 deadline tracking and timeout state machine semantics for CP.

## Features

- **State Constants**: `PT_PENDING` (`0`), `PT_RESOLVED` (`1`), `PT_REJECTED` (`2`), `PT_TIMED_OUT` (`3`), `PT_CANCELED` (`4`).
- **`TimeoutOptions`**: `{ milliseconds, has_fallback, fallback_code, error_message }` with `TimeoutOptions.withMs(ms: u64)`.
- **`TimeoutGuard`**: Zero-heap deadline tracker with saturating arithmetic:
  - `TimeoutGuard.new(start_ms: u64, opts: TimeoutOptions): TimeoutGuard`
  - `deadlineMs(): u64` / `remainingMs(now_ms: u64): u64` / `isExpired(now_ms: u64): bool`
  - `poll(*_, now_ms: u64): u8` (transitions to `PT_RESOLVED` with fallback or `PT_TIMED_OUT` on expiry)
  - `resolve(*_, now_ms: u64, value_code: i32): bool` / `reject(*_, now_ms: u64, err_code: i32): bool` / `cancel(*_): bool`
  - `isSettled(): bool`

## License

MIT
