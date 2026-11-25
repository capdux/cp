// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class CacheConfig {
    ttl_secs: u64,
    max_entries: usize,
    enable_idempotency: bool,
    idempotency_header: string,
}

export class CacheEntry {
    used: bool,
    is_idem: bool,
    key_buf: u8[128],
    key_len: usize,
    status: u16,
    ct_buf: u8[64],
    ct_len: usize,
    body_buf: u8[1024],
    body_len: usize,
    expires_at: u64,
    fn matches(key: string, idem: bool): bool;
    fn bodySlice(): string;
    fn ctSlice(): string;
}

export fn cacheMonotonicNowSec(): u64;
export fn cacheNowSec(): u64;
export fn cacheFormatCacheControl(ttl: u64): string;
export fn cacheCopyInto(dst: []u8, src: string): usize;
export fn cacheBuildGetKey(ctx: &mut Context, out_buf: []u8): string;
export fn cacheBuildIdemKey(ctx: &mut Context, idem_val: string, out_buf: []u8): string;
export fn cacheIsWriteMethod(m: AppMethod): bool;
export fn cacheEffectiveCap(): usize;
export fn cacheFindEntry(key: string, is_idem: bool, now_sec: u64): ?usize;
export fn cachePutEntry(key: string, is_idem: bool, resp: &mut Response, now_sec: u64);
export fn setCacheTestClock(now_sec: u64);
export fn resetCacheStore();
export fn cacheBeforeHandleHook(ctx: &mut Context): bool;
export fn cacheAfterHandleHook(ctx: &mut Context, resp: &mut Response);
export fn createCachePlugin(cfg: CacheConfig): Plugin;
export fn registerCache(app: &mut App, cfg: CacheConfig): &mut App;
