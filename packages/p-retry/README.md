[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# p-retry

Zero-heap exponential backoff & retry policy state machine with min/max caps, jitter, and elapsed deadline (clean-room port of npm retry / p-retry)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-retry` ([https://github.com/sindresorhus/p-retry](https://github.com/sindresorhus/p-retry))（原库名 `p-retry` → CP 包名 `p-retry`）

| Metadata | Value |
| --- | --- |
| Package | `p-retry` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add p-retry
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RetryPolicy` | `class` | Fields: `max_retries: u32, min_delay_ms: u64, max_delay_ms: u64, factor_num: u64, factor_den: u64, jitter_bps: u32, +1 more` · Methods: `exponential, constant` |
| `BackoffState` | `class` | Fields: `policy: RetryPolicy, attempt: u32, total_waited_ms: u64, rng_seed: u64` · Methods: `new, attempts, totalWaitedMs, nextDelayMs, reset` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rbkClampDelay` | `(delay: u64, max_delay: u64): u64` | Invokes `rbkClampDelay` and returns `u64`. |
| `rbkMix64` | `(x: u64): u64` | Invokes `rbkMix64` and returns `u64`. |
| `computeBackoffMs` | `(policy: RetryPolicy, attempt: u32): u64` | Invokes `computeBackoffMs` and returns `u64`. |
| `applyJitterMs` | `(base_delay_ms: u64, jitter_bps: u32, seed: u64): u64` | Invokes `applyJitterMs` and returns `u64`. |

## Minimal Example

```cp
import { rbkClampDelay } from "p-retry";

pub fn main() {
    let result = rbkClampDelay(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
