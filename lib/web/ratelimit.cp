// Source: cp/lib/ratelimit/ratelimit.cp (Token Bucket, Sliding Window Counter, GCRA & Keyed Rate Limiter)
// Pure CP deterministic rate limiters: TokenBucket, SlidingWindowCounter, GcraLimiter, and KeyedRateLimiter.
// All time-dependent operations accept an explicit monotonic millisecond timestamp `now_ms: i64`.

const std = @import("std");

/// Error set for rate limiter configuration, quota acquisition, and memory allocation.
pub err RateLimitError {
    InvalidRate,
    InvalidBurst,
    InvalidWindow,
    InvalidCost,
    OutOfMemory,
}

/// Decision returned by rate limiter probes and acquisitions.
pub struct RateDecision {
    allowed: bool,
    remaining: u64,
    retry_after_ms: i64,
    reset_after_ms: i64,
}

fn ceilDivMs(numer: f64, denom: f64): i64 {
    if (numer <= 0.0 || denom <= 0.0) {
        return 0;
    }
    const raw = std.math.ceil(numer / denom);
    const val: i64 = @intFromFloat(raw);
    return if (val < 1) (1 as i64) else val;
}

/// Token Bucket rate limiter with smooth refill, burst capacity, peek, and clock-rollback clamping.
pub struct TokenBucket {
    capacity: u64,
    refill_per_sec: f64,
    tokens: f64,
    last_refill_ms: i64,

    /// Initializes a full `TokenBucket` with `capacity` burst and `refill_per_sec` replenishment rate.
    pub static fn init(capacity: u64, refill_per_sec: f64, now_ms: i64): TokenBucket ! RateLimitError {
        if (capacity == 0) {
            return RateLimitError.InvalidBurst;
        }
        if (std.math.isNan(refill_per_sec) || std.math.isInf(refill_per_sec) || refill_per_sec <= 0.0) {
            return RateLimitError.InvalidRate;
        }
        return TokenBucket{
            capacity: capacity,
            refill_per_sec: refill_per_sec,
            tokens: capacity as f64,
            last_refill_ms: now_ms,
        };
    }

    /// Initializes a `TokenBucket` with an explicit starting token count (clamped to `capacity`).
    pub fn initWithTokens(
        capacity: u64,
        refill_per_sec: f64,
        initial_tokens: u64,
        now_ms: i64,
    ): TokenBucket ! RateLimitError {
        if (capacity == 0) {
            return RateLimitError.InvalidBurst;
        }
        if (std.math.isNan(refill_per_sec) || std.math.isInf(refill_per_sec) || refill_per_sec <= 0.0) {
            return RateLimitError.InvalidRate;
        }
        const start_tok = if (initial_tokens > capacity) capacity else initial_tokens;
        return TokenBucket{
            capacity: capacity,
            refill_per_sec: refill_per_sec,
            tokens: start_tok as f64,
            last_refill_ms: now_ms,
        };
    }

    priv fn refill(*_, now_ms: i64) {
        // Clamp against monotonic clock rollback: never deduct tokens or move last_refill_ms backward.
        if (now_ms <= _.last_refill_ms) {
            return;
        }
        const elapsed_ms = now_ms - _.last_refill_ms;
        _.last_refill_ms = now_ms;
        const added = ((elapsed_ms as f64) * _.refill_per_sec) / 1000.0;
        _.tokens += added;
        const cap_f = _.capacity as f64;
        if (_.tokens > cap_f) {
            _.tokens = cap_f;
        }
    }

    priv fn calcResetAfterMs(*_): i64 {
        const cap_f = _.capacity as f64;
        if (_.tokens + 0.000000001 >= cap_f) {
            return 0;
        }
        const missing = cap_f - _.tokens;
        return ceilDivMs(missing * 1000.0, _.refill_per_sec);
    }

