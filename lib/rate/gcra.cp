// Source: cp/lib/rate/gcra.cp — Industrial-grade GCRA Rate Limiter (benchmarked against Rust `governor`)
// Pure CP, zero heap allocation, overflow-safe u128 arithmetic, IETF Draft RateLimit & Retry-After header formatters.

pub const NS_PER_SECOND: u64 = 1_000_000_000;
pub const NS_PER_MINUTE: u64 = 60_000_000_000;

const GCRA_U64_MAX: u64 = std.math.maxInt(u64);

fn gcraSatMulU64(a: u64, b: u64): u64 {
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a > GCRA_U64_MAX / b) {
        return GCRA_U64_MAX;
    }
    return a * b;
}

fn gcraClampU128ToU64(v: u128): u64 {
    const max_u128: u128 = GCRA_U64_MAX;
    if (v > max_u128) {
        return GCRA_U64_MAX;
    }
    return v as u64;
}

fn gcraCeilDivNsToSeconds(ns: u64): u64 {
    if (ns == 0) {
        return 0;
    }
    const ns_128: u128 = ns;
    const sec_128: u128 = NS_PER_SECOND;
    const res: u128 = (ns_128 + sec_128 - 1) / sec_128;
    return gcraClampU128ToU64(res);
}

fn gcraWriteU64Ascii(val: u64, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    if (val == 0) {
        out[0] = 48; // '0'
        return 1;
    }
    let tmp: [20]u8 = [0; 20];
    let count: usize = 0;
    let v: u64 = val;
    while (v > 0) {
        tmp[count] = (48 + (v % 10)) as u8;
        v /= 10;
        count += 1;
    }
    if (count > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < count) {
        out[i] = tmp[count - 1 - i];
        i += 1;
    }
    return count;
}

/// Rate-limiting quota specification (emission interval + burst capacity),
/// modeled after Rust `governor::Quota`.
pub struct Quota {
    replenish_1_per_ns: u64;
    max_burst: u64;

    /// Constructs a quota allowing `n` cells per second with default burst capacity `n`.
    /// Values of `0` are safely clamped to `1`.
    pub static fn perSecond(n: u64): Quota {
        const safe_n: u64 = if (n == 0) 1 else n;
        const raw_interval: u64 = NS_PER_SECOND / safe_n;
        const interval_ns: u64 = if (raw_interval == 0) 1 else raw_interval;
        return Quota{
            replenish_1_per_ns: interval_ns,
            max_burst: safe_n,
        };
    }

    /// Constructs a quota allowing `n` cells per minute with default burst capacity `n`.
    /// Values of `0` are safely clamped to `1`.
    pub static fn perMinute(n: u64): Quota {
        const safe_n: u64 = if (n == 0) 1 else n;
        const raw_interval: u64 = NS_PER_MINUTE / safe_n;
        const interval_ns: u64 = if (raw_interval == 0) 1 else raw_interval;
        return Quota{
            replenish_1_per_ns: interval_ns,
            max_burst: safe_n,
        };
    }

    /// Constructs a quota with an explicit per-cell emission interval in nanoseconds
    /// and an explicit burst capacity. Both parameters are clamped to `>= 1`.
    pub static fn perIntervalNs(interval_ns: u64, burst: u64): Quota {
        const safe_interval: u64 = if (interval_ns == 0) 1 else interval_ns;
        const safe_burst: u64 = if (burst == 0) 1 else burst;
        return Quota{
            replenish_1_per_ns: safe_interval,
            max_burst: safe_burst,
        };
    }

    /// Returns a new `Quota` with the same replenishment rate and burst capacity set to `b` (`>= 1`).
    pub fn withBurst(b: u64): Quota {
        const safe_burst: u64 = if (b == 0) 1 else b;
        return Quota{
            replenish_1_per_ns: _.replenish_1_per_ns,
            max_burst: safe_burst,
        };
    }

    /// Per-cell emission interval `T` in nanoseconds.
    pub fn emissionIntervalNs(): u64 {
        return _.replenish_1_per_ns;
    }

    /// Maximum burst capacity `B` (number of cells).
    pub fn burstCapacity(): u64 {
        return _.max_burst;
    }

