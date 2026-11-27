# governor — AI Agent Quick Ref

Zero-allocation IETF HTTP RateLimit headers (RateLimit-Policy / RateLimit / X-RateLimit-*) parser, formatter, and reset delay calculator for CP. (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class RateLimitPolicy { quota: u64, window_secs: u64, burst: u64, has_burst: bool, /* +1 fields */ }
class RateLimitState { limit: u64, remaining: u64, reset_secs: u64, valid: bool }
```

## API Signatures

```cp
fn rhToLower(c: u8): u8;
fn rhIsOws(c: u8): bool;
fn rhTrimOws(s: &[]u8): &[]u8;
fn rhEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
fn rhFormatU64(val: u64, buf: []u8): usize;
fn rhCopyInto(dst: []u8, pos: &mut usize, src: &[]u8);
fn parseUintHeader(value: string): i64;
fn parsePolicy(header: string): RateLimitPolicy;
fn formatPolicy(quota: u64, window_secs: u64, out: []u8): usize;
fn parseStructuredRateLimit(header: string): RateLimitState;
fn formatStructuredRateLimit(state: RateLimitState, out: []u8): usize;
fn isExhausted(state: RateLimitState): bool;
fn computeResetDelaySecs(reset_val: u64, now_unix_secs: u64): u64;
```

## Minimal Usage

```cp
import { parseUintHeader } from "governor";

pub fn main() {
    let result = parseUintHeader("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"governor"`; install via `cap add governor`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
