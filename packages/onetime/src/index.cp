// @cap/onetime — Ensure an action runs only once, caching result and tracking call count
// Clean-room MIT CP implementation of npm `onetime` (150M+/wk)
// Zero-heap, deterministic state machine with optional strict (error-on-recall) mode.

pub const ONCE_OK: i32 = 0;
pub const ONCE_ALREADY_CALLED: i32 = -1;

pub struct OnceCallResult {
    pub status: i32,
    pub executed_now: bool,
    pub value: i64,
    pub call_count: u64,
}

/// Zero-heap single-execution guard with cached `i64` result and `callCount` tracking.
pub struct OnceGuard {
    called: bool,
    strict: bool,
    call_count: u64,
    cached_value: i64,

    pub static fn new(): OnceGuard {
        return OnceGuard {
            called: false,
            strict: false,
            call_count: 0,
            cached_value: 0,
        };
    }

    /// Strict mode: subsequent invocations return `ONCE_ALREADY_CALLED` instead of silently returning cached value.
    pub static fn newStrict(): OnceGuard {
        return OnceGuard {
            called: false,
            strict: true,
            call_count: 0,
            cached_value: 0,
        };
    }

    pub fn isCalled(): bool {
        return _.called;
    }

    /// Mirrors `onetime.callCount(fn)` — returns total number of times invocation was attempted.
    pub fn callCount(): u64 {
        return _.call_count;
    }

    pub fn cachedValue(): i64 {
        return _.cached_value;
    }

    /// Acquire the one-time execution gate.
    /// - First call: marks called, increments `call_count`, stores `val`, returns `executed_now = true`.
    /// - Subsequent calls: increments `call_count`; in strict mode returns `ONCE_ALREADY_CALLED`,
    ///   otherwise returns `ONCE_OK` with the original `cached_value` and `executed_now = false`.
    pub fn runWithValue(*_, val: i64): OnceCallResult {
        _.call_count = _.call_count + 1;
        if (!_.called) {
            _.called = true;
            _.cached_value = val;
            return OnceCallResult {
                status: ONCE_OK,
                executed_now: true,
                value: val,
                call_count: _.call_count,
            };
        }
        if (_.strict) {
            return OnceCallResult {
                status: ONCE_ALREADY_CALLED,
                executed_now: false,
                value: _.cached_value,
                call_count: _.call_count,
            };
        }
        return OnceCallResult {
            status: ONCE_OK,
            executed_now: false,
            value: _.cached_value,
            call_count: _.call_count,
        };
    }

    /// Simple boolean gate: returns `true` ONLY on the first call, `false` on all subsequent calls.
    pub fn shouldRun(*_): bool {
        _.call_count = _.call_count + 1;
        if (!_.called) {
            _.called = true;
            return true;
        }
        return false;
    }

    /// Reset the guard back to uncalled state.
    pub fn reset(*_) {
        _.called = false;
        _.call_count = 0;
        _.cached_value = 0;
    }
}

/// Named OnceRegistry holding up to 64 independent one-time guards by integer key.
pub struct OnceRegistry {
    keys: [64]u64,
    values: [64]i64,
    counts: [64]u64,
    len: usize,

    pub static fn new(): OnceRegistry {
        return OnceRegistry {
            keys: undefined,
            values: undefined,
            counts: undefined,
            len: 0,
        };
    }

    pub fn size(): usize {
        return _.len;
    }

    pub fn has(key: u64): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.keys[i] == key) { return true; }
            i = i + 1;
        }
        return false;
    }

    pub fn callCountOf(key: u64): u64 {
        let i: usize = 0;
        while (i < _.len) {
            if (_.keys[i] == key) { return _.counts[i]; }
            i = i + 1;
        }
        return 0;
    }

    /// Execute once per distinct `key`. Returns `executed_now = true` on first appearance.
    pub fn runForKey(*_, key: u64, val: i64): OnceCallResult {
        let i: usize = 0;
        while (i < _.len) {
            if (_.keys[i] == key) {
                _.counts[i] = _.counts[i] + 1;
                return OnceCallResult {
                    status: ONCE_OK,
                    executed_now: false,
                    value: _.values[i],
                    call_count: _.counts[i],
                };
            }
            i = i + 1;
        }
        if (_.len >= 64) {
            return OnceCallResult {
                status: ONCE_ALREADY_CALLED,
                executed_now: false,
                value: 0,
                call_count: 0,
            };
        }
        let idx = _.len;
        _.keys[idx] = key;
        _.values[idx] = val;
        _.counts[idx] = 1;
        _.len = _.len + 1;
        return OnceCallResult {
            status: ONCE_OK,
            executed_now: true,
            value: val,
            call_count: 1,
        };
    }
}