    /// Attempts to consume `cost` tokens at `now_ms` and returns a `RateDecision`.
    pub fn tryAcquire(*_, cost: u64, now_ms: i64): RateDecision ! RateLimitError {
        if (cost == 0 || cost > _.capacity) {
            return RateLimitError.InvalidCost;
        }
        _.refill(now_ms);
        const cost_f = cost as f64;
        if (_.tokens + 0.000000001 >= cost_f) {
            _.tokens -= cost_f;
            if (_.tokens < 0.0) {
                _.tokens = 0.0;
            }
            const rem: u64 = @intFromFloat(@floor(_.tokens + 0.000000001));
            return RateDecision{
                allowed: true,
                remaining: rem,
                retry_after_ms: 0,
                reset_after_ms: _.calcResetAfterMs(),
            };
        }

        const deficit = cost_f - _.tokens;
        const retry_ms = ceilDivMs(deficit * 1000.0, _.refill_per_sec);
        const rem: u64 = @intFromFloat(@floor(_.tokens + 0.000000001));
        return RateDecision{
            allowed: false,
            remaining: rem,
            retry_after_ms: retry_ms,
            reset_after_ms: _.calcResetAfterMs(),
        };
    }

    /// Non-consuming quota probe at `now_ms` (refills elapsed tokens without deducting).
    pub fn peek(*_, now_ms: i64): RateDecision {
        _.refill(now_ms);
        const rem: u64 = @intFromFloat(@floor(_.tokens + 0.000000001));
        const retry_ms: i64 = if (rem >= 1) 0 else ceilDivMs((1.0 - _.tokens) * 1000.0, _.refill_per_sec);
        return RateDecision{
            allowed: rem >= 1,
            remaining: rem,
            retry_after_ms: retry_ms,
            reset_after_ms: _.calcResetAfterMs(),
        };
    }

    /// Resets the bucket to full capacity at `now_ms`.
    pub fn reset(*_, now_ms: i64) {
        _.tokens = _.capacity as f64;
        _.last_refill_ms = now_ms;
    }
}

/// Dual-window weighted Sliding Window Counter eliminating fixed-window boundary burst spikes.
pub struct SlidingWindowCounter {
    limit: u64,
    window_ms: i64,
    window_start_ms: i64,
    prev_count: u64,
    curr_count: u64,

    /// Initializes a `SlidingWindowCounter` with `limit` per `window_ms` starting at `now_ms`.
    pub fn init(limit: u64, window_ms: i64, now_ms: i64): SlidingWindowCounter ! RateLimitError {
        if (limit == 0) {
            return RateLimitError.InvalidBurst;
        }
        if (window_ms <= 0) {
            return RateLimitError.InvalidWindow;
        }
        return SlidingWindowCounter{
            limit: limit,
            window_ms: window_ms,
            window_start_ms: now_ms,
            prev_count: 0,
            curr_count: 0,
        };
    }

