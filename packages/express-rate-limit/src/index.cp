// @cap/express-rate-limit — Zero-heap fixed-window HTTP rate limiter
// Clean-room CP implementation inspired by express-rate-limit (MIT).

const ERL_CAPACITY: usize = 64;
const ERL_KEY_MAX: usize = 64;

pub struct RateLimitHit {
    allowed: bool,
    limit: u32,
    current: u32,
    remaining: u32,
    reset_at_ms: i64,
    retry_after_sec: u32,
}

pub struct RateLimitSlot {
    occupied: bool,
    hash: u64,
    key_len: usize,
    key_buf: [64]u8,
    count: u32,
    reset_at_ms: i64,
}

fn erlEmptySlot(): RateLimitSlot {
    return RateLimitSlot{
        occupied: false,
        hash: 0,
        key_len: 0,
        key_buf: [0 as u8; 64],
        count: 0,
        reset_at_ms: 0,
    };
}

fn erlHashKey(key: string): u64 {
    // FNV-1a 64-bit offset basis 0xcbf29ce4_84222325
    let h: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
    const prime: u64 = ((0x00000100 as u64) << 32) | (0x000001b3 as u64);
    let i: usize = 0;
    while (i < key.len) : (i += 1) {
        h ^= (key[i] as u64);
        h *%= prime;
    }
    return h;
}

fn erlSlotMatches(slot: RateLimitSlot, h: u64, key: string): bool {
    if (!slot.occupied) return false;
    if (slot.hash != h) return false;
    if (slot.key_len != key.len) return false;
    const cmp_len: usize = if (key.len < ERL_KEY_MAX) key.len else ERL_KEY_MAX;
    let i: usize = 0;
    while (i < cmp_len) : (i += 1) {
        if (slot.key_buf[i] != key[i]) return false;
    }
    return true;
}

fn erlStoreSlot(h: u64, key: string, count: u32, reset_at_ms: i64): RateLimitSlot {
    let slot = erlEmptySlot();
    slot.occupied = true;
    slot.hash = h;
    slot.key_len = key.len;
    slot.count = count;
    slot.reset_at_ms = reset_at_ms;
    const copy_len: usize = if (key.len < ERL_KEY_MAX) key.len else ERL_KEY_MAX;
    let i: usize = 0;
    while (i < copy_len) : (i += 1) {
        slot.key_buf[i] = key[i];
    }
    return slot;
}

fn erlCeilSeconds(reset_at_ms: i64, now_ms: i64): u32 {
    if (reset_at_ms <= now_ms) return 0;
    const diff_ms: u64 = u64(reset_at_ms - now_ms);
    const secs: u64 = (diff_ms + 999) / 1000;
    if (secs > 0xFFFFFFFF) return 0xFFFFFFFF;
    return u32(secs);
}

