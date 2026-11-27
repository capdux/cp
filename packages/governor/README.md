[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# governor

Zero-allocation IETF HTTP RateLimit headers (RateLimit-Policy / RateLimit / X-RateLimit-*) parser, formatter, and reset delay calculator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:governor` ([https://github.com/boinkor-net/governor](https://github.com/boinkor-net/governor)) / `crates.io:governor` ([https://github.com/boinkor-net/governor](https://github.com/boinkor-net/governor)) / `npm:express-rate-limit` ([https://github.com/express-rate-limit/express-rate-limit](https://github.com/express-rate-limit/express-rate-limit))（原库名 `governor` → CP 包名 `governor`）

| Metadata | Value |
| --- | --- |
| Package | `governor` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add governor
```

```cp
import {
    RateLimitPolicy,
    RateLimitState,
    rhToLower,
    rhIsOws,
    rhTrimOws,
    rhEqIgnoreCase,
    rhFormatU64,
    rhCopyInto,
} from "governor";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RateLimitPolicy` | `class` | Fields: `quota: u64, window_secs: u64, burst: u64, has_burst: bool, valid: bool` |
| `RateLimitState` | `class` | Fields: `limit: u64, remaining: u64, reset_secs: u64, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rhToLower` | `(c: u8): u8` | Invokes `rhToLower` and returns `u8`. |
| `rhIsOws` | `(c: u8): bool` | Invokes `rhIsOws` and returns `bool`. |
| `rhTrimOws` | `(s: &[]u8): &[]u8` | Invokes `rhTrimOws` and returns `&[]u8`. |
| `rhEqIgnoreCase` | `(a: &[]u8, b: &[]u8): bool` | Invokes `rhEqIgnoreCase` and returns `bool`. |
| `rhFormatU64` | `(val: u64, buf: []u8): usize` | Invokes `rhFormatU64` and returns `usize`. |
| `rhCopyInto` | `(dst: []u8, pos: &mut usize, src: &[]u8)` | Executes `rhCopyInto` in-place. |
| `parseUintHeader` | `(value: string): i64` | Parses input via `parseUintHeader` and returns `i64`. |
| `parsePolicy` | `(header: string): RateLimitPolicy` | Parses input via `parsePolicy` and returns `RateLimitPolicy`. |
| `formatPolicy` | `(quota: u64, window_secs: u64, out: []u8): usize` | Serializes/formats data via `formatPolicy` returning `usize`. |
| `parseStructuredRateLimit` | `(header: string): RateLimitState` | Parses input via `parseStructuredRateLimit` and returns `RateLimitState`. |
| `formatStructuredRateLimit` | `(state: RateLimitState, out: []u8): usize` | Serializes/formats data via `formatStructuredRateLimit` returning `usize`. |
| `isExhausted` | `(state: RateLimitState): bool` | Checks predicate `isExhausted` and returns `bool`. |
| `computeResetDelaySecs` | `(reset_val: u64, now_unix_secs: u64): u64` | Invokes `computeResetDelaySecs` and returns `u64`. |

## Minimal Example

```cp
import { parseUintHeader } from "governor";

pub fn main() {
    let result = parseUintHeader("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
