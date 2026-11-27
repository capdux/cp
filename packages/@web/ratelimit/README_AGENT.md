# @web/ratelimit — AI Agent Quick Ref

Official zero-heap fixed-slot sliding-window rate limiter plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RateLimitConfig,
    RateLimitDecision,
    RateLimitSlot,
    RateLimiter,
    monotonicNowSec,
    formatU64,
    extractFirstToken,
    setTestClock,
} from "@web/ratelimit";
```

## Core Types & Constants

```cp
class RateLimitConfig { max_requests: u32, window_secs: u64, key_header: string }
class RateLimitDecision { allowed: bool, limit: u32, remaining: u32, retry_after: u64 }
class RateLimitSlot { used: bool, key_buf: [RATELIMIT_MAX_KEY_LEN]u8, key_len: usize, window_start: u64, /* +1 fields */ } // methods: matchesKey(...) +1
class RateLimiter { config: RateLimitConfig, slots: [RATELIMIT_MAX_SLOTS]RateLimitSlot } // methods: init(...) +2
```

## API Signatures

```cp
fn monotonicNowSec(): u64;
fn formatU64(val: u64, out_buf: []u8): string;
fn extractFirstToken(raw: string): string;
fn setTestClock(now_sec: u64);
fn resetGlobal();
fn rateLimitHook(ctx: &mut Context): bool;
fn createRateLimitPlugin(cfg: RateLimitConfig): Plugin;
fn registerRateLimit(app: &mut App, cfg: RateLimitConfig): &mut App;
```

## Minimal Usage

```cp
import { extractFirstToken } from "@web/ratelimit";

pub fn main() {
    let result = extractFirstToken("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/ratelimit"`; install via `cap add @web/ratelimit`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
