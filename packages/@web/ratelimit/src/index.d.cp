// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class RateLimitConfig {
    max_requests: u32,
    window_secs: u64,
    key_header: string,
}

export class RateLimitDecision {
    allowed: bool,
    limit: u32,
    remaining: u32,
    retry_after: u64,
}

export class RateLimitSlot {
    used: bool,
    key_buf: u8[RATELIMIT_MAX_KEY_LEN],
    key_len: usize,
    window_start: u64,
    count: u32,
    fn matchesKey(key: string): bool;
    fn setKey(_: any, key: string);
}

export class RateLimiter {
    config: RateLimitConfig,
    slots: RateLimitSlot[RATELIMIT_MAX_SLOTS],
    fn init(cfg: RateLimitConfig): RateLimiter;
    fn reset(_: any);
    fn check(_: any, key: string, now_sec: u64): RateLimitDecision;
}

export fn monotonicNowSec(): u64;
export fn formatU64(val: u64, out_buf: []u8): string;
export fn extractFirstToken(raw: string): string;
export fn setTestClock(now_sec: u64);
export fn resetGlobal();
export fn rateLimitHook(ctx: &mut Context): bool;
export fn createRateLimitPlugin(cfg: RateLimitConfig): Plugin;
export fn registerRateLimit(app: &mut App, cfg: RateLimitConfig): &mut App;
