# express-rate-limit — AI Agent Quick Ref

Zero-heap fixed-window HTTP rate limiter with IETF draft-7 RateLimit header formatting (`v7.5.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class RateLimitHit { allowed: bool, limit: u32, current: u32, remaining: u32, /* +2 fields */ }
class RateLimitSlot { occupied: bool, hash: u64, key_len: usize, key_buf: [64]u8, /* +2 fields */ }
class RateLimiter { window_ms: i64, max_requests: u32, slots: [64]RateLimitSlot } // methods: init(...) +4
```

## API Signatures

```cp
fn erlEmptySlot(): RateLimitSlot;
fn erlHashKey(key: string): u64;
fn erlSlotMatches(slot: RateLimitSlot, h: u64, key: string): bool;
fn erlStoreSlot(h: u64, key: string, count: u32, reset_at_ms: i64): RateLimitSlot;
fn erlCeilSeconds(reset_at_ms: i64, now_ms: i64): u32;
fn erlWriteStr(out: []u8, pos: usize, s: string): usize;
fn erlWriteU32(out: []u8, pos: usize, val: u32): usize;
fn formatRateLimitHeader(hit: RateLimitHit, out: []u8): usize;
```

## Minimal Usage

```cp
import { erlHashKey } from "express-rate-limit";

pub fn main() {
    let result = erlHashKey("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"express-rate-limit"`; install via `cap add express-rate-limit`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
