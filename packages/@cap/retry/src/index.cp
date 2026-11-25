// @cap/retry — Zero-allocation Industrial Retry, Backoff & Jitter Engine
// Clean-room pure-CP implementation modeled after Rust `backoff`/`backon`, npm `p-retry`,
// and the AWS Architecture Blog "Exponential Backoff And Jitter" specification (MIT License).

const std = @import("std");

pub const NS_PER_MS: u64 = 1_000_000;
pub const NS_PER_SEC: u64 = 1_000_000_000;

/// Backoff curve strategy.
pub enum BackoffStrategy: u8 {
    Fixed = 0,
    Linear = 1,
    Exponential = 2,
    Fibonacci = 3,
}

/// Anti-thundering-herd jitter strategy (AWS Architecture Blog).
pub enum JitterMode: u8 {
    None = 0,
    /// Uniform random in `[0, computed_delay]`.
    Full = 1,
    /// `computed_delay / 2 + uniform(0, computed_delay / 2)`.
    Equal = 2,
    /// `min(max_delay, uniform(base_delay, prev_delay * 3))`.
    Decorrelated = 3,
}

/// Classification of an attempt failure to decide whether retrying makes sense.
pub enum FailureKind: u8 {
    /// Retryable transient error (network reset, 503, timeout).
    Transient = 0,
    /// Non-retryable permanent error (400, 401, validation failure) — aborts immediately.
    Permanent = 1,
    /// Server-directed backoff (HTTP 429 / 503 with explicit `Retry-After` hint).
    RateLimited = 2,
}

/// Reason why a retry loop decided to stop.
pub enum StopReason: u8 {
    None = 0,
    Succeeded = 1,
    PermanentFailure = 2,
    MaxAttemptsReached = 3,
    ElapsedBudgetExceeded = 4,
    RetryBudgetExhausted = 5,
}

/// Retry policy configuration (stack-allocated, zero heap usage).
pub struct RetryPolicy {
    strategy: BackoffStrategy = BackoffStrategy.Exponential,
    jitter: JitterMode = JitterMode.Full,
    /// Maximum number of retries after the initial attempt (`0` = no retries).
    max_retries: u32 = 3,
    /// Initial backoff delay in nanoseconds.
    base_delay_ns: u64 = 100 * NS_PER_MS,
    /// Maximum cap on any single backoff delay in nanoseconds.
    max_delay_ns: u64 = 30 * NS_PER_SEC,
    /// Optional total elapsed wall-time budget in nanoseconds (`0` = unlimited).
    max_elapsed_ns: u64 = 0,
    /// Exponential multiplier numerator/denominator (default `2/1` = 2.0x; e.g. `3/2` = 1.5x).
    factor_num: u64 = 2,
    factor_den: u64 = 1,

    /// Standard exponential backoff with AWS Full Jitter (100ms base, 30s cap, 5 retries).
    pub static fn exponentialDefault(): RetryPolicy {
        return RetryPolicy{
            strategy: BackoffStrategy.Exponential,
            jitter: JitterMode.Full,
            max_retries: 5,
            base_delay_ns: 100 * NS_PER_MS,
            max_delay_ns: 30 * NS_PER_SEC,
            max_elapsed_ns: 0,
            factor_num: 2,
            factor_den: 1,
        };
    }

    /// Fixed-interval retry policy.
    pub static fn fixed(interval_ns: u64, max_retries: u32): RetryPolicy {
        return RetryPolicy{
            strategy: BackoffStrategy.Fixed,
            jitter: JitterMode.None,
            max_retries: max_retries,
            base_delay_ns: interval_ns,
            max_delay_ns: interval_ns,
            max_elapsed_ns: 0,
            factor_num: 1,
            factor_den: 1,
        };
    }

    /// Decorrelated jitter policy (recommended by AWS for high-contention distributed queues).
    pub static fn decorrelated(base_ns: u64, max_ns: u64, max_retries: u32): RetryPolicy {
        return RetryPolicy{
            strategy: BackoffStrategy.Exponential,
            jitter: JitterMode.Decorrelated,
            max_retries: max_retries,
            base_delay_ns: base_ns,
            max_delay_ns: if (max_ns < base_ns) base_ns else max_ns,
            max_elapsed_ns: 0,
            factor_num: 2,
            factor_den: 1,
        };
    }
}

/// Decision returned after evaluating a failed attempt.
pub struct RetryDecision {
    should_retry: bool,
    delay_ns: u64,
    attempt: u32,
    stop_reason: StopReason,
}

const SPLITMIX_GAMMA: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
const SPLITMIX_MUL1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
const SPLITMIX_MUL2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);

/// Fast SplitMix64 PRNG for zero-syscall deterministic or OS-seeded jitter.
struct SplitMix64 {
    state: u64,

