// Origin: Pure-CP Stdlib (Adaptive Network Retry Backoff, Retry-After Clamping & Anti-Storm Retry Budget) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation Google SRE / Envoy / AWS Architecture anti-storm retry controller:
//   - Deterministic SplitMix64 jitter modes (`none`, `full_jitter`, `equal_jitter`, `decorrelated_jitter`)
//   - Overflow-safe exponential backoff capped by `max_delay_ms` & clamped server `retry_after_ms`
//   - Token-bucket global Retry Budget (`retry_ratio_bps` per normal request + `min_retries_per_sec` floor)
//   - Error class retryability guard (`transient_network`, `timeout`, `rate_limited_429`, `server_5xx` vs `client_4xx`, `cancelled`)
//   - Per-request total deadline verification (`deadline_ms`) with overflow & clock-rollback protection

const std = @import("std");

// ============================================================================
// Constants
// ============================================================================

/// Basis-point denominator (`10_000 bps = 100%`).
pub const RETRY_BPS_DENOM: u32 = 10_000;

/// Default retry budget ratio (`2000 bps = 20%` of normal request volume).
pub const RETRY_DEFAULT_RATIO_BPS: u32 = 2_000;

/// Default exponential backoff ceiling in milliseconds (30 seconds).
pub const RETRY_DEFAULT_MAX_DELAY_MS: u64 = 30_000;

