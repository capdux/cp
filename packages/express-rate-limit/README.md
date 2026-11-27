[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# express-rate-limit

Zero-heap fixed-window HTTP rate limiter with IETF draft-7 RateLimit header formatting

| Metadata | Value |
| --- | --- |
| Package | `express-rate-limit` |
| Version | `7.5.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add express-rate-limit
```

```cp
import {
    RateLimitHit,
    RateLimitSlot,
    RateLimiter,
    erlEmptySlot,
    erlHashKey,
    erlSlotMatches,
    erlStoreSlot,
    erlCeilSeconds,
} from "express-rate-limit";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RateLimitHit` | `class` | Fields: `allowed: bool, limit: u32, current: u32, remaining: u32, reset_at_ms: i64, retry_after_sec: u32` |
| `RateLimitSlot` | `class` | Fields: `occupied: bool, hash: u64, key_len: usize, key_buf: [64]u8, count: u32, reset_at_ms: i64` |
| `RateLimiter` | `class` | Fields: `window_ms: i64, max_requests: u32, slots: [64]RateLimitSlot` · Methods: `init, hit, resetKey, resetAll, activeCount` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `erlEmptySlot` | `(): RateLimitSlot` | Invokes `erlEmptySlot` and returns `RateLimitSlot`. |
| `erlHashKey` | `(key: string): u64` | Invokes `erlHashKey` and returns `u64`. |
| `erlSlotMatches` | `(slot: RateLimitSlot, h: u64, key: string): bool` | Invokes `erlSlotMatches` and returns `bool`. |
| `erlStoreSlot` | `(h: u64, key: string, count: u32, reset_at_ms: i64): RateLimitSlot` | Invokes `erlStoreSlot` and returns `RateLimitSlot`. |
| `erlCeilSeconds` | `(reset_at_ms: i64, now_ms: i64): u32` | Invokes `erlCeilSeconds` and returns `u32`. |
| `erlWriteStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `erlWriteStr` and returns `usize`. |
| `erlWriteU32` | `(out: []u8, pos: usize, val: u32): usize` | Invokes `erlWriteU32` and returns `usize`. |
| `formatRateLimitHeader` | `(hit: RateLimitHit, out: []u8): usize` | Serializes/formats data via `formatRateLimitHeader` returning `usize`. |

## Minimal Example

```cp
import { erlHashKey } from "express-rate-limit";

pub fn main() {
    let result = erlHashKey("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
