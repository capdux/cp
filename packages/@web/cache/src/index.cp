// Official @web/cache Plugin for `cap:web`
// Zero-heap fixed-slot GET response cache & Stripe-style Idempotency-Key store.

const std = @import("std");
import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub const CACHE_MAX_SLOTS: usize = 64;
pub const CACHE_MAX_KEY_LEN: usize = 128;
pub const CACHE_MAX_CT_LEN: usize = 64;
pub const CACHE_MAX_BODY_LEN: usize = 1024;

pub struct CacheConfig {
    ttl_secs: u64 = 60;
    max_entries: usize = 64;
    enable_idempotency: bool = true;
    idempotency_header: string = "Idempotency-Key";
}

pub struct CacheEntry {
    used: bool = false;
    is_idem: bool = false;
    key_buf: [128]u8 = [0; 128];
    key_len: usize = 0;
    status: u16 = 200;
    ct_buf: [64]u8 = [0; 64];
    ct_len: usize = 0;
    body_buf: [1024]u8 = [0; 1024];
    body_len: usize = 0;
    expires_at: u64 = 0;

    fn matches(key: string, idem: bool): bool {
        if (!_.used || _.is_idem != idem || _.key_len != key.len) return false;
        let i: usize = 0;
        while (i < key.len) : (i += 1) {
            if (_.key_buf[i] != key[i]) return false;
        }
        return true;
    }

    fn bodySlice(): string {
        return _.body_buf[0.._.body_len];
    }

    fn ctSlice(): string {
        return _.ct_buf[0.._.ct_len];
    }
}

let g_cache_cfg: CacheConfig = CacheConfig{};
let g_cache_slots: [64]CacheEntry = [CacheEntry{}; 64];
let g_cache_test_clock: u64 = 0;
let g_cache_cc_buf: [48]u8 = [0; 48];
let g_cache_cc_len: usize = 0;

fn cacheMonotonicNowSec(): u64 {
    const cache_builtin = @import("builtin");
    if (comptime cache_builtin.os.tag == .linux) {
        let ts: std.os.linux.timespec = undefined;
        * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        return if (ts.sec > 0) @intCast(ts.sec) else 1;
    }
    let ts: std.c.timespec = undefined;
    * = std.c.clock_gettime(.MONOTONIC, &ts);
    return if (ts.sec > 0) @intCast(ts.sec) else 1;
}

fn cacheNowSec(): u64 {
    return if (g_cache_test_clock > 0) g_cache_test_clock else cacheMonotonicNowSec();
}

fn cacheFormatCacheControl(ttl: u64): string {
    const prefix = "public, max-age=";
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len && pos < g_cache_cc_buf.len) : (i += 1) {
        g_cache_cc_buf[pos] = prefix[i];
        pos += 1;
    }
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = ttl;
    if (v == 0) {
        g_cache_cc_buf[pos] = '0';
        pos += 1;
    } else {
        while (v > 0) {
            tmp[n] = (48 + @as(u8, @truncate(v % 10))) as u8;
            n += 1;
            v = v / 10;
        }
        while (n > 0 && pos < g_cache_cc_buf.len) {
            n -= 1;
            g_cache_cc_buf[pos] = tmp[n];
            pos += 1;
        }
    }
    g_cache_cc_len = pos;
    return g_cache_cc_buf[0..g_cache_cc_len];
}