    priv fn advance(*_, now_ms: i64) {
        if (now_ms <= _.window_start_ms) {
            return;
        }
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

    priv fn weightedCount(*_, now_ms: i64): f64 {
        const elapsed = if (now_ms > _.window_start_ms) (now_ms - _.window_start_ms) else 0;
        let weight = 1.0 - ((elapsed as f64) / (_.window_ms as f64));
        if (weight < 0.0) {
            weight = 0.0;
        } else if (weight > 1.0) {
            weight = 1.0;
        }
        return ((_.prev_count as f64) * weight) + (_.curr_count as f64);
    }

    /// Returns the current smoothed weighted request count at `now_ms`.
    pub fn currentCount(*_, now_ms: i64): f64 {
        _.advance(now_ms);
        return _.weightedCount(now_ms);
    }

    /// Attempts to acquire `cost` units at `now_ms` using dual-window linear interpolation.
    pub fn tryAcquire(*_, cost: u64, now_ms: i64): RateDecision ! RateLimitError {
        if (cost == 0 || cost > _.limit) {
            return RateLimitError.InvalidCost;
        }
        _.advance(now_ms);
        const est = _.weightedCount(now_ms);
        const limit_f = _.limit as f64;
        const cost_f = cost as f64;
        const elapsed = if (now_ms > _.window_start_ms) (now_ms - _.window_start_ms) else 0;
        const rem_win_ms: i64 = if (_.window_ms > elapsed) (_.window_ms - elapsed) else 0;

        if (est + cost_f <= limit_f + 0.000000001) {
            _.curr_count += cost;
            const new_est = est + cost_f;
            const rem_f = if (limit_f > new_est) (limit_f - new_est) else 0.0;
            const rem: u64 = @intFromFloat(@floor(rem_f + 0.000000001));
            return RateDecision{
                allowed: true,
                remaining: rem,
                retry_after_ms: 0,
                reset_after_ms: rem_win_ms,
            };
        }

        const rem_f = if (limit_f > est) (limit_f - est) else 0.0;
        const rem: u64 = @intFromFloat(@floor(rem_f + 0.000000001));
        const excess = est + cost_f - limit_f;
        let retry_ms: i64 = if (rem_win_ms > 0) rem_win_ms else 1;
        if (_.curr_count + cost <= _.limit && _.prev_count > 0) {
            const needed_ms = ceilDivMs(excess * (_.window_ms as f64), _.prev_count as f64);
            if (needed_ms < retry_ms) {
                retry_ms = needed_ms;
            }
        }
        if (retry_ms < 1) {
            retry_ms = 1;
        }
        return RateDecision{
            allowed: false,
            remaining: rem,
            retry_after_ms: retry_ms,
            reset_after_ms: rem_win_ms,
        };
    }

    /// Resets both windows at `now_ms`.
    pub fn reset(*_, now_ms: i64) {
        _.prev_count = 0;
        _.curr_count = 0;
        _.window_start_ms = now_ms;
    }
}

/// Generic Cell Rate Algorithm (GCRA) limiter backed by a single O(1) `tat_ms` state.
pub struct GcraLimiter {
    emission_interval_ms: i64,
    burst_tolerance: u64,
    tat_ms: i64,
    initialized: bool,

    /// Initializes a `GcraLimiter` with emission interval `T` (`emission_interval_ms`) and `burst_tolerance`.
    pub fn init(emission_interval_ms: i64, burst_tolerance: u64): GcraLimiter ! RateLimitError {
        if (emission_interval_ms <= 0) {
            return RateLimitError.InvalidRate;
        }
        return GcraLimiter{
            emission_interval_ms: emission_interval_ms,
            burst_tolerance: burst_tolerance,
            tat_ms: 0,
            initialized: false,
        };
    }

    /// Attempts to acquire `cost` cells at `now_ms` using a single `tat_ms` state variable.
    pub fn tryAcquire(*_, cost: u64, now_ms: i64): RateDecision ! RateLimitError {
        const max_burst = _.burst_tolerance + 1;
        if (cost == 0 || cost > max_burst) {
            return RateLimitError.InvalidCost;
        }
        const t = _.emission_interval_ms;
        const tau_total = (max_burst as i64) * t;
        const base_tat = if (_.initialized && _.tat_ms > now_ms) _.tat_ms else now_ms;
        const increment = (cost as i64) * t;
        const new_tat = base_tat + increment;
        const allow_at = new_tat - tau_total;

        if (now_ms >= allow_at) {
            _.tat_ms = new_tat;
            _.initialized = true;
            const diff = (now_ms + tau_total) - new_tat;
            const rem: u64 = if (diff <= 0) 0 else @as(u64, @intCast(@divFloor(diff, t)));
            const reset_ms: i64 = if (new_tat > now_ms) (new_tat - now_ms) else 0;
            return RateDecision{
                allowed: true,
                remaining: rem,
                retry_after_ms: 0,
                reset_after_ms: reset_ms,
            };
        }

        const retry_ms = allow_at - now_ms;
        const cur_tat = if (_.initialized && _.tat_ms > now_ms) _.tat_ms else now_ms;
        const diff = (now_ms + tau_total) - cur_tat;
        const rem: u64 = if (diff <= 0) 0 else @as(u64, @intCast(@divFloor(diff, t)));
        const reset_ms: i64 = if (cur_tat > now_ms) (cur_tat - now_ms) else 0;
        return RateDecision{
            allowed: false,
            remaining: rem,
            retry_after_ms: if (retry_ms < 1) (1 as i64) else retry_ms,
            reset_after_ms: reset_ms,
        };
    }