// ============================================================================
// Internal Numeric & PRNG Helpers
// ============================================================================

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn splitMix64Next(state: *u64): u64 {
    state.* = state.* +% 0x9e3779b97f4a7c15;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

/// Returns a uniform integer in `[0, bound_inclusive]`.
fn uniformInclusive(state: *u64, bound_inclusive: u64): u64 {
    if (bound_inclusive == 0) return 0;
    const r: u64 = splitMix64Next(state);
    if (bound_inclusive == 0xFFFFFFFFFFFFFFFF) return r;
    return r % (bound_inclusive + 1);
}

fn saturatingExpCap(base_ms: u64, max_ms: u64, shift_steps: u32): u64 {
    if (base_ms >= max_ms) return max_ms;
    let cur: u64 = base_ms;
    let step: u32 = 0;
    while (step < shift_steps) : (step += 1) {
        if (cur >= max_ms) return max_ms;
        if (cur > max_ms / 2) return max_ms;
        cur = cur * 2;
    }
    return if (cur > max_ms) max_ms else cur;
}

fn saturatingMul3(v: u64, cap: u64): u64 {
    if (v >= cap) return cap;
    if (v > cap / 3) return cap;
    return v * 3;
}

// ============================================================================
// 1. Enums & Configurations
// ============================================================================

/// Backoff jitter strategy (AWS Architecture / Envoy standard algorithms).
pub enum RetryJitterMode: u8 {
    /// Deterministic exponential backoff: `min(max_delay_ms, base_delay_ms * 2^(attempt-1))`.
    none = 0,
    /// Full Jitter: `uniform[0, exp_cap]`.
    full_jitter = 1,
    /// Equal Jitter: `exp_cap / 2 + uniform[0, exp_cap - exp_cap / 2]`.
    equal_jitter = 2,
    /// Decorrelated Jitter: `min(max_delay_ms, uniform[base_delay_ms, max(base_delay_ms, prev_delay * 3)])`.
    decorrelated_jitter = 3,
}

/// Classification of upstream/network failures for retryability gating.
pub enum RetryErrorClass: u8 {
    transient_network = 0,
    timeout = 1,
    rate_limited_429 = 2,
    server_5xx = 3,
    client_4xx = 4,
    cancelled = 5,
}

/// Outcome of `AdaptiveRetryController.evaluateRetry(...)`.
pub enum RetryDecisionKind: u8 {
    /// Retry approved; caller should sleep `delay_ms` before `next_attempt`.
    retry_allowed = 0,
    /// Failure class is non-retryable (`client_4xx` or `cancelled`).
    reject_non_retryable = 1,
    /// Request has already reached `max_attempts`.
    reject_max_attempts = 2,
    /// Request deadline has already passed or `now_ms + delay_ms > deadline_ms`.
    reject_deadline_exceeded = 3,
    /// Global anti-storm retry budget is exhausted; shedding retry to prevent cascade.
    reject_budget_exhausted = 4,
}

/// Per-request backoff & retry policy configuration.
pub struct RetryPolicyConfig {
    /// Maximum total attempts including the initial attempt (`>= 1`).
    max_attempts: u32 = 4;
    /// Base backoff delay in milliseconds (`> 0`).
    base_delay_ms: u64 = 100;
    /// Maximum capped backoff delay in milliseconds (`>= base_delay_ms`).
    max_delay_ms: u64 = RETRY_DEFAULT_MAX_DELAY_MS;
    /// Jitter distribution algorithm.
    jitter_mode: RetryJitterMode = RetryJitterMode.full_jitter;
    /// When `true`, honors positive server `retry_after_ms` clamped to `max_retry_after_ms`.
    respect_retry_after: bool = true;
    /// Upper clamp for server-provided `Retry-After` hints (`> 0`).
    max_retry_after_ms: u64 = 60_000;

    /// Returns production defaults (4 attempts, 100ms..30s full jitter, 60s Retry-After cap).
    static fn defaults(): RetryPolicyConfig {
        return RetryPolicyConfig{};
    }

    /// Validates all numeric invariants.
    fn isValid(): bool {
        if (_.max_attempts == 0) return false;
        if (_.base_delay_ms == 0 || _.max_delay_ms < _.base_delay_ms) return false;
        if (_.max_retry_after_ms == 0) return false;
        return true;
    }
}

/// Global token-bucket retry budget configuration (Google SRE / Envoy model).
pub struct RetryBudgetConfig {
    /// Budget tokens earned per normal request in basis points (`2000 = 20%`, `0..=10_000`).
    retry_ratio_bps: u32 = RETRY_DEFAULT_RATIO_BPS;
    /// Minimum retry token refill rate per second for low-QPS traffic.
    min_retries_per_sec: u32 = 10;
    /// Maximum token bucket capacity (`>= 1.0`).
    max_budget_tokens: f64 = 100.0;
    /// Starting token balance (`0.0 <= initial_tokens <= max_budget_tokens`).
    initial_tokens: f64 = 20.0;

    /// Returns production defaults (20% ratio, 10/s floor, cap 100, initial 20).
    static fn defaults(): RetryBudgetConfig {
        return RetryBudgetConfig{};
    }

    /// Validates all budget parameters.
    fn isValid(): bool {
        if (_.retry_ratio_bps > RETRY_BPS_DENOM) return false;
        if (!isFiniteF64(_.max_budget_tokens) || _.max_budget_tokens < 1.0) return false;
        if (!isFiniteF64(_.initial_tokens) || _.initial_tokens < 0.0 || _.initial_tokens > _.max_budget_tokens) {
            return false;
        }
        return true;
    }
}

/// Decision returned by `AdaptiveRetryController.evaluateRetry(...)`.
pub struct RetryDecision {
    kind: RetryDecisionKind = RetryDecisionKind.reject_non_retryable;
    delay_ms: u64 = 0;
    next_attempt: u32 = 0;
    used_retry_after: bool = false;
    clamped_retry_after: bool = false;
    remaining_budget_tokens: f64 = 0.0;
}

/// Telemetry counters for `AdaptiveRetryController`.
pub struct RetryControllerStats {
    total_requests_recorded: u64 = 0;
    total_evaluations: u64 = 0;
    retries_allowed: u64 = 0;
    rejected_non_retryable: u64 = 0;
    rejected_max_attempts: u64 = 0;
    rejected_deadline: u64 = 0;
    rejected_budget: u64 = 0;
    budget_tokens: f64 = 0.0;
}

// ============================================================================
// 2. Pure Classification & Backoff Functions
// ============================================================================

/// Returns `true` iff `err_class` is eligible for automated retry.
pub fn isRetryableError(err_class: RetryErrorClass): bool {
    return err_class == RetryErrorClass.transient_network ||
        err_class == RetryErrorClass.timeout ||
        err_class == RetryErrorClass.rate_limited_429 ||
        err_class == RetryErrorClass.server_5xx;
}

/// Computes the next backoff delay in milliseconds for 1-based failed `attempt_idx`
/// using overflow-safe exponential growth and deterministic SplitMix64 jitter.
pub fn computeBackoffDelayMs(
    cfg: RetryPolicyConfig,
    attempt_idx: u32,
    prev_delay_ms: u64,
    rng_state: *u64,
): u64 {
    if (!cfg.isValid()) return 0;
    const shift_steps: u32 = if (attempt_idx > 1) (attempt_idx - 1) else 0;
    const exp_cap: u64 = saturatingExpCap(cfg.base_delay_ms, cfg.max_delay_ms, shift_steps);

    if (cfg.jitter_mode == RetryJitterMode.none) {
        return exp_cap;
    }
    if (cfg.jitter_mode == RetryJitterMode.full_jitter) {
        return uniformInclusive(rng_state, exp_cap);
    }
    if (cfg.jitter_mode == RetryJitterMode.equal_jitter) {
        const half: u64 = exp_cap / 2;
        const rem: u64 = exp_cap - half;
        return half + uniformInclusive(rng_state, rem);
    }
    // decorrelated_jitter: min(max_delay_ms, uniform[base_delay_ms, max(base_delay_ms, prev_delay * 3)])
    const seed_prev: u64 = if (prev_delay_ms > 0) prev_delay_ms else cfg.base_delay_ms;
    const upper: u64 = saturatingMul3(seed_prev, cfg.max_delay_ms);
    if (upper <= cfg.base_delay_ms) return cfg.base_delay_ms;
    const span: u64 = upper - cfg.base_delay_ms;
    const sampled: u64 = cfg.base_delay_ms + uniformInclusive(rng_state, span);
    return if (sampled > cfg.max_delay_ms) cfg.max_delay_ms else sampled;
}

// ============================================================================
// 3. AdaptiveRetryController (Anti-Storm Budget + Backoff Engine)
// ============================================================================

/// Zero-heap-allocation adaptive retry backoff & global anti-storm retry budget controller.
pub struct AdaptiveRetryController {
    policy: RetryPolicyConfig = RetryPolicyConfig{},
    budget: RetryBudgetConfig = RetryBudgetConfig{},
    rng_state: u64 = 0x517cc1b727220a95,
    budget_tokens: f64 = 0.0,
    last_refill_ms: u64 = 0,
    total_requests_recorded: u64 = 0,
    total_evaluations: u64 = 0,
    retries_allowed: u64 = 0,
    rejected_non_retryable: u64 = 0,
    rejected_max_attempts: u64 = 0,
    rejected_deadline: u64 = 0,
    rejected_budget: u64 = 0,

    /// Initializes an `AdaptiveRetryController`. Returns `null` if either config is invalid.
    static fn init(
        policy: RetryPolicyConfig,
        budget: RetryBudgetConfig,
        rng_seed: u64,
        start_ms: u64,
    ): AdaptiveRetryController? {
        if (!policy.isValid() || !budget.isValid()) return null;
        const seed: u64 = if (rng_seed == 0) 0x517cc1b727220a95 else rng_seed;
        return AdaptiveRetryController{
            policy: policy,
            budget: budget,
            rng_state: seed,
            budget_tokens: budget.initial_tokens,
            last_refill_ms: start_ms,
        };
    }

    /// Refills time-based floor tokens (`min_retries_per_sec`) with monotonic clock-rollback protection.
    fn refillFloorTokens(*_, now_ms: u64) {
        if (now_ms <= _.last_refill_ms) return;
        const elapsed_ms: u64 = now_ms - _.last_refill_ms;
        _.last_refill_ms = now_ms;
        if (_.budget.min_retries_per_sec > 0) {
            const add: f64 = ((elapsed_ms as f64) / 1000.0) * (_.budget.min_retries_per_sec as f64);
            _.budget_tokens += add;
            if (_.budget_tokens > _.budget.max_budget_tokens) {
                _.budget_tokens = _.budget.max_budget_tokens;
            }
        }
    }

    /// Records one normal (non-retry) outbound request, earning `retry_ratio_bps / 10_000`
    /// budget tokens plus any elapsed `min_retries_per_sec` floor tokens.
    fn recordNormalRequest(*_, now_ms: u64) {
        _.refillFloorTokens(now_ms);
        _.total_requests_recorded += 1;
        const ratio_add: f64 = (_.budget.retry_ratio_bps as f64) / (RETRY_BPS_DENOM as f64);
        _.budget_tokens += ratio_add;
        if (_.budget_tokens > _.budget.max_budget_tokens) {
            _.budget_tokens = _.budget.max_budget_tokens;
        }
    }

    /// Evaluates whether failed attempt `attempt_idx` (1-based) may be retried at `now_ms`.
    ///
    /// - `retry_after_ms == 0` indicates no server `Retry-After` hint.
    /// - `deadline_ms == 0` disables per-request deadline enforcement.
    /// - Budget tokens are consumed ONLY when all prior checks pass and `retry_allowed` is returned.
    fn evaluateRetry(
        *_,
        err_class: RetryErrorClass,
        attempt_idx: u32,
        prev_delay_ms: u64,
        retry_after_ms: u64,
        now_ms: u64,
        deadline_ms: u64,
    ): RetryDecision {
        _.refillFloorTokens(now_ms);
        _.total_evaluations += 1;

        // 1) Error class retryability guard
        if (!isRetryableError(err_class)) {
            _.rejected_non_retryable += 1;
            return RetryDecision{
                kind: RetryDecisionKind.reject_non_retryable,
                delay_ms: 0,
                next_attempt: 0,
                used_retry_after: false,
                clamped_retry_after: false,
                remaining_budget_tokens: _.budget_tokens,
            };
        }

        // 2) Max attempts guard (attempt_idx is 1-based failed attempt number)
        if (attempt_idx == 0 || attempt_idx >= _.policy.max_attempts) {
            _.rejected_max_attempts += 1;
            return RetryDecision{
                kind: RetryDecisionKind.reject_max_attempts,
                delay_ms: 0,
                next_attempt: 0,
                used_retry_after: false,
                clamped_retry_after: false,
                remaining_budget_tokens: _.budget_tokens,
            };
        }

        // 3) Compute delay (server Retry-After clamped, or jittered exponential backoff)
        let delay_ms: u64 = 0;
        let used_ra: bool = false;
        let clamped_ra: bool = false;
        if (_.policy.respect_retry_after && retry_after_ms > 0) {
            used_ra = true;
            if (retry_after_ms > _.policy.max_retry_after_ms) {
                delay_ms = _.policy.max_retry_after_ms;
                clamped_ra = true;
            } else {
                delay_ms = retry_after_ms;
            }
        } else {
            delay_ms = computeBackoffDelayMs(_.policy, attempt_idx, prev_delay_ms, &_.rng_state);
        }

        // 4) Per-request deadline guard (overflow-safe subtraction)
        if (deadline_ms > 0) {
            if (now_ms >= deadline_ms || delay_ms > (deadline_ms - now_ms)) {
                _.rejected_deadline += 1;
                return RetryDecision{
                    kind: RetryDecisionKind.reject_deadline_exceeded,
                    delay_ms: delay_ms,
                    next_attempt: 0,
                    used_retry_after: used_ra,
                    clamped_retry_after: clamped_ra,
                    remaining_budget_tokens: _.budget_tokens,
                };
            }
        }

        // 5) Global anti-storm retry budget gate
        if (_.budget_tokens < 1.0) {
            _.rejected_budget += 1;
            return RetryDecision{
                kind: RetryDecisionKind.reject_budget_exhausted,
                delay_ms: delay_ms,
                next_attempt: 0,
                used_retry_after: used_ra,
                clamped_retry_after: clamped_ra,
                remaining_budget_tokens: _.budget_tokens,
            };
        }

        _.budget_tokens -= 1.0;
        _.retries_allowed += 1;
        return RetryDecision{
            kind: RetryDecisionKind.retry_allowed,
            delay_ms: delay_ms,
            next_attempt: attempt_idx + 1,
            used_retry_after: used_ra,
            clamped_retry_after: clamped_ra,
            remaining_budget_tokens: _.budget_tokens,
        };
    }

    /// Returns the current available retry budget token balance.
    fn availableBudgetTokens(): f64 {
        return _.budget_tokens;
    }

    /// Returns full controller telemetry counters.
    fn stats(): RetryControllerStats {
        return RetryControllerStats{
            total_requests_recorded: _.total_requests_recorded,
            total_evaluations: _.total_evaluations,
            retries_allowed: _.retries_allowed,
            rejected_non_retryable: _.rejected_non_retryable,
            rejected_max_attempts: _.rejected_max_attempts,
            rejected_deadline: _.rejected_deadline,
            rejected_budget: _.rejected_budget,
            budget_tokens: _.budget_tokens,
        };
    }
}
