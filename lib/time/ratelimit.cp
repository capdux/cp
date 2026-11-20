// Source: cp/lib/time/ratelimit.cp (TokenBucket, SlidingWindow & ExponentialBackoff)
// lib/time/ratelimit.cp
// Deterministic rate limiters (TokenBucket, SlidingWindowLimiter) and ExponentialBackoff with full jitter

pub struct TokenBucket {
    capacity: f64 = 1.0;
    tokens: f64 = 1.0;
    refill_per_sec: f64 = 1.0;
    last_refill_ms: i64 = 0;

    static fn init(capacity: f64, refill_per_sec: f64, now_ms: i64): TokenBucket {
        const cap = if (capacity < 0.0) 0.0 else capacity;
        const rate = if (refill_per_sec < 0.0) 0.0 else refill_per_sec;
        return TokenBucket {
            capacity: cap,
            tokens: cap,
            refill_per_sec: rate,
            last_refill_ms: now_ms,
        };
    }

    static fn initWithTokens(
        capacity: f64,
        refill_per_sec: f64,
        initial_tokens: f64,
        now_ms: i64,
    ): TokenBucket {
        const cap = if (capacity < 0.0) 0.0 else capacity;
        const rate = if (refill_per_sec < 0.0) 0.0 else refill_per_sec;
        let init_tok = if (initial_tokens < 0.0) 0.0 else initial_tokens;
        if (init_tok > cap) { init_tok = cap; }
        return TokenBucket {
            capacity: cap,
            tokens: init_tok,
            refill_per_sec: rate,
            last_refill_ms: now_ms,
        };
    }

    fn refill(*_, now_ms: i64) {
        if (now_ms <= _.last_refill_ms) { return; }
        const elapsed_ms = now_ms - _.last_refill_ms;
        _.last_refill_ms = now_ms;
        if (_.refill_per_sec <= 0.0) { return; }
        const added = ((elapsed_ms as f64) * _.refill_per_sec) / 1000.0;
        _.tokens += added;
        if (_.tokens > _.capacity) {
            _.tokens = _.capacity;
        }
    }

    fn available(*_, now_ms: i64): f64 {
        _.refill(now_ms);
        return _.tokens;
    }

    fn tryAcquire(*_, tokens: f64, now_ms: i64): bool {
        if (tokens < 0.0 || tokens > _.capacity) { return false; }
        _.refill(now_ms);
        if (tokens == 0.0) { return true; }
        if (_.tokens + 0.000000000001 >= tokens) {
            _.tokens -= tokens;
            if (_.tokens < 0.0) { _.tokens = 0.0; }
            return true;
        }
        return false;
    }

    fn tryAcquireOne(*_, now_ms: i64): bool {
        return _.tryAcquire(1.0, now_ms);
    }

    /// Returns milliseconds until `tokens` become available (0 if available now, -1 if impossible).
    fn retryAfterMs(*_, tokens: f64, now_ms: i64): i64 {
        if (tokens < 0.0 || tokens > _.capacity) { return -1; }
        _.refill(now_ms);
        if (tokens == 0.0 || _.tokens + 0.000000000001 >= tokens) { return 0; }
        if (_.refill_per_sec <= 0.0) { return -1; }
        const deficit = tokens - _.tokens;
        const exact_ms = (deficit * 1000.0) / _.refill_per_sec;
        const ceiled: i64 = @intFromFloat(std.math.ceil(exact_ms));
        return if (ceiled < 1) (1 as i64) else ceiled;
    }

    fn reset(*_, now_ms: i64) {
        _.tokens = _.capacity;
        _.last_refill_ms = now_ms;
    }
}

pub struct SlidingWindowLimiter {
    limit: u64 = 1;
    window_ms: i64 = 1000;
    window_start_ms: i64 = 0;
    prev_count: u64 = 0;
    curr_count: u64 = 0;

    static fn init(limit: u64, window_ms: i64, now_ms: i64): SlidingWindowLimiter {
        const w = if (window_ms < 1) 1 else window_ms;
        return SlidingWindowLimiter {
            limit: limit,
            window_ms: w,
            window_start_ms: now_ms,
            prev_count: 0,
            curr_count: 0,
        };
    }

    priv fn advance(*_, now_ms: i64) {
        if (now_ms <= _.window_start_ms) { return; }
        const elapsed = now_ms - _.window_start_ms;
        if (elapsed >= _.window_ms * 2) {
            _.prev_count = 0;
            _.curr_count = 0;
            _.window_start_ms = now_ms - @mod(elapsed, _.window_ms);
        } else if (elapsed >= _.window_ms) {
            _.prev_count = _.curr_count;
            _.curr_count = 0;
            _.window_start_ms += _.window_ms;
        }
    }

    priv fn weightedCount(now_ms: i64): f64 {
        const elapsed = if (now_ms > _.window_start_ms) (now_ms - _.window_start_ms) else 0;
        const weight = ((_.window_ms - elapsed) as f64) / (_.window_ms as f64);
        const clamped_w = if (weight < 0.0) 0.0 else weight;
        return ((_.prev_count as f64) * clamped_w) + (_.curr_count as f64);
    }

    fn currentCount(*_, now_ms: i64): f64 {
        _.advance(now_ms);
        return _.weightedCount(now_ms);
    }

