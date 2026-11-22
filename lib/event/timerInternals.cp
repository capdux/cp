// Source: Absorbed from /data/dev/bun/src/runtime/timer/TimerObjectInternals.cp | Synced: 2026-11-22T20:08:50Z (Shared TimerObjectInternals state machine: 32-bit bitfield TimerObjectFlags, asyncID encoding, generation isolation guard & reentrant callback lifecycle)
// Pure-CP zero-allocation shared internals for TimeoutObject and ImmediateObject aligned with Bun `TimerObjectInternals.cp`.

/// Maximum 25-bit epoch value stored in `TimerObjectFlags` (`(1 << 25) - 1`).
pub const TIMER_INTERNALS_MAX_EPOCH: u32 = 33_554_431;

/// Maximum 31-bit millisecond interval (`(1 << 31) - 1`).
pub const TIMER_INTERNALS_MAX_INTERVAL_MS: u32 = 2_147_483_647;

/// Kind of timer backed by `TimerObjectInternals`.
pub enum TimerInternalsKind: u8 {
    SetTimeout = 0,
    SetInterval = 1,
    SetImmediate = 2,
}

/// Execution state of the underlying event loop timer.
pub enum TimerInternalsState: u8 {
    Active = 0,
    Fired = 1,
    Cancelled = 2,
    Destroyed = 3,
}

/// Compact 32-bit flag set aligned with Bun `@packed(u32) TimerObjectFlags`:
/// - bits 0..24:  epoch (u25)
/// - bits 25..26: kind (u2: 0=SetTimeout, 1=SetInterval, 2=SetImmediate)
/// - bit 27:      has_cleared_timer
/// - bit 28:      is_keeping_event_loop_alive
/// - bit 29:      has_accessed_primitive
/// - bit 30:      has_js_ref
/// - bit 31:      in_callback
pub struct TimerObjectFlags {
    epoch: u32 = 0,
    kind: TimerInternalsKind = TimerInternalsKind.SetTimeout,
    has_cleared_timer: bool = false,
    is_keeping_event_loop_alive: bool = false,
    has_accessed_primitive: bool = false,
    has_js_ref: bool = true,
    in_callback: bool = false,

    /// Packs all flags into a 32-bit unsigned integer matching Bun's packed bit layout.
    fn toU32(): u32 {
        let bits: u32 = _.epoch & TIMER_INTERNALS_MAX_EPOCH;
        const kind_bits: u32 = switch (_.kind) {
            .SetTimeout => 0,
            .SetInterval => 1,
            .SetImmediate => 2,
        };
        bits |= (kind_bits & 0x3) << 25;
        if (_.has_cleared_timer) bits |= @as(u32, 1) << 27;
        if (_.is_keeping_event_loop_alive) bits |= @as(u32, 1) << 28;
        if (_.has_accessed_primitive) bits |= @as(u32, 1) << 29;
        if (_.has_js_ref) bits |= @as(u32, 1) << 30;
        if (_.in_callback) bits |= @as(u32, 1) << 31;
        return bits;
    }

    /// Unpacks a 32-bit bitfield into `TimerObjectFlags`.
    static fn fromU32(bits: u32): TimerObjectFlags {
        const kind_raw: u32 = (bits >> 25) & 0x3;
        const kind = if (kind_raw == 1)
            TimerInternalsKind.SetInterval
        else if (kind_raw == 2)
            TimerInternalsKind.SetImmediate
        else
            TimerInternalsKind.SetTimeout;
        return TimerObjectFlags{
            epoch: bits & TIMER_INTERNALS_MAX_EPOCH,
            kind: kind,
            has_cleared_timer: ((bits >> 27) & 1) != 0,
            is_keeping_event_loop_alive: ((bits >> 28) & 1) != 0,
            has_accessed_primitive: ((bits >> 29) & 1) != 0,
            has_js_ref: ((bits >> 30) & 1) != 0,
            in_callback: ((bits >> 31) & 1) != 0,
        };
    }
}

/// Shared zero-allocation timer internals for `setTimeout`, `setInterval`, and `setImmediate`.
pub struct TimerObjectInternals {
    id: i32 = -1,
    interval_ms: u32 = 0,
    generation: u32 = 0,
    state: TimerInternalsState = TimerInternalsState.Cancelled,
    flags: TimerObjectFlags = TimerObjectFlags{},
    fire_count: u64 = 0,

    /// Initializes active timer internals with `id`, `kind`, `interval_ms`, `epoch`, and `generation`.
    static fn init(
        id: i32,
        kind: TimerInternalsKind,
        interval_ms: u32,
        epoch: u32,
        generation: u32,
    ): TimerObjectInternals {
        const clamped_interval = if (interval_ms > TIMER_INTERNALS_MAX_INTERVAL_MS)
            TIMER_INTERNALS_MAX_INTERVAL_MS
        else
            interval_ms;
        return TimerObjectInternals{
            id: id,
            interval_ms: clamped_interval,
            generation: generation,
            state: TimerInternalsState.Active,
            flags: TimerObjectFlags{
                epoch: epoch & TIMER_INTERNALS_MAX_EPOCH,
                kind: kind,
                has_cleared_timer: false,
                is_keeping_event_loop_alive: true,
                has_accessed_primitive: false,
                has_js_ref: true,
                in_callback: false,
            },
            fire_count: 0,
        };
    }