    static fn fromOs(): SplitMix64 {
        let seed_bytes: [8]u8 = undefined;
        * = std.os.linux.getrandom(&seed_bytes, 8, 0);
        let s: u64 = 0;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            s = (s << 8) | (seed_bytes[i] as u64);
        }
        if (s == 0) s = SPLITMIX_GAMMA;
        return SplitMix64{ state: s };
    }

    static fn fromSeed(seed: u64): SplitMix64 {
        return SplitMix64{ state: if (seed == 0) SPLITMIX_GAMMA else seed };
    }

    fn nextU64(*_): u64 {
        _.state = _.state +% SPLITMIX_GAMMA;
        let z = _.state;
        z = (z ^ (z >> 30)) *% SPLITMIX_MUL1;
        z = (z ^ (z >> 27)) *% SPLITMIX_MUL2;
        return z ^ (z >> 31);
    }

    /// Unbiased uniform random integer in `[0, bound]` inclusive.
    fn uniformInclusive(*_, bound: u64): u64 {
        if (bound == 0) return 0;
        if (bound == std.math.maxInt(u64)) return _.nextU64();
        const span: u64 = bound + 1;
        return _.nextU64() % span;
    }
}

fn saturatingMulU64(a: u64, b: u64): u64 {
    const prod: u128 = (a as u128) * (b as u128);
    if (prod >= (std.math.maxInt(u64) as u128)) return std.math.maxInt(u64);
    return prod as u64;
}

fn clampDelay(delay: u64, max_delay: u64): u64 {
    if (max_delay > 0 && delay > max_delay) return max_delay;
    return delay;
}

/// Stateful, zero-allocation retry scheduler supporting all backoff curves, AWS jitters,
/// elapsed-time deadlines, and server `Retry-After` hints.
pub struct RetryState {
    policy: RetryPolicy,
    rng: SplitMix64,
    attempts_done: u32 = 0,
    prev_delay_ns: u64 = 0,
    fib_prev: u64 = 0,
    fib_curr: u64 = 1,
    total_delay_ns: u64 = 0,
    started_at_ns: u64 = 0,

    /// Initializes a retry scheduler seeded from OS CSPRNG.
    pub static fn init(policy: RetryPolicy, now_ns: u64): RetryState {
        return RetryState{
            policy: policy,
            rng: SplitMix64.fromOs(),
            attempts_done: 0,
            prev_delay_ns: policy.base_delay_ns,
            fib_prev: 0,
            fib_curr: 1,
            total_delay_ns: 0,
            started_at_ns: now_ns,
        };
    }

    /// Initializes a deterministic retry scheduler (ideal for reproducible unit tests & simulations).
    pub static fn initSeeded(policy: RetryPolicy, now_ns: u64, seed: u64): RetryState {
        return RetryState{
            policy: policy,
            rng: SplitMix64.fromSeed(seed),
            attempts_done: 0,
            prev_delay_ns: policy.base_delay_ns,
            fib_prev: 0,
            fib_curr: 1,
            total_delay_ns: 0,
            started_at_ns: now_ns,
        };
    }

    pub fn reset(*_, now_ns: u64) {
        _.attempts_done = 0;
        _.prev_delay_ns = _.policy.base_delay_ns;
        _.fib_prev = 0;
        _.fib_curr = 1;
        _.total_delay_ns = 0;
        _.started_at_ns = now_ns;
    }

    fn computeRawDelay(*_, retry_index: u32): u64 {
        const base = _.policy.base_delay_ns;
        const cap = _.policy.max_delay_ns;
        switch (_.policy.strategy) {
            .Fixed => return clampDelay(base, cap),
            .Linear => {
                const mult: u64 = (retry_index as u64) + 1;
                return clampDelay(saturatingMulU64(base, mult), cap);
            },
            .Exponential => {
                const fnum = if (_.policy.factor_num == 0) 1 as u64 else _.policy.factor_num;
                const fden = if (_.policy.factor_den == 0) 1 as u64 else _.policy.factor_den;
                let d: u128 = base as u128;
                let step: u32 = 0;
                const cap128: u128 = if (cap > 0) (cap as u128) else (std.math.maxInt(u64) as u128);
                while (step < retry_index) : (step += 1) {
                    d = (d * (fnum as u128)) / (fden as u128);
                    if (d >= cap128) {
                        d = cap128;
                        break;
                    }
                }
                if (d >= (std.math.maxInt(u64) as u128)) return std.math.maxInt(u64);
                return clampDelay(d as u64, cap);
            },
            .Fibonacci => {
                const mult = _.fib_curr;
                const next_fib: u128 = (_.fib_prev as u128) + (_.fib_curr as u128);
                _.fib_prev = _.fib_curr;
                _.fib_curr = if (next_fib >= (std.math.maxInt(u64) as u128)) std.math.maxInt(u64) else (next_fib as u64);
                return clampDelay(saturatingMulU64(base, mult), cap);
            },
        }
    }

