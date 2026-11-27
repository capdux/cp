// @cap/ee-first — Pure CP zero-allocation first-event race listener across multiple emitters.
// Clean-room MIT implementation inspired by npm `ee-first` v1.

pub const FIRST_MAX_WATCHERS: usize = 16;

fn eefStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// A single `(emitter_id, event_name)` registration entry in a `FirstRace`.
pub struct EventWatch {
    pub emitter_id: u32,
    pub event_name: string,
    pub active: bool,
}

/// Outcome of an `emit` attempt or the winning event stored in `FirstRace`.
pub struct FirstResult {
    pub triggered: bool,
    pub emitter_id: u32,
    pub event_name: string,
    pub payload_code: i32,
    pub cancelled_count: usize,

    pub static fn empty(): FirstResult {
        return FirstResult {
            triggered: false,
            emitter_id: 0 as u32,
            event_name: "",
            payload_code: 0 as i32,
            cancelled_count: 0
        };
    }
}

/// Zero-heap 16-slot multi-emitter race listener.
/// The first matching event settles the race and automatically cancels all remaining watchers.
pub struct FirstRace {
    pub watchers: [16]EventWatch,
    pub count: usize,
    pub settled: bool,
    pub result: FirstResult,

    pub static fn new(): FirstRace {
        let empty_watch = EventWatch {
            emitter_id: 0 as u32,
            event_name: "",
            active: false
        };
        return FirstRace {
            watchers: [empty_watch; 16],
            count: 0,
            settled: false,
            result: FirstResult.empty()
        };
    }

    /// Registers a `(emitter_id, event_name)` watcher.
    /// Returns `false` if the race is already settled, capacity (16) is full, or `event_name` is empty.
    pub fn add(*_, emitter_id: u32, event_name: string): bool {
        if (_.settled || _.count >= FIRST_MAX_WATCHERS || event_name.len == 0) {
            return false;
        }
        _.watchers[_.count] = EventWatch {
            emitter_id: emitter_id,
            event_name: event_name,
            active: true
        };
        _.count = _.count + 1;
        return true;
    }

    /// Dispatches an event from `emitter_id`.
    /// If unsettled and matching an active watcher, settles the race, cancels all other active watchers,
    /// and returns a triggered `FirstResult`. Otherwise returns an untriggered result.
    pub fn emit(*_, emitter_id: u32, event_name: string, payload_code: i32): FirstResult {
        if (_.settled) {
            return FirstResult.empty();
        }
        let matched_idx: i64 = -1;
        let i: usize = 0;
        while (i < _.count) {
            let w = _.watchers[i];
            if (w.active && w.emitter_id == emitter_id && eefStrEq(w.event_name, event_name)) {
                matched_idx = i as i64;
                break;
            }
            i = i + 1;
        }
        if (matched_idx < 0) {
            return FirstResult.empty();
        }

        let cancelled: usize = 0;
        let j: usize = 0;
        while (j < _.count) {
            if (_.watchers[j].active) {
                _.watchers[j].active = false;
                if ((j as i64) != matched_idx) {
                    cancelled = cancelled + 1;
                }
            }
            j = j + 1;
        }

        _.settled = true;
        _.result = FirstResult {
            triggered: true,
            emitter_id: emitter_id,
            event_name: event_name,
            payload_code: payload_code,
            cancelled_count: cancelled
        };
        return _.result;
    }

    /// Manually cancels all remaining active watchers and settles the race.
    /// Returns the number of watchers cancelled.
    pub fn cancel(*_): usize {
        let cancelled: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.watchers[i].active) {
                _.watchers[i].active = false;
                cancelled = cancelled + 1;
            }
            i = i + 1;
        }
        _.settled = true;
        return cancelled;
    }

    /// Returns the number of currently active watchers.
    pub fn activeCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.watchers[i].active) {
                n = n + 1;
            }
            i = i + 1;
        }
        return n;
    }

    /// Returns `true` if a winning event has fired or `cancel()` was called.
    pub fn isSettled(): bool {
        return _.settled;
    }

    /// Returns the recorded winning `FirstResult` (untriggered if cancelled or still pending).
    pub fn winner(): FirstResult {
        return _.result;
    }
}

export {
    FIRST_MAX_WATCHERS,
    EventWatch,
    FirstResult,
    FirstRace
};