    /// Encodes a deterministic 64-bit async inspector ID from `(kind, id)` (aligned with `TimerObjectInternals.asyncID`).
    fn asyncID(): u64 {
        const kind_tag: u64 = switch (_.flags.kind) {
            .SetTimeout => 0,
            .SetInterval => 1,
            .SetImmediate => 2,
        };
        const id_bits: u64 = @as(u64, @as(u32, @bitCast(_.id)));
        return (kind_tag << 32) | id_bits;
    }

    /// Returns the numeric timer ID and marks `has_accessed_primitive` (aligned with `toPrimitive`).
    fn toPrimitive(*_): i32 {
        _.flags.has_accessed_primitive = true;
        return _.id;
    }

    /// Marks the timer as keeping the event loop alive when active (aligned with `doRef`).
    fn doRef(*_) {
        if (_.state == TimerInternalsState.Active && !_.flags.has_cleared_timer) {
            _.flags.is_keeping_event_loop_alive = true;
        }
    }

    /// Allows the event loop to exit without waiting for this timer (aligned with `doUnref`).
    fn doUnref(*_) {
        _.flags.is_keeping_event_loop_alive = false;
    }

    /// Returns true if the timer currently keeps the event loop alive (aligned with `hasRef`).
    fn hasRef(): bool {
        return _.state == TimerInternalsState.Active &&
            !_.flags.has_cleared_timer &&
            _.flags.is_keeping_event_loop_alive;
    }

    /// Checks whether this timer is eligible to execute under `current_generation` and loop liveness.
    /// Aligned with Bun `TimerObjectInternals.runImmediateTask` & `fire` guard conditions.
    fn shouldFire(current_generation: u32, loop_alive_excluding_immediates: bool): bool {
        if (_.state != TimerInternalsState.Active) return false;
        if (_.flags.has_cleared_timer) return false;
        if (_.generation != current_generation) return false;
        if (_.flags.kind == TimerInternalsKind.SetImmediate &&
            !_.flags.is_keeping_event_loop_alive &&
            !loop_alive_excluding_immediates)
        {
            return false;
        }
        return true;
    }

    /// Transitions into callback execution if eligible. Returns true if callback should run.
    fn beginFire(*_, current_generation: u32, loop_alive_excluding_immediates: bool): bool {
        if (!_.shouldFire(current_generation, loop_alive_excluding_immediates)) {
            _.flags.is_keeping_event_loop_alive = false;
            _.flags.has_js_ref = false;
            return false;
        }
        _.state = TimerInternalsState.Fired;
        _.flags.in_callback = true;
        _.fire_count +|= 1;
        if (_.flags.kind == TimerInternalsKind.SetImmediate) {
            _.flags.is_keeping_event_loop_alive = false;
            _.flags.has_js_ref = false;
        }
        return true;
    }

    /// Finishes callback execution; repeating intervals not cleared during callback automatically re-arm.
    fn endFire(*_) {
        if (!_.flags.in_callback) return;
        _.flags.in_callback = false;
        if (_.flags.kind == TimerInternalsKind.SetInterval &&
            !_.flags.has_cleared_timer &&
            _.state != TimerInternalsState.Cancelled &&
            _.state != TimerInternalsState.Destroyed)
        {
            _.state = TimerInternalsState.Active;
        } else if (_.state == TimerInternalsState.Fired) {
            _.flags.is_keeping_event_loop_alive = false;
            _.flags.has_js_ref = false;
        }
    }

    /// Re-arms a timer (aligned with `refresh` / `reschedule`) unless destroyed.
    fn reschedule(*_, new_epoch: u32): bool {
        if (_.state == TimerInternalsState.Destroyed) return false;
        _.flags.epoch = new_epoch & TIMER_INTERNALS_MAX_EPOCH;
        _.flags.has_cleared_timer = false;
        _.flags.has_js_ref = true;
        _.state = TimerInternalsState.Active;
        return true;
    }

    /// Cancels the timer and releases event loop ref (aligned with `TimerObjectInternals.cancel`).
    fn cancel(*_) {
        _.flags.has_cleared_timer = true;
        _.flags.is_keeping_event_loop_alive = false;
        _.flags.has_js_ref = false;
        if (_.state != TimerInternalsState.Destroyed) {
            _.state = TimerInternalsState.Cancelled;
        }
    }

    /// Destroys the timer during finalization (aligned with `TimerObjectInternals.finalize`).
    fn finalize(*_) {
        _.flags.has_cleared_timer = true;
        _.flags.is_keeping_event_loop_alive = false;
        _.flags.has_js_ref = false;
        _.flags.in_callback = false;
        _.state = TimerInternalsState.Destroyed;
    }

    /// Returns true if this timer is cleared, fired, cancelled, or destroyed (aligned with `getDestroyed`).
    fn getDestroyed(): bool {
        return _.flags.has_cleared_timer || _.state != TimerInternalsState.Active;
    }
}
