// @cap/debounce — Zero-allocation Deterministic Debouncer & Throttler State Machines
// Clean-room MIT implementation in pure CP (caller-driven `now_ms: u64`, zero OS clock dependency).

fn elapsedSince(now_ms: u64, past_ms: u64) -> u64 {
    if (now_ms >= past_ms) {
        return now_ms - past_ms;
    }
    return 0;
}

pub struct Debouncer {
    wait_ms: u64,
    leading: bool,
    trailing: bool,
    max_wait_ms: u64,
    active: bool,
    pending: bool,
    last_call_ms: u64,
    last_exec_ms: u64,
    exec_count: u64,

    /// Initializes a Debouncer. `max_wait_ms == 0` disables maximum wait clamping.
    pub static fn init(wait_ms: u64, leading: bool, trailing: bool, max_wait_ms: u64) -> Debouncer {
        return Debouncer {
            wait_ms: wait_ms,
            leading: leading,
            trailing: trailing,
            max_wait_ms: max_wait_ms,
            active: false,
            pending: false,
            last_call_ms: 0,
            last_exec_ms: 0,
            exec_count: 0,
        };
    }

    /// Records an incoming event at `now_ms`.
    /// Returns true if the action should execute immediately (leading edge or maxWait reached).
    pub fn trigger(*_, now_ms: u64) -> bool {
        if (_.wait_ms == 0) {
            if (_.leading || _.trailing) {
                _.last_call_ms = now_ms;
                _.last_exec_ms = now_ms;
                _.active = false;
                _.pending = false;
                _.exec_count += 1;
                return true;
            }
            return false;
        }

        const since_call = elapsedSince(now_ms, _.last_call_ms);
        const since_exec = elapsedSince(now_ms, _.last_exec_ms);

        // Fresh debounce cycle (idle or previous quiet window has elapsed)
        if (!_.active || since_call >= _.wait_ms) {
            _.active = true;
            _.last_call_ms = now_ms;
            _.last_exec_ms = now_ms;
            if (_.leading) {
                _.pending = false;
                _.exec_count += 1;
                return true;
            }
            _.pending = _.trailing;
            return false;
        }

        // Inside active cycle: check maxWait ceiling
        if (_.max_wait_ms > 0 && since_exec >= _.max_wait_ms) {
            _.last_call_ms = now_ms;
            _.last_exec_ms = now_ms;
            _.pending = false;
            _.exec_count += 1;
            return true;
        }

        // Suppress and arm trailing edge
        _.last_call_ms = now_ms;
        if (_.trailing) {
            _.pending = true;
        }
        return false;
    }

    /// Advances the clock to `now_ms` and fires any due trailing or maxWait invocation.
    pub fn poll(*_, now_ms: u64) -> bool {
        if (!_.active) {
            return false;
        }

        const since_call = elapsedSince(now_ms, _.last_call_ms);
        const since_exec = elapsedSince(now_ms, _.last_exec_ms);

        if (since_call >= _.wait_ms) {
            _.active = false;
            if (_.pending && _.trailing) {
                _.pending = false;
                _.last_exec_ms = now_ms;
                _.exec_count += 1;
                return true;
            }
            _.pending = false;
            return false;
        }

        if (_.max_wait_ms > 0 && since_exec >= _.max_wait_ms && _.pending) {
            _.pending = false;
            _.last_exec_ms = now_ms;
            _.exec_count += 1;
            return true;
        }

        return false;
    }

    /// Immediately executes a pending trailing invocation if one is armed.
    pub fn flush(*_, now_ms: u64) -> bool {
        if (_.pending) {
            _.pending = false;
            _.active = false;
            _.last_exec_ms = now_ms;
            _.exec_count += 1;
            return true;
        }
        return false;
    }

    /// Cancels any pending invocation and resets the active debounce window.
    pub fn cancel(*_) {
        _.pending = false;
        _.active = false;
    }

    /// Returns true if a trailing invocation is currently pending.
    pub fn isPending() -> bool {
        return _.pending;
    }

    /// Returns the total number of executions fired so far.
    pub fn execCount() -> u64 {
        return _.exec_count;
    }

    /// Returns the earliest timestamp `now_ms` at which the pending invocation becomes due,
    /// or `0` if nothing is pending.
    pub fn nextDueMs() -> u64 {
        if (!_.pending) {
            return 0;
        }
        let due = _.last_call_ms + _.wait_ms;
        if (_.max_wait_ms > 0) {
            const max_due = _.last_exec_ms + _.max_wait_ms;
            if (max_due < due) {
                due = max_due;
            }
        }
        return due;
    }
}

pub struct Throttler {
    interval_ms: u64,
    leading: bool,
    trailing: bool,
    has_window: bool,
    window_start_ms: u64,
    pending: bool,
    exec_count: u64,

    /// Initializes a fixed-interval Throttler.
    pub static fn init(interval_ms: u64, leading: bool, trailing: bool) -> Throttler {
        return Throttler {
            interval_ms: interval_ms,
            leading: leading,
            trailing: trailing,
            has_window: false,
            window_start_ms: 0,
            pending: false,
            exec_count: 0,
        };
    }

    /// Records an event at `now_ms`. Executes immediately if outside the throttle window
    /// and `leading` is enabled; otherwise suppresses (and arms `trailing` if enabled).
    pub fn trigger(*_, now_ms: u64) -> bool {
        if (_.interval_ms == 0) {
            if (_.leading || _.trailing) {
                _.exec_count += 1;
                _.pending = false;
                return true;
            }
            return false;
        }

        const since_win = elapsedSince(now_ms, _.window_start_ms);
        if (!_.has_window || since_win >= _.interval_ms) {
            _.has_window = true;
            _.window_start_ms = now_ms;
            if (_.leading) {
                _.pending = false;
                _.exec_count += 1;
                return true;
            }
            _.pending = _.trailing;
            return false;
        }

        if (_.trailing) {
            _.pending = true;
        }
        return false;
    }

    /// Advances the clock to `now_ms` and fires any pending trailing execution when the window ends.
    pub fn poll(*_, now_ms: u64) -> bool {
        if (!_.has_window || !_.pending) {
            return false;
        }
        const since_win = elapsedSince(now_ms, _.window_start_ms);
        if (since_win >= _.interval_ms) {
            _.pending = false;
            _.window_start_ms = now_ms;
            _.exec_count += 1;
            return true;
        }
        return false;
    }

    /// Cancels any pending trailing execution and clears the active throttle window.
    pub fn cancel(*_) {
        _.pending = false;
        _.has_window = false;
    }

    /// Returns true if a trailing execution is pending.
    pub fn isPending() -> bool {
        return _.pending;
    }

    /// Returns the total number of executions fired so far.
    pub fn execCount() -> u64 {
        return _.exec_count;
    }
}
