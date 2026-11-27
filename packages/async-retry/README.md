[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# async-retry

Zero-allocation industrial retry & backoff engine (Fixed/Linear/Exponential/Fibonacci + AWS Full/Equal/Decorrelated Jitter + RetryBudget) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`async-retry`](https://github.com/vercel/async-retry)（`MIT`）（原库名 `async-retry`，CP 包名 `async-retry`）。

| Metadata | Value |
| --- | --- |
| Package | `async-retry` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add async-retry
```

```cp
import {
    BackoffStrategy,
    JitterMode,
    FailureKind,
    StopReason,
    RetryPolicy,
    RetryDecision,
    SplitMix64,
    RetryState,
} from "async-retry";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `BackoffStrategy` | `Fixed, Linear, Exponential, Fibonacci` |
| `enum` | `JitterMode` | `None, Full, Equal, Decorrelated` |
| `enum` | `FailureKind` | `Transient, Permanent, RateLimited` |
| `enum` | `StopReason` | `None, Succeeded, PermanentFailure, MaxAttemptsReached, ElapsedBudgetExceeded, RetryBudgetExhausted` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RetryPolicy` | `class` | Fields: `strategy: BackoffStrategy, jitter: JitterMode, max_retries: u32, base_delay_ns: u64, max_delay_ns: u64, max_elapsed_ns: u64, +2 more` · Methods: `exponentialDefault, fixed, decorrelated` |
| `RetryDecision` | `class` | Fields: `should_retry: bool, delay_ns: u64, attempt: u32, stop_reason: StopReason` |
| `SplitMix64` | `class` | Fields: `state: u64` · Methods: `fromOs, fromSeed, nextU64, uniformInclusive` |
| `RetryState` | `class` | Fields: `policy: RetryPolicy, rng: SplitMix64, attempts_done: u32, prev_delay_ns: u64, fib_prev: u64, fib_curr: u64, +2 more` · Methods: `init, initSeeded, reset, computeRawDelay, applyJitter, onFailure, +2 more` |
| `RetryBudget` | `class` | Fields: `tokens: u64, max_tokens: u64, deposit_per_request: u64, withdraw_per_retry: u64, retries_allowed: u64, retries_rejected: u64` · Methods: `init, recordRequest, tryAcquireRetry, availableRetries` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `saturatingMulU64` | `(a: u64, b: u64): u64` | Invokes `saturatingMulU64` and returns `u64`. |
| `clampDelay` | `(delay: u64, max_delay: u64): u64` | Invokes `clampDelay` and returns `u64`. |

## Minimal Example

```cp
import { saturatingMulU64 } from "async-retry";

pub fn main() {
    let result = saturatingMulU64(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
