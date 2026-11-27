# async-retry — AI Agent Quick Ref

Zero-allocation industrial retry & backoff engine (Fixed/Linear/Exponential/Fibonacci + AWS Full/Equal/Decorrelated Jitter + RetryBudget) for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum BackoffStrategy { Fixed, Linear, Exponential, Fibonacci }
enum JitterMode { None, Full, Equal, Decorrelated }
enum FailureKind { Transient, Permanent, RateLimited }
enum StopReason { None, Succeeded, PermanentFailure, MaxAttemptsReached, ElapsedBudgetExceeded, RetryBudgetExhausted }
class RetryPolicy { strategy: BackoffStrategy, jitter: JitterMode, max_retries: u32, base_delay_ns: u64, /* +4 fields */ } // methods: exponentialDefault(...) +2
class RetryDecision { should_retry: bool, delay_ns: u64, attempt: u32, stop_reason: StopReason }
class SplitMix64 { state: u64 } // methods: fromOs(...) +3
class RetryState { policy: RetryPolicy, rng: SplitMix64, attempts_done: u32, prev_delay_ns: u64, /* +4 fields */ } // methods: init(...) +7
class RetryBudget { tokens: u64, max_tokens: u64, deposit_per_request: u64, withdraw_per_retry: u64, /* +2 fields */ } // methods: init(...) +3
```

## API Signatures

```cp
fn saturatingMulU64(a: u64, b: u64): u64;
fn clampDelay(delay: u64, max_delay: u64): u64;
```

## Minimal Usage

```cp
import { saturatingMulU64 } from "async-retry";

pub fn main() {
    let result = saturatingMulU64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"async-retry"`; install via `cap add async-retry`.
