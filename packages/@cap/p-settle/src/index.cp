// @cap/p-settle — Zero-allocation bounded-concurrency allSettled task planner in pure CP
// Clean-room MIT implementation inspired by npm `p-settle` v5.
// Reuses @cap/p-limit (`PLimit`) for bounded concurrency without short-circuiting on errors.

import { PLimit } from "../../p-limit/src/index.cp";

pub const P_SETTLE_CAP: usize = 32;

pub enum SettleStatus {
    Pending,
    Fulfilled,
    Rejected,
}

pub struct SettledEntry {
    id: u64,
    status: SettleStatus,
    value: i64,
    err_code: i32,
}

fn psetFindSlot(ids: [32]u64, count: usize, id: u64): usize? {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (ids[i] == id) {
            return i;
        }
    }
    return null;
}

/// Zero-heap fixed-capacity (32 slots) bounded-concurrency `Promise.allSettled` planner (`p-settle` v5 parity).
/// Never short-circuits or cancels remaining tasks when individual tasks reject.
pub struct PSettlePlanner {
    limiter: PLimit,
    total_count: usize,
    fulfilled_count: usize,
    rejected_count: usize,
    ids: [32]u64,
    running: [32]bool,
    statuses: [32]SettleStatus,
    values: [32]i64,
    errors: [32]i32,

    /// Create a new `PSettlePlanner` with `max_concurrency >= 1`.
    /// Returns `null` if `max_concurrency == 0`.
    static fn new(max_concurrency: usize): PSettlePlanner? {
        if (max_concurrency == 0) {
            return null;
        }
        return PSettlePlanner {
            limiter: PLimit.new(max_concurrency),
            total_count: 0,
            fulfilled_count: 0,
            rejected_count: 0,
            ids: undefined,
            running: undefined,
            statuses: undefined,
            values: undefined,
            errors: undefined,
        };
    }

    /// Enqueue a unique task ID (up to `P_SETTLE_CAP` = 32 tasks).
    /// Returns `false` if full or duplicate ID.
    fn addTask(*_, id: u64): bool {
        if (_.total_count >= P_SETTLE_CAP) {
            return false;
        }
        if (psetFindSlot(_.ids, _.total_count, id) != null) {
            return false;
        }
        if (!_.limiter.enqueue(id)) {
            return false;
        }
        const idx: usize = _.total_count;
        _.ids[idx] = id;
        _.running[idx] = false;
        _.statuses[idx] = SettleStatus.Pending;
        _.values[idx] = 0;
        _.errors[idx] = 0;
        _.total_count += 1;
        return true;
    }

    /// Dispatch the next queued task via `PLimit.startNext()` if a concurrency slot is available.
    fn dispatchNext(*_): u64? {
        if (next_id := _.limiter.startNext()) {
            if (idx := psetFindSlot(_.ids, _.total_count, next_id)) {
                _.running[idx] = true;
            }
            return next_id;
        }
        return null;
    }

    /// Mark a currently running task as `Fulfilled` with `value` and release its concurrency slot.
    fn reportFulfilled(*_, id: u64, value: i64): bool {
        if (idx := psetFindSlot(_.ids, _.total_count, id)) {
            if (!_.running[idx] || _.statuses[idx] != SettleStatus.Pending) {
                return false;
            }
            _.running[idx] = false;
            _.statuses[idx] = SettleStatus.Fulfilled;
            _.values[idx] = value;
            _.fulfilled_count += 1;
            * = _.limiter.finishOne();
            return true;
        }
        return false;
    }

    /// Mark a currently running task as `Rejected` with `err_code` and release its concurrency slot.
    /// Never cancels or clears remaining queued tasks.
    fn reportRejected(*_, id: u64, err_code: i32): bool {
        if (idx := psetFindSlot(_.ids, _.total_count, id)) {
            if (!_.running[idx] || _.statuses[idx] != SettleStatus.Pending) {
                return false;
            }
            _.running[idx] = false;
            _.statuses[idx] = SettleStatus.Rejected;
            _.errors[idx] = err_code;
            _.rejected_count += 1;
            * = _.limiter.finishOne();
            return true;
        }
        return false;
    }

    /// Returns `true` when every added task is either `Fulfilled` or `Rejected`.
    fn isAllSettled(): bool {
        return (_.fulfilled_count + _.rejected_count) == _.total_count;
    }

    /// Number of tasks settled as `Fulfilled`.
    fn fulfilledCount(): usize {
        return _.fulfilled_count;
    }

    /// Number of tasks settled as `Rejected`.
    fn rejectedCount(): usize {
        return _.rejected_count;
    }

    /// Inspect the `SettledEntry` at registration index `idx` (`0..totalCount()`).
    fn entryAt(idx: usize): SettledEntry? {
        if (idx >= _.total_count) {
            return null;
        }
        return SettledEntry {
            id: _.ids[idx],
            status: _.statuses[idx],
            value: _.values[idx],
            err_code: _.errors[idx],
        };
    }

    /// Look up the `SettledEntry` by task `id`.
    fn entryOf(id: u64): SettledEntry? {
        if (idx := psetFindSlot(_.ids, _.total_count, id)) {
            return SettledEntry {
                id: _.ids[idx],
                status: _.statuses[idx],
                value: _.values[idx],
                err_code: _.errors[idx],
            };
        }
        return null;
    }

    /// Total number of registered tasks.
    fn totalCount(): usize {
        return _.total_count;
    }

    /// Number of currently active tasks in `PLimit`.
    fn activeCount(): usize {
        return _.limiter.activeCount();
    }

    /// Number of waiting tasks in `PLimit`.
    fn pendingCount(): usize {
        return _.limiter.pendingCount();
    }

    /// Configured concurrency limit.
    fn concurrency(): usize {
        return _.limiter.concurrency();
    }

    /// Maximum batch capacity (32).
    fn capacity(): usize {
        return P_SETTLE_CAP;
    }
}

export {
    P_SETTLE_CAP,
    SettleStatus,
    SettledEntry,
    PSettlePlanner,
};
