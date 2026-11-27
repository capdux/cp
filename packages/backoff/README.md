[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# backoff

Zero-allocation deterministic retry backoff engine with Exponential, Linear, and AWS Full/Equal/Decorrelated Jitter (Rust backoff / npm exponential-backoff parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:backoff` ([https://github.com/ihrwein/backoff](https://github.com/ihrwein/backoff))

| Metadata | Value |
| --- | --- |
| Package | `backoff` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add backoff
```

```cp
import {
    JitterKind,
    BackoffPolicy,
    BackoffState,
    makeU64,
    addWrap64,
    mulWrap64,
    nextRandU64,
    randRangeInclusive,
} from "backoff";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `JitterKind` | `None, Full, Equal, Decorrelated` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BackoffPolicy` | `class` | Fields: `initial_ms: u64, max_ms: u64, multiplier_num: u64, multiplier_den: u64, max_retries: u32, jitter: JitterKind` · Methods: `defaultPolicy` |
| `BackoffState` | `class` | Fields: `policy: BackoffPolicy, attempt: u32, prev_delay_ms: u64, rng_state: u64` · Methods: `init, nextDelayMs, reset, attempts` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | Invokes `makeU64` and returns `u64`. |
| `addWrap64` | `(a: u64, b: u64): u64` | Mutates or appends state via `addWrap64`. |
| `mulWrap64` | `(a: u64, b: u64): u64` | Invokes `mulWrap64` and returns `u64`. |
| `nextRandU64` | `(state: &mut u64): u64` | Invokes `nextRandU64` and returns `u64`. |
| `randRangeInclusive` | `(state: &mut u64, lo: u64, hi: u64): u64` | Invokes `randRangeInclusive` and returns `u64`. |
| `expDelayMs` | `(attempt: u32, initial_ms: u64, max_ms: u64): u64` | Invokes `expDelayMs` and returns `u64`. |
| `linearDelayMs` | `(attempt: u32, step_ms: u64, max_ms: u64): u64` | Invokes `linearDelayMs` and returns `u64`. |
| `computePolicyRawCap` | `(policy: BackoffPolicy, attempt: u32): u64` | Invokes `computePolicyRawCap` and returns `u64`. |

## Minimal Example

```cp
import { makeU64 } from "backoff";

pub fn main() {
    let result = makeU64(0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
