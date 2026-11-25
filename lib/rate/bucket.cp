// Source: cp/lib/rate/bucket.cp (Pure CP Precision Token Bucket & Legacy GCRA Rate Limiters)

pub err RateLimitError {
    InvalidRate,
    InvalidBurst,
    InvalidCost,
}

/// Deterministic Token Bucket with nanosecond refill precision (pure CP, zero heap allocation).
pub struct TokenBucket {
    capacity: f64;
    tokens: f64;
    refill_per_ns: f64;
    last_ns: u64;

    pub static fn init(rate_per_sec: f64, burst: f64, now_ns: u64): TokenBucket ! RateLimitError {
        if (!(rate_per_sec > 0.0)) {
            return RateLimitError.InvalidRate;
        }
        if (!(burst > 0.0)) {
            return RateLimitError.InvalidBurst;
        }
        return TokenBucket{
            capacity: burst,
            tokens: burst,
            refill_per_ns: rate_per_sec / 1_000_000_000.0,
            last_ns: now_ns,
        };
    }

    pub fn refill(*_, now_ns: u64) {
        if (now_ns <= _.last_ns) {
            return;
        }
        const elapsed: f64 = @floatFromInt(now_ns - _.last_ns);
        const next_tokens = _.tokens + elapsed * _.refill_per_ns;
        _.tokens = if (next_tokens > _.capacity) _.capacity else next_tokens;
        _.last_ns = now_ns;
    }

    pub fn allow(*_, cost: f64, now_ns: u64): bool ! RateLimitError {
        if (!(cost > 0.0)) {
            return RateLimitError.InvalidCost;
        }
        if (cost > _.capacity) {
            return false;
        }
        _.refill(now_ns);
        if (_.tokens >= cost) {
            _.tokens -= cost;
            return true;
        }
        return false;
    }

    pub fn available(*_, now_ns: u64): f64 {
        _.refill(now_ns);
        return _.tokens;
    }

    /// Nanoseconds to wait before `cost` tokens are available (`0` if ready now).
    pub fn retryAfterNs(*_, cost: f64, now_ns: u64): u64 {
        _.refill(now_ns);
        if (_.tokens >= cost) {
            return 0;
        }
        const deficit = cost - _.tokens;
        const wait_f = @ceil(deficit / _.refill_per_ns);
        if (!(wait_f > 0.0)) {
            return 0;
        }
        return @intFromFloat(wait_f);
    }
}

/// Legacy float-initialized GCRA limiter retained for direct `./bucket.cp` imports.
/// Prefer `GcraLimiter` + `Quota` from `cap:rate` (`./gcra.cp`) for new code.
pub struct BucketGcraLimiter {
    emission_interval_ns: u64;
    burst_offset_ns: u64;
    tat_ns: u64;

    pub static fn init(rate_per_sec: f64, burst: u64): BucketGcraLimiter ! RateLimitError {
        if (!(rate_per_sec > 0.0)) {
            return RateLimitError.InvalidRate;
        }
        if (burst == 0) {
            return RateLimitError.InvalidBurst;
        }
        const emission_ns: u64 = @intFromFloat(@round(1_000_000_000.0 / rate_per_sec));
        const safe_emission: u64 = if (emission_ns == 0) 1 else emission_ns;
        return BucketGcraLimiter{
            emission_interval_ns: safe_emission,
            burst_offset_ns: safe_emission * burst,
            tat_ns: 0,
        };
    }

    pub fn allow(*_, now_ns: u64): bool {
        let tat = _.tat_ns;
        if (tat < now_ns) {
            tat = now_ns;
        }
        const new_tat = tat + _.emission_interval_ns;
        if (new_tat > now_ns + _.burst_offset_ns) {
            return false;
        }
        _.tat_ns = new_tat;
        return true;
    }
}
