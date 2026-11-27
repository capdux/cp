# @web/cache@1.0.0 (Agent Quickref)

> Official zero-heap response caching and Stripe-style Idempotency-Key plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/cache";` | Full types: `src/index.d.cp`

## Functions
- `export fn cacheMonotonicNowSec(): u64;`
- `export fn cacheNowSec(): u64;`
- `export fn cacheFormatCacheControl(ttl: u64): string;`
- `export fn cacheCopyInto(dst: []u8, src: string): usize;`
- `export fn cacheBuildGetKey(ctx: &mut Context, out_buf: []u8): string;`
- `export fn cacheBuildIdemKey(ctx: &mut Context, idem_val: string, out_buf: []u8): string;`
- `export fn cacheIsWriteMethod(m: AppMethod): bool;`
- `export fn cacheEffectiveCap(): usize;`
- `export fn cacheFindEntry(key: string, is_idem: bool, now_sec: u64): usize?;`
- `export fn cachePutEntry(key: string, is_idem: bool, resp: &mut Response, now_sec: u64);`
- `export fn setCacheTestClock(now_sec: u64);`
- `export fn resetCacheStore();`
- `export fn cacheBeforeHandleHook(ctx: &mut Context): bool;`
- `export fn cacheAfterHandleHook(ctx: &mut Context, resp: &mut Response);`
- `export fn createCachePlugin(cfg: CacheConfig): Plugin;`
- `export fn registerCache(app: &mut App, cfg: CacheConfig): &mut App;`