    fn tryAcquireN(*_, count: u64, now_ms: i64): bool {
        _.advance(now_ms);
        if (count == 0) { return true; }
        const est = _.weightedCount(now_ms);
        if (est + (count as f64) <= (_.limit as f64) + 0.000000001) {
            _.curr_count += count;
            return true;
        }
        return false;
    }

    fn tryAcquire(*_, now_ms: i64): bool {
        return _.tryAcquireN(1, now_ms);
    }

    fn remaining(*_, now_ms: i64): u64 {
        const est = _.currentCount(now_ms);
        const lim_f = _.limit as f64;
        if (est >= lim_f) { return 0; }
        const rem_f: f64 = std.math.floor(lim_f - est + 0.000000001);
        return @intFromFloat(rem_f);
    }

    /// Estimates milliseconds until 1 more request can be admitted under the sliding window.
    fn retryAfterMs(*_, now_ms: i64): i64 {
        _.advance(now_ms);
        if (_.limit == 0) { return -1; }
        const est = _.weightedCount(now_ms);
        if (est + 1.0 <= (_.limit as f64) + 0.000000001) { return 0; }
        if (_.prev_count > 0) {
            // Need: prev_count * (window_ms - t_new) / window_ms + curr_count + 1 <= limit
            const allowed_from_prev = (_.limit as f64) - (_.curr_count as f64) - 1.0;
            if (allowed_from_prev >= 0.0) {
                const target_elapsed = ((_.window_ms as f64) * (1.0 - allowed_from_prev / (_.prev_count as f64)));
                const ceil_elapsed: i64 = @intFromFloat(std.math.ceil(target_elapsed));
                const target_ms = _.window_start_ms + ceil_elapsed;
                const diff = target_ms - now_ms;
                return if (diff < 1) (1 as i64) else diff;
            }
        }
        // Current window alone is saturated; must wait until next window slides far enough
        const next_win_ms = _.window_start_ms + _.window_ms;
        if (_.curr_count >= _.limit) {
            const extra = ((_.window_ms as f64) * (1.0 - ((_.limit - 1) as f64) / (_.curr_count as f64)));
            const ceil_extra: i64 = @intFromFloat(std.math.ceil(extra));
            const target_ms = next_win_ms + ceil_extra;
            const diff = target_ms - now_ms;
            return if (diff < 1) (1 as i64) else diff;
        }
        const diff = next_win_ms - now_ms;
        return if (diff < 1) (1 as i64) else diff;
    }

    fn reset(*_, now_ms: i64) {
        _.window_start_ms = now_ms;
        _.prev_count = 0;
        _.curr_count = 0;
    }
}

pub struct ExponentialBackoff {
    base_ms: u64 = 100;
    max_ms: u64 = 30000;
    multiplier: f64 = 2.0;
    attempt: u32 = 0;

    static fn init(base_ms: u64, max_ms: u64, multiplier: f64 = 2.0): ExponentialBackoff {
        const eff_max = if (max_ms < base_ms) base_ms else max_ms;
        const eff_mult = if (multiplier < 1.0) 1.0 else multiplier;
        return ExponentialBackoff {
            base_ms: base_ms,
            max_ms: eff_max,
            multiplier: eff_mult,
            attempt: 0,
        };
    }

    priv static fn splitmix64(x: u64): u64 {
        const c1: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
        const c2: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
        const c3: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);
        let z: u64 = x +% c1;
        z = (z ^ (z >> 30)) *% c2;
        z = (z ^ (z >> 27)) *% c3;
        return z ^ (z >> 31);
    }

    /// Computes uncapped-jitter exponential ceiling `min(max_ms, base_ms * multiplier^attempt)`.
    fn rawDelayMs(): u64 {
        if (_.base_ms == 0 || _.max_ms == 0) { return 0; }
        let d: f64 = _.base_ms as f64;
        const max_f: f64 = _.max_ms as f64;
        let i: u32 = 0;
        while (i < _.attempt) {
            d *= _.multiplier;
            if (d >= max_f) { return _.max_ms; }
            i += 1;
        }
        const rounded: u64 = @intFromFloat(d);
        return if (rounded > _.max_ms) _.max_ms else rounded;
    }

    /// Computes deterministic full-jitter delay in `[0, rawDelayMs()]` without advancing `attempt`.
    fn peekDelayMs(seed: u64): u64 {
        const cap = _.rawDelayMs();
        if (cap == 0) { return 0; }
        const c1: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
        const mixed = ExponentialBackoff.splitmix64(seed ^ ((_.attempt as u64) *% c1));
        return mixed % (cap + 1);
    }

    /// Computes deterministic full-jitter delay in `[0, rawDelayMs()]` and increments `attempt`.
    fn nextDelayMs(*_, seed: u64): u64 {
        const delay = _.peekDelayMs(seed);
        if (_.attempt < 1000000) {
            _.attempt += 1;
        }
        return delay;
    }

    /// Computes deterministic equal-jitter delay in `[cap/2, cap]` and increments `attempt`.
    fn nextEqualJitterMs(*_, seed: u64): u64 {
        const cap = _.rawDelayMs();
        const half = cap / 2;
        const span = cap - half;
        let jitter: u64 = 0;
        if (span > 0) {
            const mixed = ExponentialBackoff.splitmix64(seed ^ ((_.attempt as u64) *% 0x517cc1b727220a95));
            jitter = mixed % (span + 1);
        }
        if (_.attempt < 1000000) {
            _.attempt += 1;
        }
        return half + jitter;
    }

    fn reset(*_) {
        _.attempt = 0;
    }
}
