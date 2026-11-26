// @cap/governor — Zero-allocation deterministic GCRA & Token Bucket rate limiters for CP
// Clean-room MIT implementation inspired by Rust `governor` and npm `rate-limiter-flexible`.

pub struct RateDecision {
    pub allowed: bool,
    pub remaining: u64,
    pub retry_after_ms: u64,
    pub reset_after_ms: u64,
}

fn ceilToU64(v: f64) -> u64 {
    if (v <= 0.0) {
        return 0;
    }
    const base = v as u64;
    if ((base as f64) < v) {
        return base + 1;
    }
    return base;
}

// ---------------------------------------------------------------------------
// 1. GCRA (Generic Cell Rate Algorithm) — Single-field `tat_ms` state
// ---------------------------------------------------------------------------

pub struct GcraLimiter {
    pub emission_interval_ms: u64,
    pub burst_tolerance_ms: u64,
    pub tat_ms: u64,

    /// Creates a GCRA limiter allowing `limit` cells per `period_ms` with burst capacity `burst`.
    pub static fn perPeriod(limit: u64, period_ms: u64, burst: u64) -> GcraLimiter {
        const eff_limit = if (limit == 0) 1 as u64 else limit;
        const eff_burst = if (burst == 0) 1 as u64 else burst;
        let interval = period_ms / eff_limit;
        if (interval == 0) {
            interval = 1;
        }
        return GcraLimiter{
            emission_interval_ms: interval,
            burst_tolerance_ms: interval * (eff_burst - 1),
            tat_ms: 0,
        };
    }

    /// Creates a GCRA limiter allowing `rate_per_sec` requests/sec with burst capacity `burst`.
    pub static fn init(rate_per_sec: u64, burst: u64) -> GcraLimiter {
        return GcraLimiter.perPeriod(rate_per_sec, 1000 as u64, burst);
    }

    /// Resets the theoretical arrival time state.
    pub fn reset(*_) {
        _.tat_ms = 0;
    }

    /// Checks and consumes `cost` cells at monotonic timestamp `now_ms`.
    pub fn checkNAt(*_, cost: u64, now_ms: u64) -> RateDecision {
        const t = _.emission_interval_ms;
        const max_tat = now_ms + _.burst_tolerance_ms + t;
        const cur_tat = if (_.tat_ms > now_ms) _.tat_ms else now_ms;

        if (cost == 0) {
            const rem = if (max_tat > cur_tat) (max_tat - cur_tat) / t else 0 as u64;
            const rst = if (cur_tat > now_ms) cur_tat - now_ms else 0 as u64;
            return RateDecision{
                allowed: true,
                remaining: rem,
                retry_after_ms: 0,
                reset_after_ms: rst,
            };
        }

        const increment = cost * t;
        const new_tat = cur_tat + increment;

        if (new_tat <= max_tat) {
            _.tat_ms = new_tat;
            const rem = (max_tat - new_tat) / t;
            const rst = if (new_tat > now_ms) new_tat - now_ms else 0 as u64;
            return RateDecision{
                allowed: true,
                remaining: rem,
                retry_after_ms: 0,
                reset_after_ms: rst,
            };
        }

        const retry = new_tat - max_tat;
        const rem = if (max_tat > cur_tat) (max_tat - cur_tat) / t else 0 as u64;
        const rst = if (cur_tat > now_ms) cur_tat - now_ms else 0 as u64;
        return RateDecision{
            allowed: false,
            remaining: rem,
            retry_after_ms: retry,
            reset_after_ms: rst,
        };
    }

    /// Checks and consumes 1 cell at monotonic timestamp `now_ms`.
    pub fn checkAt(*_, now_ms: u64) -> RateDecision {
        return _.checkNAt(1 as u64, now_ms);
    }
}

// ---------------------------------------------------------------------------
// 2. Classic Floating-Point Token Bucket
// ---------------------------------------------------------------------------

pub struct TokenBucket {
    pub capacity: f64,
    pub refill_per_sec: f64,
    pub tokens: f64,
    pub last_ms: u64,

    /// Creates a full `TokenBucket` with `capacity` tokens refilling at `refill_per_sec`.
    pub static fn init(capacity: f64, refill_per_sec: f64, now_ms: u64) -> TokenBucket {
        const cap = if (capacity < 0.0) 0.0 else capacity;
        const rate = if (refill_per_sec < 0.0) 0.0 else refill_per_sec;
        return TokenBucket{
            capacity: cap,
            refill_per_sec: rate,
            tokens: cap,
            last_ms: now_ms,
        };
    }

    pub fn refill(*_, now_ms: u64) {
        if (now_ms > _.last_ms) {
            const elapsed_sec = ((now_ms - _.last_ms) as f64) / 1000.0;
            _.tokens += elapsed_sec * _.refill_per_sec;
            if (_.tokens > _.capacity) {
                _.tokens = _.capacity;
            }
            _.last_ms = now_ms;
        }
    }

    /// Returns the currently available token balance at `now_ms`.
    pub fn available(*_, now_ms: u64) -> f64 {
        _.refill(now_ms);
        return _.tokens;
    }

    /// Resets the bucket to full capacity at `now_ms`.
    pub fn reset(*_, now_ms: u64) {
        _.tokens = _.capacity;
        _.last_ms = now_ms;
    }

    /// Attempts to acquire `tokens` at `now_ms`.
    pub fn tryAcquire(*_, tokens: f64, now_ms: u64) -> RateDecision {
        _.refill(now_ms);
        if (tokens <= 0.0) {
            const missing = _.capacity - _.tokens;
            const rst = if (_.refill_per_sec > 0.0 && missing > 0.0)
                ceilToU64(missing * 1000.0 / _.refill_per_sec)
            else
                0 as u64;
            return RateDecision{
                allowed: true,
                remaining: _.tokens as u64,
                retry_after_ms: 0,
                reset_after_ms: rst,
            };
        }

        if (_.tokens >= tokens) {
            _.tokens -= tokens;
            const missing = _.capacity - _.tokens;
            const rst = if (_.refill_per_sec > 0.0 && missing > 0.0)
                ceilToU64(missing * 1000.0 / _.refill_per_sec)
            else
                0 as u64;
            return RateDecision{
                allowed: true,
                remaining: _.tokens as u64,
                retry_after_ms: 0,
                reset_after_ms: rst,
            };
        }

        const deficit = tokens - _.tokens;
        const retry = if (_.refill_per_sec > 0.0)
            ceilToU64(deficit * 1000.0 / _.refill_per_sec)
        else
            0 as u64;
        const missing = _.capacity - _.tokens;
        const rst = if (_.refill_per_sec > 0.0 && missing > 0.0)
            ceilToU64(missing * 1000.0 / _.refill_per_sec)
        else
            0 as u64;
        return RateDecision{
            allowed: false,
            remaining: _.tokens as u64,
            retry_after_ms: retry,
            reset_after_ms: rst,
        };
    }
}
