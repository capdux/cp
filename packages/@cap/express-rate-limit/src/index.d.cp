// Type definitions for CP

export class RateLimitHit {
    allowed: bool,
    limit: u32,
    current: u32,
    remaining: u32,
    reset_at_ms: i64,
    retry_after_sec: u32,
}

export class RateLimitSlot {
    occupied: bool,
    hash: u64,
    key_len: usize,
    key_buf: [64]u8,
    count: u32,
    reset_at_ms: i64,
}

export fn erlEmptySlot(): RateLimitSlot;
export fn erlHashKey(key: string): u64;
export fn erlSlotMatches(slot: RateLimitSlot, h: u64, key: string): bool;
export fn erlStoreSlot(h: u64, key: string, count: u32, reset_at_ms: i64): RateLimitSlot;
export fn erlCeilSeconds(reset_at_ms: i64, now_ms: i64): u32;
export fn erlWriteStr(out: []u8, pos: usize, s: string): usize;
export fn erlWriteU32(out: []u8, pos: usize, val: u32): usize;
export class RateLimiter {
    window_ms: i64,
    max_requests: u32,
    slots: [64]RateLimitSlot,
    fn init(window_ms: i64, max_requests: u32): RateLimiter;
    fn hit(_: auto, key: string, now_ms: i64): RateLimitHit;
    fn resetKey(_: auto, key: string);
    fn resetAll(_: auto);
    fn activeCount(now_ms: i64): usize;
}

export fn formatRateLimitHeader(hit: RateLimitHit, out: []u8): usize;
