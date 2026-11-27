# p-retry — AI Agent Quick Ref

Zero-heap exponential backoff & retry policy state machine with min/max caps, jitter, and elapsed deadline (clean-room port of npm retry / p-retry) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RetryPolicy,
    BackoffState,
    rbkClampDelay,
    rbkMix64,
    computeBackoffMs,
    applyJitterMs,
} from "p-retry";
```

## Core Types & Constants

```cp
class RetryPolicy { max_retries: u32, min_delay_ms: u64, max_delay_ms: u64, factor_num: u64, /* +3 fields */ } // methods: exponential(...) +1
class BackoffState { policy: RetryPolicy, attempt: u32, total_waited_ms: u64, rng_seed: u64 } // methods: new(...) +4
```

## API Signatures

```cp
fn rbkClampDelay(delay: u64, max_delay: u64): u64;
fn rbkMix64(x: u64): u64;
fn computeBackoffMs(policy: RetryPolicy, attempt: u32): u64;
fn applyJitterMs(base_delay_ms: u64, jitter_bps: u32, seed: u64): u64;
```

## Minimal Usage

```cp
import { rbkClampDelay } from "p-retry";

pub fn main() {
    let result = rbkClampDelay(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"p-retry"`; install via `cap add p-retry`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
