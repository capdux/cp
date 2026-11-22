// Source: Absorbed from /data/dev/bun/src/runtime/timer/TimeoutObject.cp | Synced: 2026-11-22T19:51:30Z (Node/Bun setTimeout & setInterval TimeoutObject lifecycle, ref/unref/refresh state machine & fixed-slot registry)
// Pure-CP zero-allocation TimeoutObject (setTimeout/setInterval) lifecycle manager with ref/unref/refresh and u31 interval clamping.

/// Maximum signed 31-bit millisecond timeout interval (matches Node.js & Bun `u31` limit: `2^31 - 1`).
pub const TIMEOUT_MAX_INTERVAL_MS: u32 = 2_147_483_647;

/// Maximum concurrent active/tracked timers in `TimeoutRegistry`.
pub const TIMEOUT_REGISTRY_MAX: usize = 64;

/// Timer scheduling kind (`setTimeout` one-shot vs `setInterval` repeating).
pub enum TimeoutKind: u8 {
    SetTimeout = 0,
    SetInterval = 1,
}

/// Lifecycle state of a `TimeoutObject`.
pub enum TimeoutState: u8 {
    Active = 0,
    Fired = 1,
    Cancelled = 2,
    Destroyed = 3,
}

/// Error set for `TimeoutObject` and `TimeoutRegistry`.
pub err TimeoutObjectError {
    AlreadyDestroyed,
    RegistryFull,
    NotFound,
}

/// Clamps a raw millisecond timeout according to Node.js / Bun `TimeoutOverflowWarning` rules:
/// values `<= 0` or `> 2_147_483_647` clamp to `1ms`.
pub fn clampTimeoutIntervalMs(raw_ms: i64): u32 {
    if (raw_ms <= 0 || raw_ms > @as(i64, TIMEOUT_MAX_INTERVAL_MS)) {
        return 1;
    }
    return @as(u32, @intCast(raw_ms));
}

/// Zero-allocation Node/Bun `TimeoutObject` state machine (aligned with Bun `TimeoutObject.cp`).
pub struct TimeoutObject {
    id: i32 = 0,
    kind: TimeoutKind = TimeoutKind.SetTimeout,
    interval_ms: u32 = 1,
    next_fire_ms: u64 = 0,
    state: TimeoutState = TimeoutState.Destroyed,
    has_ref: bool = false,
    ref_count: u32 = 1,
    fire_count: u64 = 0,

    /// Initializes a new active `TimeoutObject` scheduled at `now_ms + clamp(raw_interval_ms)`.
    static fn init(id: i32, kind: TimeoutKind, now_ms: u64, raw_interval_ms: i64): TimeoutObject {
        const clamped = clampTimeoutIntervalMs(raw_interval_ms);
        return TimeoutObject{
            id: id,
            kind: kind,
            interval_ms: clamped,
            next_fire_ms: now_ms +| @as(u64, clamped),
            state: TimeoutState.Active,
            has_ref: true,
            ref_count: 1,
            fire_count: 0,
        };
    }

    /// Returns the numeric timer ID (aligned with `TimeoutObject.toPrimitive`).
    fn toPrimitive(): i32 {
        return _.id;
    }

    /// Keeps the event loop alive while this timer is active (aligned with `doRef`).
    fn doRef(*_) {
        if (_.state == TimeoutState.Active) {
            _.has_ref = true;
        }
    }

    /// Allows the event loop to exit if this is the only remaining timer (aligned with `doUnref`).
    fn doUnref(*_) {
        _.has_ref = false;
    }

    /// Returns whether this timer currently holds an active event loop ref (aligned with `hasRef`).
    fn hasRef(): bool {
        return _.state == TimeoutState.Active && _.has_ref;
    }

    /// Increments internal object reference count.
    fn ref(*_) {
        _.ref_count +|= 1;
    }

    /// Decrements internal object reference count and returns true when it hits zero.
    fn deref(*_): bool {
        if (_.ref_count > 0) {
            _.ref_count -= 1;
        }
        return _.ref_count == 0;
    }

    /// Resets the timer's expiration to `now_ms + interval_ms` (aligned with `doRefresh`).
    /// Re-arms a previously fired one-shot timeout unless destroyed.
    fn doRefresh(*_, now_ms: u64): u64 ! TimeoutObjectError {
        if (_.state == TimeoutState.Destroyed) return TimeoutObjectError.AlreadyDestroyed;
        _.next_fire_ms = now_ms +| @as(u64, _.interval_ms);
        _.state = TimeoutState.Active;
        return _.next_fire_ms;
    }

