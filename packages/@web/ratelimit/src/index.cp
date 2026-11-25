// Origin: Official @web/ratelimit Plugin (Zero-Heap Fixed-Slot Limiter) | Created: 2026-11-25
// ============================================================================
// @web/ratelimit — Zero-Heap Fixed-Slot Sliding/Fixed Window Rate Limiter
//   - Key dimensions: configured header (default X-Forwarded-For) -> X-Real-IP -> "global"
//   - Automatic `RateLimit-Limit` / `RateLimit-Remaining` response headers
//   - HTTP 429 Too Many Requests + `Retry-After` header + JSON error body on breach
// ============================================================================

const std = @import("std");
import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub const RATELIMIT_MAX_SLOTS: usize = 64;
pub const RATELIMIT_MAX_KEY_LEN: usize = 64;

pub struct RateLimitConfig {
    max_requests: u32 = 100;
    window_secs: u64 = 60;
    key_header: string = "X-Forwarded-For";
}

pub struct RateLimitDecision {
    allowed: bool = true;
    limit: u32 = 100;
    remaining: u32 = 100;
    retry_after: u64 = 0;
}

pub struct RateLimitSlot {
    used: bool = false;
    key_buf: [RATELIMIT_MAX_KEY_LEN]u8 = [0; RATELIMIT_MAX_KEY_LEN];
    key_len: usize = 0;
    window_start: u64 = 0;
    count: u32 = 0;

    fn matchesKey(key: string): bool {
        if (!_.used || _.key_len != key.len) return false;
        let i: usize = 0;
        while (i < key.len) : (i += 1) {
            if (_.key_buf[i] != key[i]) return false;
        }
        return true;
    }

    fn setKey(*_, key: string) {
        const n = if (key.len < RATELIMIT_MAX_KEY_LEN) key.len else RATELIMIT_MAX_KEY_LEN;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            _.key_buf[i] = key[i];
        }
        _.key_len = n;
        _.used = true;
    }
}

pub struct RateLimiter {
    config: RateLimitConfig = RateLimitConfig{};
    slots: [RATELIMIT_MAX_SLOTS]RateLimitSlot = [RateLimitSlot{}; RATELIMIT_MAX_SLOTS];

    pub static fn init(cfg: RateLimitConfig): RateLimiter {
        return RateLimiter{
            config: cfg,
            slots: [RateLimitSlot{}; RATELIMIT_MAX_SLOTS],
        };
    }

    pub fn reset(*_) {
        let i: usize = 0;
        while (i < RATELIMIT_MAX_SLOTS) : (i += 1) {
            _.slots[i] = RateLimitSlot{};
        }
    }

    /// Evaluate and record one request for `key` at timestamp `now_sec`.
    pub fn check(*_, key: string, now_sec: u64): RateLimitDecision {
        const eff_key = if (key.len == 0) "global" else if (key.len > RATELIMIT_MAX_KEY_LEN) key[0..RATELIMIT_MAX_KEY_LEN] else key;
        const win: u64 = if (_.config.window_secs == 0) 1 else _.config.window_secs;
        const limit: u32 = _.config.max_requests;

        let slot_idx: usize? = null;
        let free_idx: usize? = null;
        let oldest_idx: usize = 0;
        let oldest_start: u64 = std.math.maxInt(u64);

        let i: usize = 0;
        while (i < RATELIMIT_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].matchesKey(eff_key)) {
                slot_idx = i;
                break;
            }
            if (!_.slots[i].used && free_idx == null) {
                free_idx = i;
            } else if (_.slots[i].used) {
                if (now_sec >= _.slots[i].window_start + win && free_idx == null) {
                    free_idx = i;
                }
                if (_.slots[i].window_start < oldest_start) {
                    oldest_start = _.slots[i].window_start;
                    oldest_idx = i;
                }
            }
        }

        let idx: usize = 0;
        if (found := slot_idx) {
            idx = found;
        } else if (free_slot := free_idx) {
            idx = free_slot;
            _.slots[idx].setKey(eff_key);
            _.slots[idx].window_start = now_sec;
            _.slots[idx].count = 0;
        } else {
            idx = oldest_idx;
            _.slots[idx].setKey(eff_key);
            _.slots[idx].window_start = now_sec;
            _.slots[idx].count = 0;
        }

        if (now_sec >= _.slots[idx].window_start + win) {
            _.slots[idx].window_start = now_sec;
            _.slots[idx].count = 0;
        }

        if (_.slots[idx].count < limit) {
            _.slots[idx].count += 1;
            const rem: u32 = limit - _.slots[idx].count;
            return RateLimitDecision{
                allowed: true,
                limit: limit,
                remaining: rem,
                retry_after: 0,
            };
        }

        const window_end = _.slots[idx].window_start + win;
        const retry: u64 = if (window_end > now_sec) (window_end - now_sec) else 1;
        return RateLimitDecision{
            allowed: false,
            limit: limit,
            remaining: 0,
            retry_after: retry,
        };
    }
}

