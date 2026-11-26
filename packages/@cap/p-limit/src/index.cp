// @cap/p-limit — Zero-allocation concurrency limiter & FIFO slot scheduler in pure CP
// Clean-room MIT implementation inspired by npm `p-limit`.
// Reuses @cap/yocto-queue (`YoctoQueueU64`) for O(1) zero-heap FIFO pending task storage.

import {
    YOCTO_QUEUE_CAP,
    PopU64Result,
    YoctoQueueU64,
} from "../../yocto-queue/src/index.cp";

pub struct SubmitResult {
    pub started_immediately: bool,
    pub queued: bool,
    pub rejected: bool,
}

pub struct CompleteResult {
    pub has_next: bool,
    pub next_task_id: u64,
}

fn plClampConcurrency(c: usize): usize {
    if (c == 0) {
        return 1;
    }
    return c;
}

/// Zero-allocation concurrency slot limiter backed by `YoctoQueueU64`.
pub struct PLimit {
    limit: usize,
    active: usize,
    queue: YoctoQueueU64,

    /// Create a new concurrency limiter with `max_concurrency >= 1` (clamps `0` to `1`).
    pub static fn new(max_concurrency: usize): PLimit {
        return PLimit {
            limit: plClampConcurrency(max_concurrency),
            active: 0,
            queue: YoctoQueueU64.new(),
        };
    }

    /// Number of currently running (active) tasks.
    pub fn activeCount(): usize {
        return _.active;
    }

    /// Number of tasks waiting in the FIFO queue.
    pub fn pendingCount(): usize {
        return _.queue.size();
    }

    /// Maximum capacity of the underlying pending queue.
    pub fn queueCapacity(): usize {
        return YOCTO_QUEUE_CAP;
    }

    /// Current concurrency limit.
    pub fn concurrency(): usize {
        return _.limit;
    }

    /// Dynamically adjust concurrency limit (`new_limit >= 1`).
    /// Returns `false` if `new_limit == 0` (leaving limit unchanged).
    /// After scaling up, call `drainReady()` to pop newly runnable task IDs.
    pub fn setConcurrency(*_, new_limit: usize): bool {
        if (new_limit == 0) {
            return false;
        }
        _.limit = new_limit;
        return true;
    }

    /// Submit a task ID: starts immediately if an active slot is available,
    /// otherwise enqueues into `YoctoQueueU64` (or rejects if the queue is full).
    pub fn submit(*_, task_id: u64): SubmitResult {
        if (_.active < _.limit) {
            _.active += 1;
            return SubmitResult {
                started_immediately: true,
                queued: false,
                rejected: false,
            };
        }
        if (_.queue.enqueue(task_id)) {
            return SubmitResult {
                started_immediately: false,
                queued: true,
                rejected: false,
            };
        }
        return SubmitResult {
            started_immediately: false,
            queued: false,
            rejected: true,
        };
    }

    /// Mark one active task as completed. Releases its slot and automatically dequeues
    /// the next waiting task if `active < limit` and the queue is non-empty.
    pub fn complete(*_): CompleteResult {
        if (_.active > 0) {
            _.active -= 1;
        }
        if (_.active < _.limit && !_.queue.isEmpty()) {
            const pop: PopU64Result = _.queue.dequeue();
            if (pop.ok) {
                _.active += 1;
                return CompleteResult {
                    has_next: true,
                    next_task_id: pop.value,
                };
            }
        }
        return CompleteResult {
            has_next: false,
            next_task_id: 0,
        };
    }

    /// Clear all pending tasks from the queue without affecting currently active tasks.
    /// Returns the number of dropped pending tasks.
    pub fn clearQueue(*_): usize {
        const dropped: usize = _.queue.size();
        _.queue.clear();
        return dropped;
    }

    /// Pop up to `min(limit - active, pending, out.len)` queued tasks into `out`,
    /// transitioning each popped task into the active state.
    pub fn drainReady(*_, out: []u64): usize {
        let n: usize = 0;
        while (_.active < _.limit && n < out.len && !_.queue.isEmpty()) {
            const pop: PopU64Result = _.queue.dequeue();
            if (!pop.ok) {
                break;
            }
            out[n] = pop.value;
            _.active += 1;
            n += 1;
        }
        return n;
    }
}

export {
    SubmitResult,
    CompleteResult,
    PLimit,
};
