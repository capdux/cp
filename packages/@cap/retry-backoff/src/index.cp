// @cap/retry-backoff — Zero-heap exponential backoff & retry policy state machine.
// Clean-room MIT port of npm `retry` / `p-retry` & Rust `backoff` (60M+/week).

pub struct RetryPolicy {
    pub max_retries: u32,
    pub min_delay_ms: u64,
    pub max_delay_ms: u64,
    pub factor_num: u64, // numerator (e.g. 200 = 2.0x)
    pub factor_den: u64, // denominator (e.g. 100)
    pub jitter_bps: u32, // 0..10000 (e.g. 2000 = +/-20% jitter)
    pub max_elapsed_ms: u64, // 0 = unlimited

    pub static fn exponential(): RetryPolicy {
        return RetryPolicy {
            max_retries: 5,
            min_delay_ms: 100,
            max_delay_ms: 30000,
            factor_num: 200,
            factor_den: 100,
            jitter_bps: 0,
            max_elapsed_ms: 0,
        };
    }

    pub static fn constant(max_retries: u32, delay_ms: u64): RetryPolicy {
        return RetryPolicy {
            max_retries: max_retries,
            min_delay_ms: delay_ms,
            max_delay_ms: delay_ms,
            factor_num: 100,
            factor_den: 100,
            jitter_bps: 0,
            max_elapsed_ms: 0,
        };
    }
}

fn rbkClampDelay(delay: u64, max_delay: u64): u64 {
    if (max_delay > 0 && delay > max_delay) { return max_delay; }
    return delay;
}

/// Deterministic xorshift64* mixer for reproducible jitter without global RNG state or overflow.
fn rbkMix64(x: u64): u64 {
    let s: u64 = if (x == 0) { 0x12345678 } else { x };
    let z = s ^ (s >> 12);
    let z1 = z ^ (z << 25);
    return z1 ^ (z1 >> 27);
}

/// Computes the base exponential delay for zero-based `attempt` (`0, 1, 2, ...`).
pub fn computeBackoffMs(policy: RetryPolicy, attempt: u32): u64 {
    let den = if (policy.factor_den == 0) { 1 } else { policy.factor_den };
    let num = if (policy.factor_num == 0) { den } else { policy.factor_num };
    let delay = policy.min_delay_ms;
    let i: u32 = 0;
    while (i < attempt) {
        if (policy.max_delay_ms > 0 && delay >= policy.max_delay_ms) {
            return policy.max_delay_ms;
        }
        // Overflow-safe scaling
        let safe_max: u64 = 0x7fffffffffffffff;
        if (delay > safe_max / num) {
            return if (policy.max_delay_ms > 0) { policy.max_delay_ms } else { delay };
        }
        delay = (delay * num) / den;
        i = i + 1;
    }
    return rbkClampDelay(delay, policy.max_delay_ms);
}

/// Applies deterministic symmetric jitter (`+/- jitter_bps / 10000`) to `base_delay_ms`.
pub fn applyJitterMs(base_delay_ms: u64, jitter_bps: u32, seed: u64): u64 {
    if (jitter_bps == 0 || base_delay_ms == 0) { return base_delay_ms; }
    let bps: u64 = if (jitter_bps > 10000) { 10000 } else { jitter_bps as u64 };
    let delta = (base_delay_ms * bps) / 10000;
    if (delta == 0) { return base_delay_ms; }
    let lo = if (base_delay_ms > delta) { base_delay_ms - delta } else { 0 };
    let span = (delta * 2) + 1;
    let r = rbkMix64(seed) % span;
    return lo + r;
}

/// Stateful retry backoff tracker (`BackoffState`).
pub struct BackoffState {
    pub policy: RetryPolicy,
    pub attempt: u32,
    pub total_waited_ms: u64,
    pub rng_seed: u64,

    pub static fn new(policy: RetryPolicy, seed: u64): BackoffState {
        return BackoffState {
            policy: policy,
            attempt: 0,
            total_waited_ms: 0,
            rng_seed: seed,
        };
    }

    pub fn attempts(): u32 { return _.attempt; }
    pub fn totalWaitedMs(): u64 { return _.total_waited_ms; }

    /// Returns the next backoff delay in ms and advances state, or `null` if retries/deadline exhausted.
    pub fn nextDelayMs(*_): u64? {
        if (_.attempt >= _.policy.max_retries) {
            return null;
        }
        let base = computeBackoffMs(_.policy, _.attempt);
        let delay = applyJitterMs(base, _.policy.jitter_bps, _.rng_seed + (_.attempt as u64));
        if (_.policy.max_elapsed_ms > 0 && _.total_waited_ms + delay > _.policy.max_elapsed_ms) {
            return null;
        }
        _.attempt = _.attempt + 1;
        _.total_waited_ms = _.total_waited_ms + delay;
        return delay;
    }

    pub fn reset(*_) {
        _.attempt = 0;
        _.total_waited_ms = 0;
    }
}