    /// GCRA burst tolerance `tau = (B - 1) * T` in nanoseconds (saturating on overflow).
    pub fn burstToleranceNs(): u64 {
        const extra: u64 = if (_.max_burst > 1) _.max_burst - 1 else 0;
        return gcraSatMulU64(_.replenish_1_per_ns, extra);
    }
}

/// Deterministic decision returned by `GcraLimiter` checks and probes.
pub struct RateDecision {
    allowed: bool;
    remaining: u64;
    limit: u64;
    retry_after_ns: u64;
    reset_after_ns: u64;

    /// Wait duration in whole seconds (ceiling) for HTTP `Retry-After` header; `0` when allowed.
    pub fn retryAfterSeconds(): u64 {
        if (_.allowed) {
            return 0;
        }
        return gcraCeilDivNsToSeconds(_.retry_after_ns);
    }

    /// Duration in whole seconds (ceiling) until full burst capacity is restored (`RateLimit-Reset`).
    pub fn resetAfterSeconds(): u64 {
        return gcraCeilDivNsToSeconds(_.reset_after_ns);
    }
}

/// Generic Cell Rate Algorithm (GCRA) rate limiter modeled after Rust `governor`.
///
/// Uses a single Theoretical Arrival Time (`tat_ns`) state with exact `u128` internal
/// arithmetic for zero-drift pacing, burst tolerance, batch admission (`checkNAt`),
/// atomic non-advancing rejection, clock-rollback safety, and overflow immunity.
pub struct GcraLimiter {
    quota: Quota;
    tat_ns: u64;

    /// Initializes a fresh GCRA limiter from a `Quota`.
    pub static fn init(quota: Quota): GcraLimiter {
        return GcraLimiter{
            quota: quota,
            tat_ns: 0,
        };
    }

    /// Checks and consumes 1 cell at monotonic timestamp `now_ns`.
    pub fn checkAt(*_, now_ns: u64): RateDecision {
        return _.checkNAt(1, now_ns);
    }

    /// Checks and atomically consumes `n` cells at monotonic timestamp `now_ns`.
    /// When rejected, internal `tat_ns` is NOT advanced.
    pub fn checkNAt(*_, n: u64, now_ns: u64): RateDecision {
        const t: u128 = _.quota.replenish_1_per_ns;
        const b: u128 = _.quota.max_burst;
        const now: u128 = now_ns;
        const tat: u128 = _.tat_ns;
        const burst_window: u128 = b * t;
        const max_tat: u128 = now + burst_window;
        const base: u128 = if (tat > now) tat else now;

        // Zero-cost check: always allowed without advancing TAT.
        if (n == 0) {
            const avail_ns: u128 = if (max_tat > base) max_tat - base else 0;
            const rem: u64 = gcraClampU128ToU64(avail_ns / t);
            const reset_ns: u64 = gcraClampU128ToU64(base - now);
            return RateDecision{
                allowed: true,
                remaining: rem,
                limit: _.quota.max_burst,
                retry_after_ns: 0,
                reset_after_ns: reset_ns,
            };
        }

        // Batch larger than total burst capacity can never be satisfied in a single call.
        if (n > _.quota.max_burst) {
            const avail_ns: u128 = if (max_tat > base) max_tat - base else 0;
            const rem: u64 = gcraClampU128ToU64(avail_ns / t);
            const reset_ns: u64 = gcraClampU128ToU64(base - now);
            return RateDecision{
                allowed: false,
                remaining: rem,
                limit: _.quota.max_burst,
                retry_after_ns: GCRA_U64_MAX,
                reset_after_ns: reset_ns,
            };
        }

        const cost_ns: u128 = (n as u128) * t;
        const new_tat: u128 = base + cost_ns;

        if (new_tat > max_tat) {
            const retry_ns: u64 = gcraClampU128ToU64(new_tat - max_tat);
            const avail_ns: u128 = if (max_tat > base) max_tat - base else 0;
            const rem: u64 = gcraClampU128ToU64(avail_ns / t);
            const reset_ns: u64 = gcraClampU128ToU64(base - now);
            return RateDecision{
                allowed: false,
                remaining: rem,
                limit: _.quota.max_burst,
                retry_after_ns: retry_ns,
                reset_after_ns: reset_ns,
            };
        }

        _.tat_ns = gcraClampU128ToU64(new_tat);
        const avail_after: u128 = max_tat - new_tat;
        const rem_after: u64 = gcraClampU128ToU64(avail_after / t);
        const reset_after: u64 = gcraClampU128ToU64(new_tat - now);
        return RateDecision{
            allowed: true,
            remaining: rem_after,
            limit: _.quota.max_burst,
            retry_after_ns: 0,
            reset_after_ns: reset_after,
        };
    }

