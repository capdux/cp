// Source: Absorbed from /data/dev/bun/src/runtime/timer/ImmediateObject.cp | Synced: 2026-11-22T20:05:16Z (Node/Bun setImmediate & clearImmediate ImmediateObject lifecycle, ref/unref/dispose state machine & fixed-slot FIFO queue)
// Pure-CP zero-allocation ImmediateObject (setImmediate/clearImmediate) lifecycle & turn-bounded FIFO queue.

/// Maximum concurrent scheduled immediates in `ImmediateQueue`.
pub const IMMEDIATE_QUEUE_MAX: usize = 64;

/// Lifecycle state of an `ImmediateObject`.
pub enum ImmediateState: u8 {
    Active = 0,
    Fired = 1,
    Cancelled = 2,
    Destroyed = 3,
}

/// Error set for `ImmediateObject` and `ImmediateQueue`.
pub err ImmediateObjectError {
    QueueFull,
    NotFound,
    AlreadyDestroyed,
}

/// Zero-allocation Node/Bun `ImmediateObject` state machine (aligned with Bun `ImmediateObject.cp`).
pub struct ImmediateObject {
    id: i32 = 0,
    turn: u64 = 0,
    state: ImmediateState = ImmediateState.Destroyed,
    has_ref: bool = false,
    ref_count: u32 = 1,

    /// Initializes a new active `ImmediateObject` assigned to scheduling `turn`.
    static fn init(id: i32, turn: u64): ImmediateObject {
        return ImmediateObject{
            id: id,
            turn: turn,
            state: ImmediateState.Active,
            has_ref: true,
            ref_count: 1,
        };
    }

    /// Returns the numeric immediate ID (aligned with `ImmediateObject.toPrimitive`).
    fn toPrimitive(): i32 {
        return _.id;
    }

    /// Keeps the event loop alive while this immediate is pending (aligned with `doRef`).
    fn doRef(*_) {
        if (_.state == ImmediateState.Active) {
            _.has_ref = true;
        }
    }

    /// Allows the event loop to exit if this is the only pending immediate (aligned with `doUnref`).
    fn doUnref(*_) {
        _.has_ref = false;
    }

    /// Returns whether this immediate currently holds an active event loop ref (aligned with `hasRef`).
    fn hasRef(): bool {
        return _.state == ImmediateState.Active && _.has_ref;
    }

    /// Increments internal reference count.
    fn ref(*_) {
        _.ref_count +|= 1;
    }

    /// Decrements internal reference count and returns true when it hits zero.
    fn deref(*_): bool {
        if (_.ref_count > 0) {
            _.ref_count -= 1;
        }
        return _.ref_count == 0;
    }

    /// Executes the immediate once if active (aligned with `runImmediateTask`).
    /// Returns true if fired, false if already fired, cancelled, or destroyed.
    fn runImmediateTask(*_): bool {
        if (_.state != ImmediateState.Active) return false;
        _.state = ImmediateState.Fired;
        _.has_ref = false;
        return true;
    }

    /// Cancels this immediate (aligned with `ImmediateObject.dispose` / `clearImmediate`).
    fn dispose(*_) {
        if (_.state == ImmediateState.Active) {
            _.state = ImmediateState.Cancelled;
            _.has_ref = false;
        }
    }

    /// Marks this immediate as destroyed during teardown (aligned with `ImmediateObject.finalize`).
    fn finalize(*_) {
        _.state = ImmediateState.Destroyed;
        _.has_ref = false;
    }

    /// Returns true if this immediate is no longer active (aligned with `getDestroyed`).
    fn getDestroyed(): bool {
        return _.state != ImmediateState.Active;
    }

    fn isActive(): bool {
        return _.state == ImmediateState.Active;
    }
}

/// Fixed-slot zero-allocation FIFO `setImmediate` queue with turn-bounded draining.
pub struct ImmediateQueue {
    slots: [IMMEDIATE_QUEUE_MAX]ImmediateObject = [ImmediateObject{}; IMMEDIATE_QUEUE_MAX],
    occupied: [IMMEDIATE_QUEUE_MAX]bool = [false; IMMEDIATE_QUEUE_MAX],
    next_id: i32 = 1,
    current_turn: u64 = 0,
    total_fired: u64 = 0,

    static fn init(): ImmediateQueue {
        return ImmediateQueue{};
    }

    /// Schedules a new immediate for the current turn and returns its positive ID.
    fn schedule(*_): i32 ! ImmediateObjectError {
        let i: usize = 0;
        while (i < IMMEDIATE_QUEUE_MAX) {
            if (!_.occupied[i]) {
                const id = _.next_id;
                _.next_id +|= 1;
                if (_.next_id <= 0) _.next_id = 1;
                _.slots[i] = ImmediateObject.init(id, _.current_turn);
                _.occupied[i] = true;
                return id;
            }
            i += 1;
        }
        return ImmediateObjectError.QueueFull;
    }

    /// Looks up a currently occupied immediate by ID.
    fn get(id: i32): ImmediateObject ! ImmediateObjectError {
        let i: usize = 0;
        while (i < IMMEDIATE_QUEUE_MAX) {
            if (_.occupied[i] && _.slots[i].id == id) {
                return _.slots[i];
            }
            i += 1;
        }
        return ImmediateObjectError.NotFound;
    }

    /// Cancels and releases an immediate by ID (`clearImmediate`).
    fn clear(*_, id: i32): bool {
        let i: usize = 0;
        while (i < IMMEDIATE_QUEUE_MAX) {
            if (_.occupied[i] && _.slots[i].id == id) {
                _.slots[i].dispose();
                _.occupied[i] = false;
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Updates `ref`/`unref` status for an active immediate by ID.
    fn setRef(*_, id: i32, keep_alive: bool): bool ! ImmediateObjectError {
        let i: usize = 0;
        while (i < IMMEDIATE_QUEUE_MAX) {
            if (_.occupied[i] && _.slots[i].id == id) {
                if (_.slots[i].state == ImmediateState.Destroyed) {
                    return ImmediateObjectError.AlreadyDestroyed;
                }
                if (keep_alive) {
                    _.slots[i].doRef();
                } else {
                    _.slots[i].doUnref();
                }
                return _.slots[i].hasRef();
            }
            i += 1;
        }
        return ImmediateObjectError.NotFound;
    }

    /// Returns total active immediates currently queued.
    fn activeCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < IMMEDIATE_QUEUE_MAX) {
            if (_.occupied[i] && _.slots[i].isActive()) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// Returns number of active immediates currently keeping the event loop alive.
    fn activeRefCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < IMMEDIATE_QUEUE_MAX) {
            if (_.occupied[i] && _.slots[i].hasRef()) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// Drains all active immediates scheduled up to the current turn into `out_ids`
    /// and advances `current_turn`, preventing recursive `setImmediate` starvation.
    fn drainTurn(*_, out_ids: []i32): usize {
        const snapshot_turn = _.current_turn;
        _.current_turn +|= 1;

        let fired: usize = 0;
        let i: usize = 0;
        while (i < IMMEDIATE_QUEUE_MAX) {
            if (_.occupied[i] && _.slots[i].turn <= snapshot_turn) {
                if (_.slots[i].runImmediateTask()) {
                    if (fired < out_ids.len) {
                        out_ids[fired] = _.slots[i].id;
                    }
                    fired += 1;
                    _.total_fired +|= 1;
                }
                _.occupied[i] = false;
            }
            i += 1;
        }
        return fired;
    }
}
