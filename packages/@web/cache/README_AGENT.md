# @web/cache — AI Agent Quick Ref

Official zero-heap response caching and Stripe-style Idempotency-Key plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CacheConfig,
    CacheEntry,
    cacheMonotonicNowSec,
    cacheNowSec,
    cacheFormatCacheControl,
    cacheCopyInto,
    cacheBuildGetKey,
    cacheBuildIdemKey,
} from "@web/cache";
```

## Core Types & Constants

```cp
class CacheConfig { ttl_secs: u64, max_entries: usize, enable_idempotency: bool, idempotency_header: string }
class CacheEntry { used: bool, is_idem: bool, key_buf: [128]u8, key_len: usize, /* +6 fields */ } // methods: matches(...) +2
```

## API Signatures

```cp
fn cacheMonotonicNowSec(): u64;
fn cacheNowSec(): u64;
fn cacheFormatCacheControl(ttl: u64): string;
fn cacheCopyInto(dst: []u8, src: string): usize;
fn cacheBuildGetKey(ctx: &mut Context, out_buf: []u8): string;
fn cacheBuildIdemKey(ctx: &mut Context, idem_val: string, out_buf: []u8): string;
fn cacheIsWriteMethod(m: AppMethod): bool;
fn cacheEffectiveCap(): usize;
fn cacheFindEntry(key: string, is_idem: bool, now_sec: u64): usize?;
fn cachePutEntry(key: string, is_idem: bool, resp: &mut Response, now_sec: u64);
fn setCacheTestClock(now_sec: u64);
fn resetCacheStore();
fn cacheBeforeHandleHook(ctx: &mut Context): bool;
fn cacheAfterHandleHook(ctx: &mut Context, resp: &mut Response);
fn createCachePlugin(cfg: CacheConfig): Plugin;
fn registerCache(app: &mut App, cfg: CacheConfig): &mut App;
```

## Minimal Usage

```cp
import { cacheCopyInto } from "@web/cache";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = cacheCopyInto(out_buf[0..256], "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/cache"`; install via `cap add @web/cache`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