    /// Returns the internal Theoretical Arrival Time (`tat_ms`) for external/Redis state mapping.
    pub fn tatMs(*_): i64 {
        return _.tat_ms;
    }

    /// Resets the GCRA limiter back to its initial unallocated state.
    pub fn reset(*_) {
        _.tat_ms = 0;
        _.initialized = false;
    }
}

struct KeyBucketEntry {
    bucket: TokenBucket,
    last_seen_ms: i64,
}

/// Per-key / per-tenant isolated rate limiter backed by `TokenBucket` entries and idle TTL eviction.
pub struct KeyedRateLimiter {
    alloc: std.mem.Allocator,
    capacity: u64,
    refill_per_sec: f64,
    entries: std.StringHashMapUnmanaged(KeyBucketEntry) = .empty,

    /// Initializes a `KeyedRateLimiter` with per-key `capacity` and `refill_per_sec`.
    pub fn init(alloc: std.mem.Allocator, capacity: u64, refill_per_sec: f64): KeyedRateLimiter ! RateLimitError {
        if (capacity == 0) {
            return RateLimitError.InvalidBurst;
        }
        if (std.math.isNan(refill_per_sec) || std.math.isInf(refill_per_sec) || refill_per_sec <= 0.0) {
            return RateLimitError.InvalidRate;
        }
        return KeyedRateLimiter{
            alloc: alloc,
            capacity: capacity,
            refill_per_sec: refill_per_sec,
            entries: .empty,
        };
    }

    /// Releases all owned key strings and map storage.
    pub fn deinit(*_) {
        let it = _.entries.iterator();
        while (kv := it.next()) {
            _.alloc.free(kv.key_ptr.*);
        }
        _.entries.deinit(_.alloc);
    }

    /// Attempts to acquire `cost` tokens for `key` at `now_ms`, creating the key bucket on first sight.
    pub fn tryAcquireFor(*_, key: string, cost: u64, now_ms: i64): RateDecision ! RateLimitError {
        if (cost == 0 || cost > _.capacity) {
            return RateLimitError.InvalidCost;
        }
        if (entry_ptr := _.entries.getPtr(key)) {
            if (now_ms > entry_ptr.last_seen_ms) {
                entry_ptr.last_seen_ms = now_ms;
            }
            return entry_ptr.bucket.tryAcquire(cost, now_ms);
        }

        let bucket = TokenBucket.init(_.capacity, _.refill_per_sec, now_ms)?;
        const decision = bucket.tryAcquire(cost, now_ms)?;
        const key_copy = _.alloc.dupe(u8, key) catch return RateLimitError.OutOfMemory;
        errdefer _.alloc.free(key_copy);
        _.entries.put(_.alloc, key_copy, KeyBucketEntry{
            bucket: bucket,
            last_seen_ms: now_ms,
        }) catch return RateLimitError.OutOfMemory;
        return decision;
    }

    /// Evicts all keys whose `now_ms - last_seen_ms >= idle_ttl_ms` and frees their owned keys.
    pub fn evictIdle(*_, now_ms: i64, idle_ttl_ms: i64): usize {
        if (idle_ttl_ms < 0) {
            return 0;
        }
        let to_remove: std.ArrayListUnmanaged(string) = .empty;
        defer to_remove.deinit(_.alloc);

        let it = _.entries.iterator();
        while (kv := it.next()) {
            const idle_ms = now_ms - kv.value_ptr.last_seen_ms;
            if (idle_ms >= idle_ttl_ms) {
                to_remove.append(_.alloc, kv.key_ptr.*) catch break;
            }
        }

        let removed: usize = 0;
        for (k of to_remove.items) {
            if (old_kv := _.entries.fetchRemove(k)) {
                _.alloc.free(old_kv.key);
                removed += 1;
            }
        }
        return removed;
    }

    /// Returns the number of currently tracked keys.
    pub fn keyCount(*_): usize {
        return _.entries.count();
    }
}
