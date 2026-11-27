// Type definitions for CP

export class RetryPolicy {
    max_retries: u32,
    min_delay_ms: u64,
    max_delay_ms: u64,
    factor_num: u64,
    factor_den: u64,
    jitter_bps: u32,
    max_elapsed_ms: u64,
    fn exponential(): RetryPolicy;
    fn constant(max_retries: u32, delay_ms: u64): RetryPolicy;
}

export fn rbkClampDelay(delay: u64, max_delay: u64): u64;
export fn rbkMix64(x: u64): u64;
export fn computeBackoffMs(policy: RetryPolicy, attempt: u32): u64;
export fn applyJitterMs(base_delay_ms: u64, jitter_bps: u32, seed: u64): u64;
export class BackoffState {
    policy: RetryPolicy,
    attempt: u32,
    total_waited_ms: u64,
    rng_seed: u64,
    fn new(policy: RetryPolicy, seed: u64): BackoffState;
    fn attempts(): u32;
    fn totalWaitedMs(): u64;
    fn nextDelayMs(_: auto): u64?;
    fn reset(_: auto);
}

