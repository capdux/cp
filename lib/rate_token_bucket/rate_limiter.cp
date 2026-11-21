// Source: cp/lib/rate_token_bucket/rate_limiter.cp (Pure CP Token Bucket & GCRA Rate Limiters)
const std = @import("std");

pub const RateLimitError = error{
    InvalidRate,
    InvalidBurst,
    InvalidCost,
};

/// Deterministic Token Bucket with nanosecond refill precision
pub const TokenBucket = struct {
    capacity: f64,
    tokens: f64,
    refill_per_ns: f64,
    last_ns: u64,

    pub fn init(rate_per_sec: f64, burst: f64, now_ns: u64): RateLimitError!TokenBucket {
        if (rate_per_sec <= 0.0) { return RateLimitError.InvalidRate; }
        if (burst <= 0.0) { return RateLimitError.InvalidBurst; }
        return TokenBucket{
            .capacity = burst,
            .tokens = burst,
            .refill_per_ns = rate_per_sec / 1_000_000_000.0,
            .last_ns = now_ns,
        };
    }

    pub fn refill(self: *TokenBucket, now_ns: u64) {
        if (now_ns <= self.last_ns) { return; }
        const elapsed: f64 = @floatFromInt(now_ns - self.last_ns);
        self.tokens = @min(self.capacity, self.tokens + elapsed * self.refill_per_ns);
        self.last_ns = now_ns;
    }

    pub fn allow(self: *TokenBucket, cost: f64, now_ns: u64): RateLimitError!bool {
        if (cost <= 0.0) { return RateLimitError.InvalidCost; }
        if (cost > self.capacity) { return false; }
        self.refill(now_ns);
        if (self.tokens >= cost) {
            self.tokens -= cost;
            return true;
        }
        return false;
    }

    pub fn available(self: *TokenBucket, now_ns: u64): f64 {
        self.refill(now_ns);
        return self.tokens;
    }

    /// Nanoseconds to wait before `cost` tokens are available (0 if ready now)
    pub fn retryAfterNs(self: *TokenBucket, cost: f64, now_ns: u64): u64 {
        self.refill(now_ns);
        if (self.tokens >= cost) { return 0; }
        const deficit = cost - self.tokens;
        const wait_f = @ceil(deficit / self.refill_per_ns);
        if (wait_f <= 0.0) { return 0; }
        return @intFromFloat(wait_f);
    }
};

/// Generic Cell Rate Algorithm (GCRA) — zero-drift leaky-bucket equivalent using TAT
pub const GcraLimiter = struct {
    emission_interval_ns: u64,
    burst_offset_ns: u64,
    tat_ns: u64,

    pub fn init(rate_per_sec: f64, burst: u64): RateLimitError!GcraLimiter {
        if (rate_per_sec <= 0.0) { return RateLimitError.InvalidRate; }
        if (burst == 0) { return RateLimitError.InvalidBurst; }
        const emission_ns: u64 = @intFromFloat(@round(1_000_000_000.0 / rate_per_sec));
        const safe_emission = if (emission_ns == 0) 1 else emission_ns;
        return GcraLimiter{
            .emission_interval_ns = safe_emission,
            .burst_offset_ns = safe_emission * burst,
            .tat_ns = 0,
        };
    }

    pub fn allow(self: *GcraLimiter, now_ns: u64): bool {
        let tat = self.tat_ns;
        if (tat < now_ns) { tat = now_ns; }
        const new_tat = tat + self.emission_interval_ns;
        if (new_tat > now_ns + self.burst_offset_ns) {
            return false;
        }
        self.tat_ns = new_tat;
        return true;
    }
};
