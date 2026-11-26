// @cap/backoff — Zero-allocation deterministic retry backoff & AWS jitter engine for CP
// Clean-room MIT implementation inspired by Rust `backoff` and npm `exponential-backoff`.

pub enum JitterKind {
    None,
    Full,
    Equal,
    Decorrelated,
}

pub struct BackoffPolicy {
    pub initial_ms: u64,
    pub max_ms: u64,
    pub multiplier_num: u64,
    pub multiplier_den: u64,
    pub max_retries: u32,
    pub jitter: JitterKind,

    pub static fn defaultPolicy() -> BackoffPolicy {
        return BackoffPolicy{
            initial_ms: 100 as u64,
            max_ms: 30000 as u64,
            multiplier_num: 2 as u64,
            multiplier_den: 1 as u64,
            max_retries: 5 as u32,
            jitter: JitterKind.Full,
        };
    }
}

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) * (0x100000000 as u64)) | (lo as u64);
}

fn addWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const lo = (a & mask32) + (b & mask32);
    const carry = lo / shift32;
    const hi = ((a / shift32) + (b / shift32) + carry) & mask32;
    return (hi * shift32) | (lo & mask32);
}

fn mulWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const a_lo = a & mask32;
    const a_hi = a / shift32;
    const b_lo = b & mask32;
    const b_hi = b / shift32;
    const p_lo = a_lo * b_lo;
    const mid1 = (a_lo * b_hi) & mask32;
    const mid2 = (a_hi * b_lo) & mask32;
    const hi_sum = ((p_lo / shift32) + mid1 + mid2) & mask32;
    return (hi_sum * shift32) | (p_lo & mask32);
}

fn nextRandU64(state: *u64) -> u64 {
    const c1 = makeU64(0x9E3779B9 as u32, 0x7F4A7C15 as u32);
    const m1 = makeU64(0xBF58476D as u32, 0x1CE4E5B9 as u32);
    const m2 = makeU64(0x94D049BB as u32, 0x133111EB as u32);
    state.* = addWrap64(state.*, c1);
    let z = state.*;
    z = mulWrap64(z ^ (z / (0x40000000 as u64)), m1);
    z = mulWrap64(z ^ (z / (0x8000000 as u64)), m2);
    return z ^ (z / (0x80000000 as u64));
}

fn randRangeInclusive(state: *u64, lo: u64, hi: u64) -> u64 {
    if (hi <= lo) {
        return lo;
    }
    const diff = hi - lo;
    const r = nextRandU64(state);
    if (diff == makeU64(0xFFFFFFFF as u32, 0xFFFFFFFF as u32)) {
        return r;
    }
    const span = diff + (1 as u64);
    return lo + (r % span);
}

pub fn expDelayMs(attempt: u32, initial_ms: u64, max_ms: u64) -> u64 {
    if (initial_ms == (0 as u64) || max_ms == (0 as u64)) {
        return 0 as u64;
    }
    if (initial_ms >= max_ms) {
        return max_ms;
    }
    let cur = initial_ms;
    let i: u32 = 0;
    while (i < attempt) {
        if (cur >= max_ms || cur > (max_ms / (2 as u64))) {
            return max_ms;
        }
        cur = cur * (2 as u64);
        i += 1 as u32;
    }
    return cur;
}

pub fn linearDelayMs(attempt: u32, step_ms: u64, max_ms: u64) -> u64 {
    if (step_ms == (0 as u64) || max_ms == (0 as u64)) {
        return 0 as u64;
    }
    const k = (attempt as u64) + (1 as u64);
    if (k > (max_ms / step_ms)) {
        return max_ms;
    }
    return k * step_ms;
}

fn computePolicyRawCap(policy: BackoffPolicy, attempt: u32) -> u64 {
    if (policy.initial_ms == (0 as u64) || policy.max_ms == (0 as u64)) {
        return 0 as u64;
    }
    const den = if (policy.multiplier_den == (0 as u64)) { 1 as u64 } else { policy.multiplier_den };
    const num = policy.multiplier_num;
    if (num == (0 as u64)) {
        return 0 as u64;
    }
    let cur = if (policy.initial_ms > policy.max_ms) { policy.max_ms } else { policy.initial_ms };
    if (num == den) {
        return cur;
    }
    const limitSafe = makeU64(0x7FFFFFFF as u32, 0xFFFFFFFF as u32);
    let i: u32 = 0;
    while (i < attempt) {
        if (num > den) {
            if (cur >= policy.max_ms) {
                return policy.max_ms;
            }
            if (cur > (limitSafe / num)) {
                return policy.max_ms;
            }
            const nextVal = (cur * num) / den;
            if (nextVal >= policy.max_ms) {
                return policy.max_ms;
            }
            if (nextVal <= cur) {
                cur += 1 as u64;
            } else {
                cur = nextVal;
            }
        } else {
            if (cur > (limitSafe / num)) {
                cur = (cur / den) * num;
            } else {
                cur = (cur * num) / den;
            }
        }
        i += 1 as u32;
    }
    return if (cur > policy.max_ms) { policy.max_ms } else { cur };
}

pub struct BackoffState {
    pub policy: BackoffPolicy,
    pub attempt: u32,
    pub prev_delay_ms: u64,
    pub rng_state: u64,

    pub static fn init(policy: BackoffPolicy, seed: u64) -> BackoffState {
        const startDelay = if (policy.initial_ms > policy.max_ms) { policy.max_ms } else { policy.initial_ms };
        return BackoffState{
            policy: policy,
            attempt: 0 as u32,
            prev_delay_ms: startDelay,
            rng_state: seed,
        };
    }

    pub fn nextDelayMs(*_) -> u64? {
        if (_.attempt >= _.policy.max_retries) {
            return null;
        }
        const curAttempt = _.attempt;
        const rawCap = computePolicyRawCap(_.policy, curAttempt);
        let delay: u64 = rawCap;

        switch (_.policy.jitter) {
            JitterKind.None => {
                delay = rawCap;
            },
            JitterKind.Full => {
                delay = randRangeInclusive(&_.rng_state, 0 as u64, rawCap);
            },
            JitterKind.Equal => {
                const half = rawCap / (2 as u64);
                delay = randRangeInclusive(&_.rng_state, half, rawCap);
            },
            JitterKind.Decorrelated => {
                const base = if (_.policy.initial_ms > _.policy.max_ms) { _.policy.max_ms } else { _.policy.initial_ms };
                const prev = if (curAttempt == (0 as u32)) { base } else { _.prev_delay_ms };
                const maxMs = _.policy.max_ms;
                let hi = if (prev > (maxMs / (3 as u64))) { maxMs } else { prev * (3 as u64) };
                if (hi < base) {
                    hi = base;
                }
                delay = randRangeInclusive(&_.rng_state, base, hi);
                if (delay > maxMs) {
                    delay = maxMs;
                }
            },
        }

        _.prev_delay_ms = delay;
        _.attempt += 1 as u32;
        return delay;
    }

    pub fn reset(*_) {
        _.attempt = 0 as u32;
        _.prev_delay_ms = if (_.policy.initial_ms > _.policy.max_ms) { _.policy.max_ms } else { _.policy.initial_ms };
    }

    pub fn attempts() -> u32 {
        return _.attempt;
    }
}

export {
    JitterKind,
    BackoffPolicy,
    BackoffState,
    expDelayMs,
    linearDelayMs,
};