    fn applyJitter(*_, raw_delay: u64): u64 {
        const base = _.policy.base_delay_ns;
        const cap = _.policy.max_delay_ns;
        switch (_.policy.jitter) {
            .None => {
                _.prev_delay_ns = raw_delay;
                return raw_delay;
            },
            .Full => {
                const d = _.rng.uniformInclusive(raw_delay);
                _.prev_delay_ns = d;
                return d;
            },
            .Equal => {
                const half = raw_delay / 2;
                const d = half + _.rng.uniformInclusive(raw_delay - half);
                _.prev_delay_ns = d;
                return d;
            },
            .Decorrelated => {
                const hi = saturatingMulU64(_.prev_delay_ns, 3);
                const lo = if (base <= hi) base else hi;
                const span = hi - lo;
                const sample = lo + _.rng.uniformInclusive(span);
                const d = clampDelay(sample, cap);
                _.prev_delay_ns = d;
                return d;
            },
        }
    }

    /// Evaluates a failed attempt and returns whether to retry and how long to wait.
    /// `retry_after_hint_ns` is honored when `kind == FailureKind.RateLimited` and `> 0`.
    pub fn onFailure(
        *_,
        kind: FailureKind,
        retry_after_hint_ns: u64,
        now_ns: u64,
    ): RetryDecision {
        if (kind == FailureKind.Permanent) {
            return RetryDecision{
                should_retry: false,
                delay_ns: 0,
                attempt: _.attempts_done + 1,
                stop_reason: StopReason.PermanentFailure,
            };
        }

        if (_.attempts_done >= _.policy.max_retries) {
            return RetryDecision{
                should_retry: false,
                delay_ns: 0,
                attempt: _.attempts_done + 1,
                stop_reason: StopReason.MaxAttemptsReached,
            };
        }

        const retry_idx = _.attempts_done;
        let delay: u64 = 0;
        if (kind == FailureKind.RateLimited && retry_after_hint_ns > 0) {
            delay = clampDelay(retry_after_hint_ns, _.policy.max_delay_ns);
            _.prev_delay_ns = delay;
        } else {
            const raw = _.computeRawDelay(retry_idx);
            delay = _.applyJitter(raw);
        }

        // Enforce total elapsed deadline budget
        if (_.policy.max_elapsed_ns > 0) {
            const elapsed = if (now_ns >= _.started_at_ns) (now_ns - _.started_at_ns) else 0;
            if (elapsed >= _.policy.max_elapsed_ns) {
                return RetryDecision{
                    should_retry: false,
                    delay_ns: 0,
                    attempt: _.attempts_done + 1,
                    stop_reason: StopReason.ElapsedBudgetExceeded,
                };
            }
            const remaining = _.policy.max_elapsed_ns - elapsed;
            if (delay > remaining) {
                return RetryDecision{
                    should_retry: false,
                    delay_ns: 0,
                    attempt: _.attempts_done + 1,
                    stop_reason: StopReason.ElapsedBudgetExceeded,
                };
            }
        }

        _.attempts_done += 1;
        _.total_delay_ns = _.total_delay_ns +| delay;
        return RetryDecision{
            should_retry: true,
            delay_ns: delay,
            attempt: _.attempts_done,
            stop_reason: StopReason.None,
        };
    }

    pub fn attempts(): u32 {
        return _.attempts_done;
    }

    pub fn totalDelayNs(): u64 {
        return _.total_delay_ns;
    }
}

/// Token-bucket retry storm protector (modeled after Finagle / Envoy Retry Budgets).
/// Each normal request deposits `deposit_per_request` tokens; each retry withdraws `withdraw_per_retry`.
/// Prevents cascading retry storms when downstream services are degraded.
pub struct RetryBudget {
    tokens: u64,
    max_tokens: u64,
    deposit_per_request: u64,
    withdraw_per_retry: u64,
    retries_allowed: u64 = 0,
    retries_rejected: u64 = 0,

    /// Creates a retry budget allowing roughly `ratio_percent`% extra retry traffic (e.g. 20% -> deposit=20, withdraw=100).
    pub static fn init(max_burst_retries: u64, ratio_percent: u64): RetryBudget {
        const dep = if (ratio_percent == 0) 1 as u64 else ratio_percent;
        const wd: u64 = 100;
        const max_tok = saturatingMulU64(if (max_burst_retries == 0) 1 else max_burst_retries, wd);
        return RetryBudget{
            tokens: max_tok,
            max_tokens: max_tok,
            deposit_per_request: dep,
            withdraw_per_retry: wd,
            retries_allowed: 0,
            retries_rejected: 0,
        };
    }

    /// Record a normal (non-retry) request attempt, replenishing retry budget.
    pub fn recordRequest(*_) {
        const next = _.tokens +| _.deposit_per_request;
        _.tokens = if (next > _.max_tokens) _.max_tokens else next;
    }

    /// Attempt to consume budget for one retry. Returns `true` if permitted, `false` to shed retry load.
    pub fn tryAcquireRetry(*_): bool {
        if (_.tokens >= _.withdraw_per_retry) {
            _.tokens -= _.withdraw_per_retry;
            _.retries_allowed += 1;
            return true;
        }
        _.retries_rejected += 1;
        return false;
    }

    pub fn availableRetries(): u64 {
        return _.tokens / _.withdraw_per_retry;
    }
}