fn cacheCopyInto(dst: []u8, src: string): usize {
    const n = if (src.len < dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

fn cacheBuildGetKey(ctx: *Context, out_buf: []u8): string {
    let pos: usize = cacheCopyInto(out_buf, ctx.path);
    if (ctx.raw_query.len > 0 && pos < out_buf.len) {
        out_buf[pos] = '?';
        pos += 1;
        pos += cacheCopyInto(out_buf[pos..], ctx.raw_query);
    }
    return out_buf[0..pos];
}

fn cacheBuildIdemKey(ctx: *Context, idem_val: string, out_buf: []u8): string {
    let pos: usize = cacheCopyInto(out_buf, ctx.method.asString());
    if (pos < out_buf.len) {
        out_buf[pos] = ':';
        pos += 1;
    }
    pos += cacheCopyInto(out_buf[pos..], ctx.path);
    if (pos < out_buf.len) {
        out_buf[pos] = ':';
        pos += 1;
    }
    pos += cacheCopyInto(out_buf[pos..], idem_val);
    return out_buf[0..pos];
}

fn cacheIsWriteMethod(m: AppMethod): bool {
    return m == AppMethod.POST || m == AppMethod.PUT || m == AppMethod.PATCH;
}

fn cacheEffectiveCap(): usize {
    if (g_cache_cfg.max_entries == 0) return 1;
    return if (g_cache_cfg.max_entries < CACHE_MAX_SLOTS) g_cache_cfg.max_entries else CACHE_MAX_SLOTS;
}

fn cacheFindEntry(key: string, is_idem: bool, now_sec: u64): usize? {
    const cap = cacheEffectiveCap();
    let i: usize = 0;
    while (i < cap) : (i += 1) {
        if (g_cache_slots[i].matches(key, is_idem)) {
            if (now_sec < g_cache_slots[i].expires_at) {
                return i;
            }
            // Expired entry: reclaim lazily
            g_cache_slots[i].used = false;
            return null;
        }
    }
    return null;
}

fn cachePutEntry(key: string, is_idem: bool, resp: *Response, now_sec: u64) {
    const body: string = if (resp.body_buf_len > 0)
        resp.body_buf[0..resp.body_buf_len]
    else
        resp.body;

    // Skip caching if payload exceeds fixed slot capacity
    if (body.len > CACHE_MAX_BODY_LEN) return;

    const cap = cacheEffectiveCap();
    let target_idx: usize? = null;
    let free_idx: usize? = null;
    let oldest_idx: usize = 0;
    let oldest_exp: u64 = std.math.maxInt(u64);

    let i: usize = 0;
    while (i < cap) : (i += 1) {
        if (g_cache_slots[i].matches(key, is_idem)) {
            target_idx = i;
            break;
        }
        if (!g_cache_slots[i].used && free_idx == null) {
            free_idx = i;
        } else if (g_cache_slots[i].used) {
            if (now_sec >= g_cache_slots[i].expires_at && free_idx == null) {
                free_idx = i;
            }
            if (g_cache_slots[i].expires_at < oldest_exp) {
                oldest_exp = g_cache_slots[i].expires_at;
                oldest_idx = i;
            }
        }
    }

    let idx: usize = oldest_idx;
    if (t := target_idx) {
        idx = t;
    } else if (f := free_idx) {
        idx = f;
    }
    const ttl: u64 = if (g_cache_cfg.ttl_secs == 0) 1 else g_cache_cfg.ttl_secs;

    g_cache_slots[idx].used = true;
    g_cache_slots[idx].is_idem = is_idem;
    g_cache_slots[idx].key_len = cacheCopyInto(g_cache_slots[idx].key_buf[0..], key);
    g_cache_slots[idx].status = resp.status;
    g_cache_slots[idx].ct_len = cacheCopyInto(g_cache_slots[idx].ct_buf[0..], resp.content_type);
    g_cache_slots[idx].body_len = cacheCopyInto(g_cache_slots[idx].body_buf[0..], body);
    g_cache_slots[idx].expires_at = now_sec + ttl;
}

/// Optional deterministic clock override for unit tests (`0` restores monotonic clock).
pub fn setCacheTestClock(now_sec: u64) {
    g_cache_test_clock = now_sec;
}

/// Clear all entries in the global cache / idempotency store.
pub fn resetCacheStore() {
    let i: usize = 0;
    while (i < CACHE_MAX_SLOTS) : (i += 1) {
        g_cache_slots[i] = CacheEntry{};
    }
}

/// `onBeforeHandle` hook: serves cached GET responses (`X-Cache: HIT`)
/// and replays cached `Idempotency-Key` mutations (`X-Idempotency-Replayed: true`).
pub fn cacheBeforeHandleHook(ctx: *Context): bool {
    const now_sec = cacheNowSec();
    let key_buf: [128]u8 = [0; 128];

    if (ctx.method == AppMethod.GET) {
        const key = cacheBuildGetKey(ctx, key_buf[0..]);
        if (idx := cacheFindEntry(key, false, now_sec)) {
            let resp = Response.bytes(
                g_cache_slots[idx].status,
                g_cache_slots[idx].bodySlice(),
                g_cache_slots[idx].ctSlice(),
            );
            * = resp.setHeader("X-Cache", "HIT");
            * = resp.setHeader("Cache-Control", cacheFormatCacheControl(g_cache_cfg.ttl_secs));
            ctx.abort(resp);
            return false;
        }
        return true;
    }

    if (g_cache_cfg.enable_idempotency && cacheIsWriteMethod(ctx.method)) {
        if (idem_val := ctx.header(g_cache_cfg.idempotency_header)) {
            if (idem_val.len > 0) {
                const key = cacheBuildIdemKey(ctx, idem_val, key_buf[0..]);
                if (idx := cacheFindEntry(key, true, now_sec)) {
                    let resp = Response.bytes(
                        g_cache_slots[idx].status,
                        g_cache_slots[idx].bodySlice(),
                        g_cache_slots[idx].ctSlice(),
                    );
                    * = resp.setHeader("X-Idempotency-Replayed", "true");
                    ctx.abort(resp);
                    return false;
                }
            }
        }
    }

    return true;
}

/// `onAfterHandle` hook: stores GET responses (`X-Cache: MISS` + `Cache-Control`)
/// and persists first-run `Idempotency-Key` mutation responses.
pub fn cacheAfterHandleHook(ctx: *Context, resp: *Response) {
    // Do not overwrite cache or headers on cache HIT / replay / aborted responses
    if (ctx.aborted) return;

    const now_sec = cacheNowSec();
    let key_buf: [128]u8 = [0; 128];

    if (ctx.method == AppMethod.GET) {
        if (resp.status >= 200 && resp.status < 300) {
            const key = cacheBuildGetKey(ctx, key_buf[0..]);
            cachePutEntry(key, false, resp, now_sec);
            * = resp.setHeader("X-Cache", "MISS");
            * = resp.setHeader("Cache-Control", cacheFormatCacheControl(g_cache_cfg.ttl_secs));
        }
        return;
    }

    if (g_cache_cfg.enable_idempotency && cacheIsWriteMethod(ctx.method)) {
        if (idem_val := ctx.header(g_cache_cfg.idempotency_header)) {
            if (idem_val.len > 0) {
                const key = cacheBuildIdemKey(ctx, idem_val, key_buf[0..]);
                cachePutEntry(key, true, resp, now_sec);
            }
        }
    }
}

/// Build a reusable `cap:web` `Plugin` for `@web/cache`.
pub fn createCachePlugin(cfg: CacheConfig): Plugin {
    g_cache_cfg = cfg;
    let p = Plugin.init("@web/cache");
    * = p.withScope(PluginScope.Global)
        .onBeforeHandle(cacheBeforeHandleHook)
        .onAfterHandle(cacheAfterHandleHook);
    return p;
}

/// One-call registration onto a `cap:web` `App`.
pub fn registerCache(app: *App, cfg: CacheConfig): *App {
    const p = createCachePlugin(cfg);
    return app.usePlugin(&p);
}