fn erlWriteStr(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn erlWriteU32(out: []u8, pos: usize, val: u32): usize {
    if (pos >= out.len) return pos;
    if (val == 0) {
        out[pos] = '0';
        return pos + 1;
    }
    let tmp: [16]u8 = [0 as u8; 16];
    let n: usize = 0;
    let v: u32 = val;
    while (v > 0) {
        const digit: u32 = v % 10;
        tmp[n] = u8(digit) + '0';
        n += 1;
        v = v / 10;
    }
    let p: usize = pos;
    while (n > 0 && p < out.len) {
        n -= 1;
        out[p] = tmp[n];
        p += 1;
    }
    return p;
}

pub struct RateLimiter {
    window_ms: i64,
    max_requests: u32,
    slots: [64]RateLimitSlot,

    static fn init(window_ms: i64, max_requests: u32): RateLimiter {
        const safe_window: i64 = if (window_ms > 0) window_ms else 1;
        let limiter = RateLimiter{
            window_ms: safe_window,
            max_requests: max_requests,
            slots: [erlEmptySlot(); 64],
        };
        return limiter;
    }

    fn hit(*_, key: string, now_ms: i64): RateLimitHit {
        const h: u64 = erlHashKey(key);
        const start: usize = usize(h % (ERL_CAPACITY as u64));

        let found_idx: usize = ERL_CAPACITY;
        let free_idx: usize = ERL_CAPACITY;
        let expired_idx: usize = ERL_CAPACITY;
        let oldest_idx: usize = start;
        let oldest_reset: i64 = _.slots[start].reset_at_ms;

        let step: usize = 0;
        while (step < ERL_CAPACITY) : (step += 1) {
            const idx: usize = (start + step) % ERL_CAPACITY;
            const slot = _.slots[idx];
            if (erlSlotMatches(slot, h, key)) {
                found_idx = idx;
                break;
            }
            if (!slot.occupied) {
                if (free_idx == ERL_CAPACITY) {
                    free_idx = idx;
                }
            } else {
                if (slot.reset_at_ms <= now_ms && expired_idx == ERL_CAPACITY) {
                    expired_idx = idx;
                }
                if (slot.reset_at_ms < oldest_reset) {
                    oldest_reset = slot.reset_at_ms;
                    oldest_idx = idx;
                }
            }
        }

        let target_idx: usize = found_idx;
        let current: u32 = 1;
        let reset_at: i64 = now_ms + _.window_ms;

        if (found_idx != ERL_CAPACITY) {
            const existing = _.slots[found_idx];
            if (now_ms >= existing.reset_at_ms) {
                _.slots[found_idx].count = 1;
                _.slots[found_idx].reset_at_ms = reset_at;
                current = 1;
            } else {
                current = existing.count + 1;
                reset_at = existing.reset_at_ms;
                _.slots[found_idx].count = current;
            }
        } else {
            if (free_idx != ERL_CAPACITY) {
                target_idx = free_idx;
            } else if (expired_idx != ERL_CAPACITY) {
                target_idx = expired_idx;
            } else {
                target_idx = oldest_idx;
            }
            _.slots[target_idx] = erlStoreSlot(h, key, 1, reset_at);
            current = 1;
        }

        const allowed: bool = current <= _.max_requests;
        const remaining: u32 = if (allowed) (_.max_requests - current) else 0;
        const retry_sec: u32 = erlCeilSeconds(reset_at, now_ms);

        return RateLimitHit{
            allowed: allowed,
            limit: _.max_requests,
            current: current,
            remaining: remaining,
            reset_at_ms: reset_at,
            retry_after_sec: retry_sec,
        };
    }

    fn resetKey(*_, key: string) {
        const h: u64 = erlHashKey(key);
        const start: usize = usize(h % (ERL_CAPACITY as u64));
        let step: usize = 0;
        while (step < ERL_CAPACITY) : (step += 1) {
            const idx: usize = (start + step) % ERL_CAPACITY;
            if (erlSlotMatches(_.slots[idx], h, key)) {
                _.slots[idx] = erlEmptySlot();
                return;
            }
        }
    }

    fn resetAll(*_) {
        let i: usize = 0;
        while (i < ERL_CAPACITY) : (i += 1) {
            _.slots[i] = erlEmptySlot();
        }
    }

    fn activeCount(now_ms: i64): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < ERL_CAPACITY) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].reset_at_ms > now_ms) {
                n += 1;
            }
        }
        return n;
    }
}

pub fn formatRateLimitHeader(hit: RateLimitHit, out: []u8): usize {
    let pos: usize = 0;
    pos = erlWriteStr(out, pos, "limit=");
    pos = erlWriteU32(out, pos, hit.limit);
    pos = erlWriteStr(out, pos, ", remaining=");
    pos = erlWriteU32(out, pos, hit.remaining);
    pos = erlWriteStr(out, pos, ", reset=");
    pos = erlWriteU32(out, pos, hit.retry_after_sec);
    return pos;
}
