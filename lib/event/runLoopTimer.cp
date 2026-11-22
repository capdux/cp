// Source: Absorbed from /data/dev/bun/src/runtime/timer/WTFTimer.cp | Synced: 2026-11-22T20:07:08Z (High-precision f64-second RunLoopTimer with imminent zero-delay fast path, i32-second clamping, timespec carry normalization & repeat/cancel lifecycle)
// Pure-CP zero-allocation RunLoopTimer modeling WebKit/Bun WTFTimer scheduling and imminent GC timer fast path.

/// Maximum clamped timeout duration in seconds (aligned with Bun `WTFTimer`: `std.math.maxInt(i32)`).
pub const RUN_LOOP_TIMER_MAX_SECONDS: f64 = 2_147_483_647.0;

/// Nanoseconds per second for timespec carry normalization.
pub const RUN_LOOP_TIMER_NS_PER_SEC: i64 = 1_000_000_000;

/// Lifecycle state of a `RunLoopTimer`.
pub enum RunLoopTimerState: u8 {
    Cancelled = 0,
    Imminent = 1,
    Active = 2,
    Fired = 3,
}

/// Normalized `(sec, nsec)` monotonic timestamp for `RunLoopTimer` scheduling.
pub struct RunLoopTimeSpec {
    sec: i64 = 0,
    nsec: i64 = 0,

    static fn init(sec: i64, nsec: i64): RunLoopTimeSpec {
        let s = sec;
        let ns = nsec;
        while (ns >= RUN_LOOP_TIMER_NS_PER_SEC) {
            s += 1;
            ns -= RUN_LOOP_TIMER_NS_PER_SEC;
        }
        while (ns < 0) {
            s -= 1;
            ns += RUN_LOOP_TIMER_NS_PER_SEC;
        }
        return RunLoopTimeSpec{ sec: s, nsec: ns };
    }

    /// Adds a floating-point second offset clamped to `[0, RUN_LOOP_TIMER_MAX_SECONDS]`
    /// with nanosecond carry normalization (aligned with Bun `WTFTimer.update`).
    fn addSeconds(seconds: f64): RunLoopTimeSpec {
        if (!(seconds > 0.0)) {
            return RunLoopTimeSpec{ sec: _.sec, nsec: _.nsec };
        }
        const clamped = if (seconds > RUN_LOOP_TIMER_MAX_SECONDS) RUN_LOOP_TIMER_MAX_SECONDS else seconds;
        const ipart_i64: i64 = @as(i64, @intFromFloat(clamped));
        const fpart = clamped - @as(f64, @floatFromInt(ipart_i64));
        const fpart_ns: i64 = @as(i64, @intFromFloat(fpart * 1_000_000_000.0));

        let s = _.sec + ipart_i64;
        let ns = _.nsec + fpart_ns;
        if (ns >= RUN_LOOP_TIMER_NS_PER_SEC) {
            s += 1;
            ns -= RUN_LOOP_TIMER_NS_PER_SEC;
        }
        return RunLoopTimeSpec{ sec: s, nsec: ns };
    }

    /// Compares two timestamps (`-1` if `<`, `0` if `==`, `1` if `>`).
    fn compare(other: RunLoopTimeSpec): i8 {
        if (_.sec < other.sec) return -1;
        if (_.sec > other.sec) return 1;
        if (_.nsec < other.nsec) return -1;
        if (_.nsec > other.nsec) return 1;
        return 0;
    }

    /// Returns true if `now >= self`.
    fn isReached(now: RunLoopTimeSpec): bool {
        return now.compare(_) >= 0;
    }
}

/// Zero-allocation high-precision `RunLoopTimer` (aligned with Bun `WTFTimer.cp`).
pub struct RunLoopTimer {
    context_id: u32 = 0,
    state: RunLoopTimerState = RunLoopTimerState.Cancelled,
    next: RunLoopTimeSpec = RunLoopTimeSpec{},
    interval_sec: f64 = 0.0,
    repeat: bool = false,
    imminent: bool = false,
    fire_count: u64 = 0,

    /// Initializes a cancelled `RunLoopTimer` bound to `context_id` (`0` = detached/invalid context).
    static fn init(context_id: u32): RunLoopTimer {
        return RunLoopTimer{
            context_id: context_id,
            state: RunLoopTimerState.Cancelled,
        };
    }

    fn hasValidContext(): bool {
        return _.context_id != 0;
    }

    /// Schedules or reschedules the timer (aligned with Bun `WTFTimer.update`).
    /// Non-positive or NaN `seconds` enters the zero-delay `Imminent` fast path.
    fn update(*_, now: RunLoopTimeSpec, seconds: f64, repeat: bool) {
        if (!(seconds > 0.0)) {
            _.imminent = true;
            _.repeat = repeat;
            _.interval_sec = 0.0;
            _.next = now;
            _.state = RunLoopTimerState.Imminent;
            return;
        }

        _.imminent = false;
        const clamped = if (seconds > RUN_LOOP_TIMER_MAX_SECONDS) RUN_LOOP_TIMER_MAX_SECONDS else seconds;
        _.interval_sec = clamped;
        _.repeat = repeat;
        _.next = now.addSeconds(clamped);
        _.state = RunLoopTimerState.Active;
    }

    fn isImminent(): bool {
        return _.imminent && _.state == RunLoopTimerState.Imminent;
    }

    fn isDue(now: RunLoopTimeSpec): bool {
        if (_.isImminent()) return true;
        return _.state == RunLoopTimerState.Active && _.next.isReached(now);
    }

    /// Fires immediately if this timer is in the `Imminent` fast path (aligned with `WTFTimer__runIfImminent`).
    fn runIfImminent(*_): bool {
        if (!_.isImminent()) return false;
        _.imminent = false;
        _.state = RunLoopTimerState.Fired;
        _.fire_count +|= 1;
        return true;
    }

    /// Fires the timer if due at `now`. Repeating timers reschedule automatically.
    fn fire(*_, now: RunLoopTimeSpec): bool {
        if (!_.isDue(now)) return false;
        _.imminent = false;
        _.fire_count +|= 1;
        if (_.repeat && _.interval_sec > 0.0) {
            _.next = now.addSeconds(_.interval_sec);
            _.state = RunLoopTimerState.Active;
        } else {
            _.state = RunLoopTimerState.Fired;
        }
        return true;
    }

    /// Cancels the timer when bound to a valid execution context (aligned with `WTFTimer.cancel`).
    fn cancel(*_) {
        if (_.hasValidContext()) {
            _.imminent = false;
            _.state = RunLoopTimerState.Cancelled;
        }
    }
}
