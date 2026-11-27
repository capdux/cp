// Type definitions for CP

export enum JitterKind {
    None,
    Full,
    Equal,
    Decorrelated
}

export class BackoffPolicy {
    initial_ms: u64,
    max_ms: u64,
    multiplier_num: u64,
    multiplier_den: u64,
    max_retries: u32,
    jitter: JitterKind,
    fn defaultPolicy(): BackoffPolicy;
}

export fn makeU64(hi: u32, lo: u32): u64;
export fn addWrap64(a: u64, b: u64): u64;
export fn mulWrap64(a: u64, b: u64): u64;
export fn nextRandU64(state: &mut u64): u64;
export fn randRangeInclusive(state: &mut u64, lo: u64, hi: u64): u64;
export fn expDelayMs(attempt: u32, initial_ms: u64, max_ms: u64): u64;
export fn linearDelayMs(attempt: u32, step_ms: u64, max_ms: u64): u64;
export fn computePolicyRawCap(policy: BackoffPolicy, attempt: u32): u64;
export class BackoffState {
    policy: BackoffPolicy,
    attempt: u32,
    prev_delay_ms: u64,
    rng_state: u64,
    fn init(policy: BackoffPolicy, seed: u64): BackoffState;
    fn nextDelayMs(_: auto): u64?;
    fn reset(_: auto);
    fn attempts(): u32;
}