// ============================================================================
// Internal Helpers & Module-Level Zero-Heap State
// ============================================================================

fn monotonicNowSec(): u64 {
    const rl_builtin = @import("builtin");
    if (comptime rl_builtin.os.tag == .linux) {
        let ts: std.os.linux.timespec = undefined;
        * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        return if (ts.sec > 0) @intCast(ts.sec) else 1;
    }
    let ts: std.c.timespec = undefined;
    * = std.c.clock_gettime(.MONOTONIC, &ts);
    return if (ts.sec > 0) @intCast(ts.sec) else 1;
}

fn formatU64(val: u64, out_buf: []u8): string {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        out_buf[0] = '0';
        return out_buf[0..1];
    }
    while (v > 0) {
        tmp[n] = (48 + @as(u8, @truncate(v % 10))) as u8;
        n += 1;
        v = v / 10;
    }
    let pos: usize = 0;
    while (n > 0 && pos < out_buf.len) {
        n -= 1;
        out_buf[pos] = tmp[n];
        pos += 1;
    }
    return out_buf[0..pos];
}

fn extractFirstToken(raw: string): string {
    let start: usize = 0;
    while (start < raw.len && (raw[start] == ' ' || raw[start] == '\t')) : (start += 1) {}
    let end: usize = start;
    while (end < raw.len && raw[end] != ',') : (end += 1) {}
    while (end > start && (raw[end - 1] == ' ' || raw[end - 1] == '\t')) : (end -= 1) {}
    return raw[start..end];
}

let g_limiter: RateLimiter = RateLimiter{};
let g_test_clock_sec: u64 = 0;
let g_limit_hdr_buf: [24]u8 = [0; 24];
let g_rem_hdr_buf: [24]u8 = [0; 24];
let g_retry_hdr_buf: [24]u8 = [0; 24];

/// Optional deterministic clock override for unit tests (`0` restores monotonic clock).
pub fn setTestClock(now_sec: u64) {
    g_test_clock_sec = now_sec;
}

/// Reset the global rate limiter table (useful between test cases).
pub fn resetGlobal() {
    g_limiter.reset();
}

pub fn rateLimitHook(ctx: *Context): bool {
    let key: string = "global";
    if (g_limiter.config.key_header.len > 0) {
        if (h := ctx.header(g_limiter.config.key_header)) {
            const first = extractFirstToken(h);
            if (first.len > 0) {
                key = first;
            }
        }
    }
    if (key.len == 6 && key[0] == 'g' && key[1] == 'l' && key[2] == 'o' && key[3] == 'b' && key[4] == 'a' && key[5] == 'l') {
        if (real_ip := ctx.header("X-Real-IP")) {
            const trimmed = extractFirstToken(real_ip);
            if (trimmed.len > 0) {
                key = trimmed;
            }
        }
    }

    const now_sec: u64 = if (g_test_clock_sec > 0) g_test_clock_sec else monotonicNowSec();
    const decision = g_limiter.check(key, now_sec);

    const limit_str = formatU64(@as(u64, decision.limit), g_limit_hdr_buf[0..]);
    const rem_str = formatU64(@as(u64, decision.remaining), g_rem_hdr_buf[0..]);

    * = ctx.setHeader("RateLimit-Limit", limit_str);
    * = ctx.setHeader("RateLimit-Remaining", rem_str);

    if (!decision.allowed) {
        const retry_str = formatU64(decision.retry_after, g_retry_hdr_buf[0..]);
        let resp = Response.json(429, "{\"error\":\"Too Many Requests\",\"message\":\"Rate limit exceeded\"}");
        * = resp.setHeader("Retry-After", retry_str);
        * = resp.setHeader("RateLimit-Limit", limit_str);
        * = resp.setHeader("RateLimit-Remaining", "0");
        ctx.abort(resp);
        return false;
    }

    return true;
}

/// Create a composable Elysia-style `Plugin` for `@web/ratelimit`.
pub fn createRateLimitPlugin(cfg: RateLimitConfig): Plugin {
    g_limiter = RateLimiter.init(cfg);
    let p = Plugin.init("@web/ratelimit");
    * = p.withScope(PluginScope.Global);
    * = p.onRequest(rateLimitHook);
    return p;
}

/// Register `@web/ratelimit` directly onto `app` (fluent chainable).
pub fn registerRateLimit(app: *App, cfg: RateLimitConfig): *App {
    const p = createRateLimitPlugin(cfg);
    return app.usePlugin(&p);
}
