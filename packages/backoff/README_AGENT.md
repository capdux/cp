# backoff — AI Agent Quick Ref

Zero-allocation deterministic retry backoff engine with Exponential, Linear, and AWS Full/Equal/Decorrelated Jitter (Rust backoff / npm exponential-backoff parity) for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
enum JitterKind { None, Full, Equal, Decorrelated }
class BackoffPolicy { initial_ms: u64, max_ms: u64, multiplier_num: u64, multiplier_den: u64, /* +2 fields */ } // methods: defaultPolicy(...)
class BackoffState { policy: BackoffPolicy, attempt: u32, prev_delay_ms: u64, rng_state: u64 } // methods: init(...) +3
```

## API Signatures

```cp
fn makeU64(hi: u32, lo: u32): u64;
fn addWrap64(a: u64, b: u64): u64;
fn mulWrap64(a: u64, b: u64): u64;
fn nextRandU64(state: &mut u64): u64;
fn randRangeInclusive(state: &mut u64, lo: u64, hi: u64): u64;
fn expDelayMs(attempt: u32, initial_ms: u64, max_ms: u64): u64;
fn linearDelayMs(attempt: u32, step_ms: u64, max_ms: u64): u64;
fn computePolicyRawCap(policy: BackoffPolicy, attempt: u32): u64;
```

## Minimal Usage

```cp
import { makeU64 } from "backoff";

pub fn main() {
    let result = makeU64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"backoff"`; install via `cap add backoff`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
