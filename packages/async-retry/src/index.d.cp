// Type definitions for CP

export enum BackoffStrategy {
    Fixed = 0,
    Linear = 1,
    Exponential = 2,
    Fibonacci = 3
}

export enum JitterMode {
    None = 0,
    Full = 1,
    Equal = 2,
    Decorrelated = 3
}

export enum FailureKind {
    Transient = 0,
    Permanent = 1,
    RateLimited = 2
}

export enum StopReason {
    None = 0,
    Succeeded = 1,
    PermanentFailure = 2,
    MaxAttemptsReached = 3,
    ElapsedBudgetExceeded = 4,
    RetryBudgetExhausted = 5
}

export class RetryPolicy {
    strategy: BackoffStrategy,
    jitter: JitterMode,
    max_retries: u32,
    base_delay_ns: u64,
    max_delay_ns: u64,
    max_elapsed_ns: u64,
    factor_num: u64,
    factor_den: u64,
    fn exponentialDefault(): RetryPolicy;
    fn fixed(interval_ns: u64, max_retries: u32): RetryPolicy;
    fn decorrelated(base_ns: u64, max_ns: u64, max_retries: u32): RetryPolicy;
}

export class RetryDecision {
    should_retry: bool,
    delay_ns: u64,
    attempt: u32,
    stop_reason: StopReason,
}

export class SplitMix64 {
    state: u64,
    fn fromOs(): SplitMix64;
    fn fromSeed(seed: u64): SplitMix64;
    fn nextU64(_: auto): u64;
    fn uniformInclusive(_: auto, bound: u64): u64;
}

export fn saturatingMulU64(a: u64, b: u64): u64;
export fn clampDelay(delay: u64, max_delay: u64): u64;
export class RetryState {
    policy: RetryPolicy,
    rng: SplitMix64,
    attempts_done: u32,
    prev_delay_ns: u64,
    fib_prev: u64,
    fib_curr: u64,
    total_delay_ns: u64,
    started_at_ns: u64,
    fn init(policy: RetryPolicy, now_ns: u64): RetryState;
    fn initSeeded(policy: RetryPolicy, now_ns: u64, seed: u64): RetryState;
    fn reset(_: auto, now_ns: u64);
    fn computeRawDelay(_: auto, retry_index: u32): u64;
    fn applyJitter(_: auto, raw_delay: u64): u64;
    fn onFailure(_: auto, kind: FailureKind, retry_after_hint_ns: u64, now_ns: u64): RetryDecision;
    fn attempts(): u32;
    fn totalDelayNs(): u64;
}

export class RetryBudget {
    tokens: u64,
    max_tokens: u64,
    deposit_per_request: u64,
    withdraw_per_retry: u64,
    retries_allowed: u64,
    retries_rejected: u64,
    fn init(max_burst_retries: u64, ratio_percent: u64): RetryBudget;
    fn recordRequest(_: auto);
    fn tryAcquireRetry(_: auto): bool;
    fn availableRetries(): u64;
}

