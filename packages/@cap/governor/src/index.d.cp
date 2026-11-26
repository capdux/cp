// Type definitions for CP

export class RateDecision {
    allowed: bool,
    remaining: u64,
    retry_after_ms: u64,
    reset_after_ms: u64,
}

export fn ceilToU64(v: f64): u64;
export class GcraLimiter {
    emission_interval_ms: u64,
    burst_tolerance_ms: u64,
    tat_ms: u64,
    fn perPeriod(limit: u64, period_ms: u64, burst: u64): GcraLimiter;
    fn init(rate_per_sec: u64, burst: u64): GcraLimiter;
    fn reset(_: auto);
    fn checkNAt(_: auto, cost: u64, now_ms: u64): RateDecision;
    fn checkAt(_: auto, now_ms: u64): RateDecision;
}

export class TokenBucket {
    capacity: f64,
    refill_per_sec: f64,
    tokens: f64,
    last_ms: u64,
    fn init(capacity: f64, refill_per_sec: f64, now_ms: u64): TokenBucket;
    fn refill(_: auto, now_ms: u64);
    fn available(_: auto, now_ms: u64): f64;
    fn reset(_: auto, now_ms: u64);
    fn tryAcquire(_: auto, tokens: f64, now_ms: u64): RateDecision;
}

