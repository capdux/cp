// @cap/mitt — Zero-allocation tiny functional event emitter with wildcard "*" support
// Clean-room MIT implementation in pure CP modeled after npm `mitt` v3.

pub const MIT_MAX_LISTENERS: usize = 32;
pub const MIT_MAX_DISPATCH_LOG: usize = 32;

/// Single registered listener entry stored inline inside `Emitter`.
pub struct MittListener {
    active: bool,
    event_type: string,
    handler_id: u32,
    is_wildcard: bool,

    pub static fn empty(): MittListener {
        return MittListener{
            active: false,
            event_type: "",
            handler_id: 0,
            is_wildcard: false,
        };
    }
}

/// Dispatch log entry captured during `Emitter.emit`.
pub struct MittDispatch {
    handler_id: u32,
    payload: i64,
    is_wildcard: bool,

    pub static fn empty(): MittDispatch {
        return MittDispatch{
            handler_id: 0,
            payload: 0,
            is_wildcard: false,
        };
    }
}

fn mitStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn mitIsWildcard(event_type: string): bool {
    return event_type.len == 1 && event_type[0] == 42;
}

/// Fixed-capacity, zero-heap-allocation event emitter supporting named events
/// and `"*"` wildcard listeners (up to 32 active listener slots).
pub struct Emitter {
    listeners: [32]MittListener,
    count: usize,
    last_dispatched: [32]MittDispatch,
    last_dispatch_count: usize,
    emit_count: usize,

    /// Create an empty `Emitter` with zero active listeners.
    pub static fn new(): Emitter {
        let listeners: [32]MittListener = undefined;
        let last_dispatched: [32]MittDispatch = undefined;
        let i: usize = 0;
        while (i < MIT_MAX_LISTENERS) {
            listeners[i] = MittListener.empty();
            last_dispatched[i] = MittDispatch.empty();
            i += 1;
        }
        return Emitter{
            listeners: listeners,
            count: 0,
            last_dispatched: last_dispatched,
            last_dispatch_count: 0,
            emit_count: 0,
        };
    }

    /// Register a listener `handler_id` (must be non-zero) for `event_type` (or `"*"` wildcard).
    /// Returns `true` on success, or `false` if full or invalid arguments.
    pub fn on(*_, event_type: string, handler_id: u32): bool {
        if (event_type.len == 0 || handler_id == 0 || _.count >= MIT_MAX_LISTENERS) {
            return false;
        }
        const idx: usize = _.count;
        _.listeners[idx] = MittListener{
            active: true,
            event_type: event_type,
            handler_id: handler_id,
            is_wildcard: mitIsWildcard(event_type),
        };
        _.count += 1;
        return true;
    }

    /// Remove a listener for `event_type`.
    /// - If `handler_id != 0`, removes the first matching `(event_type, handler_id)` listener.
    /// - If `handler_id == 0`, removes all listeners registered for `event_type`.
    /// Returns `true` if at least one listener was removed.
    pub fn off(*_, event_type: string, handler_id: u32): bool {
        if (event_type.len == 0 || _.count == 0) {
            return false;
        }
        let removed: bool = false;
        let i: usize = 0;
        while (i < _.count) {
            const matches_type: bool = mitStrEq(_.listeners[i].event_type, event_type);
            const matches_handler: bool = (handler_id == 0) || (_.listeners[i].handler_id == handler_id);
            if (matches_type && matches_handler) {
                // Shift remaining listeners left to preserve registration order
                let j: usize = i;
                while (j + 1 < _.count) {
                    _.listeners[j] = _.listeners[j + 1];
                    j += 1;
                }
                _.count -= 1;
                _.listeners[_.count] = MittListener.empty();
                removed = true;
                if (handler_id != 0) {
                    return true;
                }
                // Re-check current index i since elements shifted left
                continue;
            }
            i += 1;
        }
        return removed;
    }

    /// Emit `event_type` with `payload`.
    /// Invokes exact-match listeners first (in registration order), then `"*"` wildcard listeners.
    /// Records every invocation into `last_dispatched[0..last_dispatch_count]` and increments `emit_count`.
    /// Returns the number of listener invocations triggered by this emit.
    pub fn emit(*_, event_type: string, payload: i64): usize {
        _.last_dispatch_count = 0;
        if (event_type.len == 0 || _.count == 0) {
            return 0;
        }

        let dispatched: usize = 0;
        const emitting_wildcard: bool = mitIsWildcard(event_type);

        // Pass 1: exact named listeners (skip if emitting "*" directly to avoid double dispatch)
        if (!emitting_wildcard) {
            let i: usize = 0;
            while (i < _.count) {
                const l: MittListener = _.listeners[i];
                if (l.active && !l.is_wildcard && mitStrEq(l.event_type, event_type)) {
                    if (dispatched < MIT_MAX_DISPATCH_LOG) {
                        _.last_dispatched[dispatched] = MittDispatch{
                            handler_id: l.handler_id,
                            payload: payload,
                            is_wildcard: false,
                        };
                    }
                    dispatched += 1;
                }
                i += 1;
            }
        }

        // Pass 2: wildcard "*" listeners
        let k: usize = 0;
        while (k < _.count) {
            const l: MittListener = _.listeners[k];
            if (l.active && l.is_wildcard) {
                if (dispatched < MIT_MAX_DISPATCH_LOG) {
                    _.last_dispatched[dispatched] = MittDispatch{
                        handler_id: l.handler_id,
                        payload: payload,
                        is_wildcard: true,
                    };
                }
                dispatched += 1;
            }
            k += 1;
        }

        _.last_dispatch_count = dispatched;
        _.emit_count += dispatched;
        return dispatched;
    }

    /// Count active listeners registered specifically for `event_type`.
    pub fn listenerCount(event_type: string): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.listeners[i].active && mitStrEq(_.listeners[i].event_type, event_type)) {
                total += 1;
            }
            i += 1;
        }
        return total;
    }

    /// Return total number of active listeners across all event types (including `"*"`).
    pub fn totalListeners(): usize {
        return _.count;
    }

    /// Remove all registered listeners.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.count) {
            _.listeners[i] = MittListener.empty();
            i += 1;
        }
        _.count = 0;
    }

    /// Check whether a listener for `event_type` is registered.
    /// If `handler_id == 0`, returns `true` if any listener exists for `event_type`.
    pub fn hasListener(event_type: string, handler_id: u32): bool {
        let i: usize = 0;
        while (i < _.count) {
            const l: MittListener = _.listeners[i];
            if (l.active && mitStrEq(l.event_type, event_type)) {
                if (handler_id == 0 || l.handler_id == handler_id) {
                    return true;
                }
            }
            i += 1;
        }
        return false;
    }
}

/// Functional factory helper (parity with `mitt()`).
pub fn mitt(): Emitter {
    return Emitter.new();
}

export {
    MIT_MAX_LISTENERS,
    MIT_MAX_DISPATCH_LOG,
    MittListener,
    MittDispatch,
    Emitter,
    mitt,
};
