[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/cache

Official zero-heap response caching and Stripe-style Idempotency-Key plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/cache` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/cache
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CacheConfig` | `class` | Fields: `ttl_secs: u64, max_entries: usize, enable_idempotency: bool, idempotency_header: string` |
| `CacheEntry` | `class` | Fields: `used: bool, is_idem: bool, key_buf: [128]u8, key_len: usize, status: u16, ct_buf: [64]u8, +4 more` · Methods: `matches, bodySlice, ctSlice` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cacheMonotonicNowSec` | `(): u64` | Invokes `cacheMonotonicNowSec` and returns `u64`. |
| `cacheNowSec` | `(): u64` | Invokes `cacheNowSec` and returns `u64`. |
| `cacheFormatCacheControl` | `(ttl: u64): string` | Invokes `cacheFormatCacheControl` and returns `string`. |
| `cacheCopyInto` | `(dst: []u8, src: string): usize` | Invokes `cacheCopyInto` and returns `usize`. |
| `cacheBuildGetKey` | `(ctx: &mut Context, out_buf: []u8): string` | Invokes `cacheBuildGetKey` and returns `string`. |
| `cacheBuildIdemKey` | `(ctx: &mut Context, idem_val: string, out_buf: []u8): string` | Invokes `cacheBuildIdemKey` and returns `string`. |
| `cacheIsWriteMethod` | `(m: AppMethod): bool` | Invokes `cacheIsWriteMethod` and returns `bool`. |
| `cacheEffectiveCap` | `(): usize` | Invokes `cacheEffectiveCap` and returns `usize`. |
| `cacheFindEntry` | `(key: string, is_idem: bool, now_sec: u64): usize?` | Invokes `cacheFindEntry` and returns `usize?`. |
| `cachePutEntry` | `(key: string, is_idem: bool, resp: &mut Response, now_sec: u64)` | Executes `cachePutEntry` in-place. |
| `setCacheTestClock` | `(now_sec: u64)` | Mutates or appends state via `setCacheTestClock`. |
| `resetCacheStore` | `()` | Executes `resetCacheStore` in-place. |
| `cacheBeforeHandleHook` | `(ctx: &mut Context): bool` | Invokes `cacheBeforeHandleHook` and returns `bool`. |
| `cacheAfterHandleHook` | `(ctx: &mut Context, resp: &mut Response)` | Executes `cacheAfterHandleHook` in-place. |
| `createCachePlugin` | `(cfg: CacheConfig): Plugin` | Constructs/initializes an instance via `createCachePlugin`. |
| `registerCache` | `(app: &mut App, cfg: CacheConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { cacheCopyInto } from "@web/cache";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = cacheCopyInto(out_buf[0..256], "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
