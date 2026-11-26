// @cap/p-all — Zero-allocation bounded-concurrency batch task planner in pure CP
// Clean-room MIT implementation inspired by npm `p-all` v5.
// Reuses @cap/p-limit (`PLimit`) for bounded concurrency & FIFO scheduling.

import { PLimit } from "../../p-limit/src/index.cp";

pub const P_ALL_CAP: usize = 32;

const PALL_STATE_QUEUED: u8 = 0;
const PALL_STATE_RUNNING: u8 = 1;
const PALL_STATE_SUCCESS: u8 = 2;
const PALL_STATE_ERROR: u8 = 3;
const PALL_STATE_CANCELLED: u8 = 4;

fn pallFindSlot(ids: [32]u64, count: usize, id: u64): usize? {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (ids[i] == id) {
            return i;
        }
    }
    return null;
}

/// Zero-heap fixed-capacity (32 slots) bounded-concurrency batch task planner (`p-all` v5 parity).
pub struct PAllPlanner {
    limiter: PLimit,
    stop_on_error: bool,
    failed: bool,
    first_err: i32,
    total_count: usize,
    completed_count: usize,
    error_count: usize,
    ids: [32]u64,
    states: [32]u8,
    values: [32]i64,
    errors: [32]i32,

    /// Create a new `PAllPlanner` with `max_concurrency >= 1` and `stop_on_error` policy.
    /// Returns `null` if `max_concurrency == 0`.
    static fn new(max_concurrency: usize, stop_on_error: bool): PAllPlanner? {
        if (max_concurrency == 0) {
            return null;
        }
        return PAllPlanner {
            limiter: PLimit.new(max_concurrency),
            stop_on_error: stop_on_error,
            failed: false,
            first_err: 0,
            total_count: 0,
            completed_count: 0,
            error_count: 0,
            ids: undefined,
            states: undefined,
            values: undefined,
            errors: undefined,
        };
    }

    /// Enqueue a unique task ID into the batch (up to `P_ALL_CAP` = 32 tasks).
    /// Returns `false` if full, duplicate ID, or already aborted under `stop_on_error`.
    fn addTask(*_, id: u64): bool {
        if (_.total_count >= P_ALL_CAP) {
            return false;
        }
        if (_.stop_on_error && _.failed) {
            return false;
        }
        if (pallFindSlot(_.ids, _.total_count, id) != null) {
            return false;
        }
        if (!_.limiter.enqueue(id)) {
            return false;
        }
        const idx: usize = _.total_count;
        _.ids[idx] = id;
        _.states[idx] = PALL_STATE_QUEUED;
        _.values[idx] = 0;
        _.errors[idx] = 0;
        _.total_count += 1;
        return true;
    }

    /// Dispatch the next queued task via `PLimit.startNext()` if concurrency permits
    /// and the planner has not aborted due to `stop_on_error`.
    fn dispatchNext(*_): u64? {
        if (_.stop_on_error && _.failed) {
            return null;
        }
        if (next_id := _.limiter.startNext()) {
            if (idx := pallFindSlot(_.ids, _.total_count, next_id)) {
                _.states[idx] = PALL_STATE_RUNNING;
            }
            return next_id;
        }
        return null;
    }

    /// Report successful completion for a currently running task ID.
    /// Releases one concurrency slot and records `value`.
    fn reportSuccess(*_, id: u64, value: i64): bool {
        if (idx := pallFindSlot(_.ids, _.total_count, id)) {
            if (_.states[idx] != PALL_STATE_RUNNING) {
                return false;
            }
            _.states[idx] = PALL_STATE_SUCCESS;
            _.values[idx] = value;
            _.completed_count += 1;
            * = _.limiter.finishOne();
            return true;
        }
        return false;
    }

    /// Report failure for a currently running task ID.
    /// Records the first error code; if `stop_on_error` is enabled, immediately sets
    /// `failed = true`, calls `limiter.clearQueue()`, and cancels remaining queued tasks.
    fn reportError(*_, id: u64, err_code: i32): bool {
        if (idx := pallFindSlot(_.ids, _.total_count, id)) {
            if (_.states[idx] != PALL_STATE_RUNNING) {
                return false;
            }
            _.states[idx] = PALL_STATE_ERROR;
            _.errors[idx] = err_code;
            _.error_count += 1;
            if (!_.failed) {
                _.failed = true;
                _.first_err = err_code;
            }
            * = _.limiter.finishOne();
            if (_.stop_on_error) {
                * = _.limiter.clearQueue();
                let i: usize = 0;
                while (i < _.total_count) : (i += 1) {
                    if (_.states[i] == PALL_STATE_QUEUED) {
                        _.states[i] = PALL_STATE_CANCELLED;
                    }
                }
            }
            return true;
        }
        return false;
    }

    /// Returns `true` when the batch has finished: either aborted immediately under
    /// `stop_on_error`, or all enqueued tasks have settled (succeeded or errored).
    fn isDone(): bool {
        if (_.stop_on_error && _.failed) {
            return true;
        }
        return (_.completed_count + _.error_count) == _.total_count;
    }

    /// Returns `true` if at least one task has reported an error.
    fn hasFailed(): bool {
        return _.failed;
    }

    /// Returns the error code of the first failed task (or `0` if none failed).
    fn firstErrorCode(): i32 {
        return _.first_err;
    }

    /// Returns the number of tasks that completed successfully.
    fn completedCount(): usize {
        return _.completed_count;
    }

    /// Look up the result value of a successfully completed task by `id`.
    /// Returns `null` if unknown, still pending/running, cancelled, or errored.
    fn resultOf(id: u64): i64? {
        if (idx := pallFindSlot(_.ids, _.total_count, id)) {
            if (_.states[idx] == PALL_STATE_SUCCESS) {
                return _.values[idx];
            }
        }
        return null;
    }

    /// Total number of tasks registered in this batch.
    fn totalCount(): usize {
        return _.total_count;
    }

    /// Number of tasks that reported an error.
    fn errorCount(): usize {
        return _.error_count;
    }

    /// Number of currently running tasks tracked by the underlying `PLimit`.
    fn activeCount(): usize {
        return _.limiter.activeCount();
    }

    /// Number of tasks still queued in the underlying `PLimit`.
    fn pendingCount(): usize {
        return _.limiter.pendingCount();
    }

    /// Configured concurrency limit.
    fn concurrency(): usize {
        return _.limiter.concurrency();
    }

    /// Maximum batch capacity (32).
    fn capacity(): usize {
        return P_ALL_CAP;
    }
}

export {
    P_ALL_CAP,
    PAllPlanner,
};