    /// Convenience boolean check for 1 cell at `now_ns`.
    pub fn allow(*_, now_ns: u64): bool {
        return _.checkAt(now_ns).allowed;
    }

    /// Read-only probe of current capacity at `now_ns` without mutating `tat_ns`.
    pub fn peek(now_ns: u64): RateDecision {
        const t: u128 = _.quota.replenish_1_per_ns;
        const b: u128 = _.quota.max_burst;
        const now: u128 = now_ns;
        const tat: u128 = _.tat_ns;
        const burst_window: u128 = b * t;
        const max_tat: u128 = now + burst_window;
        const base: u128 = if (tat > now) tat else now;
        const new_tat: u128 = base + t;
        const avail_ns: u128 = if (max_tat > base) max_tat - base else 0;
        const rem: u64 = gcraClampU128ToU64(avail_ns / t);
        const reset_ns: u64 = gcraClampU128ToU64(base - now);

        if (new_tat > max_tat) {
            return RateDecision{
                allowed: false,
                remaining: rem,
                limit: _.quota.max_burst,
                retry_after_ns: gcraClampU128ToU64(new_tat - max_tat),
                reset_after_ns: reset_ns,
            };
        }

        return RateDecision{
            allowed: true,
            remaining: rem,
            limit: _.quota.max_burst,
            retry_after_ns: 0,
            reset_after_ns: reset_ns,
        };
    }

    /// Resets the limiter to its initial full-burst state.
    pub fn reset(*_) {
        _.tat_ns = 0;
    }

    /// Returns the configured `Quota`.
    pub fn getQuota(): Quota {
        return _.quota;
    }
}

/// Byte lengths written into caller-provided buffers by `formatRateLimitHeaders`.
pub struct RateLimitHeaderLengths {
    limit_len: usize;
    remaining_len: usize;
    reset_len: usize;
}

/// Computes the HTTP `Retry-After` delta-seconds value (ceiling) from a `RateDecision`.
/// Returns `0` when the request was allowed.
pub fn formatRetryAfterSeconds(decision: RateDecision): u64 {
    if (decision.allowed) {
        return 0;
    }
    return gcraCeilDivNsToSeconds(decision.retry_after_ns);
}

/// Computes the IETF Draft `RateLimit-Reset` delta-seconds value (ceiling) from a `RateDecision`.
pub fn formatResetAfterSeconds(decision: RateDecision): u64 {
    return gcraCeilDivNsToSeconds(decision.reset_after_ns);
}

/// Zero-allocation formatter for IETF Draft `RateLimit-Limit`, `RateLimit-Remaining`,
/// and `RateLimit-Reset` header values into caller-supplied byte buffers.
/// Returns the number of ASCII bytes written into each buffer (`0` if a buffer is too small).
pub fn formatRateLimitHeaders(
    decision: RateDecision,
    out_limit: []u8,
    out_remaining: []u8,
    out_reset: []u8,
): RateLimitHeaderLengths {
    const limit_len = gcraWriteU64Ascii(decision.limit, out_limit);
    const remaining_len = gcraWriteU64Ascii(decision.remaining, out_remaining);
    const reset_len = gcraWriteU64Ascii(formatResetAfterSeconds(decision), out_reset);
    return RateLimitHeaderLengths{
        limit_len: limit_len,
        remaining_len: remaining_len,
        reset_len: reset_len,
    };
}

/// Zero-allocation formatter for the HTTP `Retry-After` header value (delta-seconds).
/// Writes ASCII digits into `out` and returns the written byte length (`0` if `out` is too small).
pub fn formatRetryAfterHeader(decision: RateDecision, out: []u8): usize {
    return gcraWriteU64Ascii(formatRetryAfterSeconds(decision), out);
}