    /// Returns true if the timer is active and due at `now_ms`.
    fn isDue(now_ms: u64): bool {
        return _.state == TimeoutState.Active && now_ms >= _.next_fire_ms;
    }

    /// Fires the timer if due at `now_ms`. Returns true if fired.
    /// Repeating `SetInterval` timers reschedule automatically; one-shot `SetTimeout` transitions to `Fired`.
    fn fire(*_, now_ms: u64): bool {
        if (!_.isDue(now_ms)) return false;
        _.fire_count +|= 1;
        if (_.kind == TimeoutKind.SetInterval) {
            _.next_fire_ms = now_ms +| @as(u64, _.interval_ms);
        } else {
            _.state = TimeoutState.Fired;
            _.has_ref = false;
        }
        return true;
    }

    /// Cancels the timer (`clearTimeout` / `clearInterval`).
    fn cancel(*_) {
        if (_.state != TimeoutState.Destroyed) {
            _.state = TimeoutState.Cancelled;
            _.has_ref = false;
        }
    }

    /// Finalizes and destroys the timer (aligned with `TimeoutObject.finalize`).
    fn finalize(*_) {
        _.state = TimeoutState.Destroyed;
        _.has_ref = false;
    }
}

/// Zero-allocation fixed-slot `TimeoutObject` registry (`setTimeout` / `setInterval` scheduler).
pub struct TimeoutRegistry {
    slots: [TIMEOUT_REGISTRY_MAX]TimeoutObject = [TimeoutObject{}; TIMEOUT_REGISTRY_MAX],
    occupied: [TIMEOUT_REGISTRY_MAX]bool = [false; TIMEOUT_REGISTRY_MAX],
    next_id: i32 = 1,

    static fn init(): TimeoutRegistry {
        return TimeoutRegistry{};
    }

    fn allocSlot(*_, kind: TimeoutKind, now_ms: u64, raw_interval_ms: i64): i32 ! TimeoutObjectError {
        let i: usize = 0;
        while (i < TIMEOUT_REGISTRY_MAX) {
            if (!_.occupied[i]) {
                const id = _.next_id;
                _.next_id +|= 1;
                _.slots[i] = TimeoutObject.init(id, kind, now_ms, raw_interval_ms);
                _.occupied[i] = true;
                return id;
            }
            i += 1;
        }
        return TimeoutObjectError.RegistryFull;
    }

    fn scheduleTimeout(*_, now_ms: u64, raw_interval_ms: i64): i32 ! TimeoutObjectError {
        return _.allocSlot(TimeoutKind.SetTimeout, now_ms, raw_interval_ms);
    }

    fn scheduleInterval(*_, now_ms: u64, raw_interval_ms: i64): i32 ! TimeoutObjectError {
        return _.allocSlot(TimeoutKind.SetInterval, now_ms, raw_interval_ms);
    }

    fn findIndex(id: i32): usize? {
        let i: usize = 0;
        while (i < TIMEOUT_REGISTRY_MAX) {
            if (_.occupied[i] && _.slots[i].id == id) return i;
            i += 1;
        }
        return null;
    }

    fn get(id: i32): TimeoutObject? {
        if (idx := _.findIndex(id)) return _.slots[idx];
        return null;
    }

    fn clear(*_, id: i32): bool {
        if (idx := _.findIndex(id)) {
            _.slots[idx].cancel();
            _.occupied[idx] = false;
            return true;
        }
        return false;
    }

    fn refresh(*_, id: i32, now_ms: u64): u64 ! TimeoutObjectError {
        if (idx := _.findIndex(id)) {
            return _.slots[idx].doRefresh(now_ms);
        }
        return TimeoutObjectError.NotFound;
    }

    fn setRef(*_, id: i32, keep_alive: bool): bool {
        if (idx := _.findIndex(id)) {
            if (keep_alive) _.slots[idx].doRef() else _.slots[idx].doUnref();
            return true;
        }
        return false;
    }

    /// Returns the number of active timers currently holding an event loop ref.
    fn activeRefCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < TIMEOUT_REGISTRY_MAX) {
            if (_.occupied[i] && _.slots[i].hasRef()) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// Fires all due timers at `now_ms`, returns the number of timers fired.
    fn tickDue(*_, now_ms: u64): usize {
        let fired: usize = 0;
        let i: usize = 0;
        while (i < TIMEOUT_REGISTRY_MAX) {
            if (_.occupied[i] && _.slots[i].fire(now_ms)) {
                fired += 1;
            }
            i += 1;
        }
        return fired;
    }
}
