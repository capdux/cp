// packages/@cap/p-timeout-core/src/index.cp
// Zero-allocation deadline tracker and timeout state machine guard for CP.
// Clean-room MIT pure CP implementation of npm `p-timeout` v6 core semantics.

pub const PT_PENDING: u8 = 0;
pub const PT_RESOLVED: u8 = 1;
pub const PT_REJECTED: u8 = 2;
pub const PT_TIMED_OUT: u8 = 3;
pub const PT_CANCELED: u8 = 4;

fn ptoMaxU64(): u64 {
    let lo: u64 = 4294967295;
    return (lo << 32) | lo;
}

fn ptoSaturatingAdd(a: u64, b: u64): u64 {
    let max_val: u64 = ptoMaxU64();
    if (b > max_val - a) {
        return max_val;
    }
    return a + b;
}

/// Configuration options for `TimeoutGuard`.
pub struct TimeoutOptions {
    pub milliseconds: u64,
    pub has_fallback: bool,
    pub fallback_code: i32,
    pub error_message: string,

    pub static fn withMs(ms: u64): TimeoutOptions {
        return TimeoutOptions {
            milliseconds: ms,
            has_fallback: false,
            fallback_code: 0,
            error_message: "Promise timed out",
        };
    }
}

/// Zero-heap deadline and settlement state machine guard.
pub struct TimeoutGuard {
    pub start_ms: u64,
    pub deadline_ms: u64,
    pub opts: TimeoutOptions,
    pub state: u8,
    pub result_code: i32,
    pub used_fallback: bool,

    pub static fn new(start_ms: u64, opts: TimeoutOptions): TimeoutGuard {
        let dl: u64 = ptoSaturatingAdd(start_ms, opts.milliseconds);
        return TimeoutGuard {
            start_ms: start_ms,
            deadline_ms: dl,
            opts: opts,
            state: PT_PENDING,
            result_code: 0,
            used_fallback: false,
        };
    }

    /// Returns the absolute deadline timestamp in milliseconds.
    pub fn deadlineMs(): u64 {
        return _.deadline_ms;
    }

    /// Returns remaining milliseconds until deadline, or `0` if expired.
    pub fn remainingMs(now_ms: u64): u64 {
        if (now_ms >= _.deadline_ms) {
            return 0;
        }
        return _.deadline_ms - now_ms;
    }

    /// Returns `true` if `now_ms` has reached or passed the deadline.
    pub fn isExpired(now_ms: u64): bool {
        return now_ms >= _.deadline_ms;
    }

    /// Polls the guard at `now_ms`. If still pending and expired, transitions to
    /// `PT_RESOLVED` (with `fallback_code`) when `has_fallback` is true, or `PT_TIMED_OUT` otherwise.
    pub fn poll(*_, now_ms: u64): u8 {
        if (_.state != PT_PENDING) {
            return _.state;
        }
        if (now_ms >= _.deadline_ms) {
            if (_.opts.has_fallback) {
                _.state = PT_RESOLVED;
                _.result_code = _.opts.fallback_code;
                _.used_fallback = true;
            } else {
                _.state = PT_TIMED_OUT;
                _.result_code = 0;
            }
        }
        return _.state;
    }

    /// Resolves the guard with `value_code` if still `PT_PENDING` and not expired at `now_ms`.
    pub fn resolve(*_, now_ms: u64, value_code: i32): bool {
        if (_.state != PT_PENDING) {
            return false;
        }
        if (now_ms >= _.deadline_ms) {
            * = _.poll(now_ms);
            return false;
        }
        _.state = PT_RESOLVED;
        _.result_code = value_code;
        _.used_fallback = false;
        return true;
    }

    /// Rejects the guard with `err_code` if still `PT_PENDING` and not expired at `now_ms`.
    pub fn reject(*_, now_ms: u64, err_code: i32): bool {
        if (_.state != PT_PENDING) {
            return false;
        }
        if (now_ms >= _.deadline_ms) {
            * = _.poll(now_ms);
            return false;
        }
        _.state = PT_REJECTED;
        _.result_code = err_code;
        return true;
    }

    /// Cancels the guard if still `PT_PENDING`.
    pub fn cancel(*_): bool {
        if (_.state != PT_PENDING) {
            return false;
        }
        _.state = PT_CANCELED;
        return true;
    }

    /// Returns `true` if the guard is no longer `PT_PENDING`.
    pub fn isSettled(): bool {
        return _.state != PT_PENDING;
    }
}
