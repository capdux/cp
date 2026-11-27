// Source: cp/lib/circuit/policy.cp (Exponential Backoff Jitter & Bulkhead Limiter)
// Pure CP retry backoff with deterministic SplitMix64 jitter strategies
// and non-blocking concurrency bulkhead limiter.

const std = @import("std");
import { CircuitError } from "./breaker.cp";

/// Jitter distribution strategy for `BackoffPolicy`.
pub enum JitterMode: u8 {
    None = 0,
    Full = 1,
    Equal = 2,
    Decorrelated = 3,
}

fn splitMix64(x_in: u64): u64 {
    let z: u64 = x_in +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

/// Deterministic exponential backoff policy with configurable jitter.
pub struct BackoffPolicy {
    base_delay_ms: i64 = 100,
    max_delay_ms: i64 = 30000,
    multiplier: f64 = 2.0,
    max_attempts: u32 = 5,
    jitter: JitterMode = .Full,

    /// Validates and constructs a `BackoffPolicy`.
    pub static fn init(
        base_delay_ms: i64,
        max_delay_ms: i64,
        multiplier: f64,
        max_attempts: u32,
        jitter: JitterMode,
    ): BackoffPolicy ! CircuitError {
        if (base_delay_ms <= 0 ||
            max_delay_ms < base_delay_ms ||
            std.math.isNan(multiplier) ||
            multiplier < 1.0)
        {
            return CircuitError.InvalidConfig;
        }
        return BackoffPolicy{
            base_delay_ms: base_delay_ms,
            max_delay_ms: max_delay_ms,
            multiplier: multiplier,
            max_attempts: max_attempts,
            jitter: jitter,
        };
    }

    /// Returns true if `attempt` (0-based) is strictly below `max_attempts`.
    pub fn canRetry(*_, attempt: u32): bool {
        return attempt < _.max_attempts;
    }

    /// Computes the next retry delay in milliseconds using deterministic SplitMix64 PRNG.
    pub fn nextDelayMs(*_, attempt: u32, prev_delay_ms: i64, seed: u64): i64 {
        const exp_f = (_.base_delay_ms as f64) * std.math.pow(f64, _.multiplier, attempt as f64);
        const max_f = _.max_delay_ms as f64;
        const capped: i64 = if (std.math.isNan(exp_f) || exp_f >= max_f)
            _.max_delay_ms
        else
            exp_f as i64;

        const r = splitMix64(seed ^ ((attempt as u64) *% 0x9e3779b97f4a7c15));

        if (_.jitter == .None) {
            return capped;
        }
        if (_.jitter == .Full) {
            if (capped <= 0) return 0;
            const span = (capped as u64) + 1;
            return (r % span) as i64;
        }
        if (_.jitter == .Equal) {
            if (capped <= 0) return 0;
            const half = @divTrunc(capped, 2);
            const rem = capped - half;
            const span = (rem as u64) + 1;
            return half + ((r % span) as i64);
        }
        // Decorrelated jitter: random between base_delay_ms and min(max_delay_ms, prev_delay_ms * 3)
        const lo = _.base_delay_ms;
        const prev_clamped = if (prev_delay_ms > lo) prev_delay_ms else lo;
        const limit_div3 = @divTrunc(_.max_delay_ms, 3);
        let hi = if (prev_clamped > limit_div3) _.max_delay_ms else prev_clamped * 3;
        if (hi < lo) hi = lo;
        const span = ((hi - lo) as u64) + 1;
        return lo + ((r % span) as i64);
    }
}

/// Non-blocking concurrency bulkhead limiter.
pub struct Bulkhead {
    max_concurrent: usize,
    max_queue: usize,
    active_count: usize,
    rejected_count: u64,

    /// Validates and initializes a `Bulkhead` limiter.
    pub static fn init(max_concurrent: usize, max_queue: usize): Bulkhead ! CircuitError {
        if (max_concurrent == 0) {
            return CircuitError.InvalidConfig;
        }
        return Bulkhead{
            max_concurrent: max_concurrent,
            max_queue: max_queue,
            active_count: 0,
            rejected_count: 0,
        };
    }

    /// Attempts to acquire an execution permit; increments `rejected_count` when full.
    pub fn tryAcquire(*_): bool {
        if (_.active_count < _.max_concurrent) {
            _.active_count += 1;
            return true;
        }
        _.rejected_count += 1;
        return false;
    }

    /// Releases a previously acquired execution permit.
    pub fn releasePermit(*_) {
        if (_.active_count > 0) {
            _.active_count -= 1;
        }
    }

    /// Returns the number of currently active permits.
    pub fn activeCount(*_): usize {
        return _.active_count;
    }

    /// Returns total rejected acquisition attempts.
    pub fn rejectedCount(*_): u64 {
        return _.rejected_count;
    }

    /// Returns configured maximum concurrent permits.
    pub fn maxConcurrent(*_): usize {
        return _.max_concurrent;
    }

    /// Returns configured maximum queue capacity.
    pub fn maxQueue(*_): usize {
        return _.max_queue;
    }
}
