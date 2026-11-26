// packages/@cap/p-limit-core/src/index.cp
// Zero-allocation concurrency limiter core for CP.
// Clean-room MIT pure CP implementation of npm `p-limit` v6, reusing `@cap/p-queue`.

import {
    QueueTask,
    TaskQueue,
} from "../../p-queue/src/index.cp";

fn plimEmptyTask(): QueueTask {
    return QueueTask {
        id: 0,
        priority: 0,
        seq: 0,
    };
}

/// Zero-heap concurrency limiter built directly on `@cap/p-queue`'s `TaskQueue`.
pub struct PLimit {
    pub queue: TaskQueue,

    /// Creates a new `PLimit` with strict validation (`max_concurrency == 0` returns `null`).
    pub static fn new(max_concurrency: usize): PLimit? {
        if (max_concurrency == 0) {
            return null;
        }
        return PLimit {
            queue: TaskQueue.new(max_concurrency),
        };
    }

    /// Creates a single-concurrency `PLimit` (`max_concurrency = 1`), useful as an unwrap fallback.
    pub static fn single(): PLimit {
        return PLimit {
            queue: TaskQueue.new(1),
        };
    }

    /// Enqueues a task ID into the FIFO limiter queue. Returns `false` if the 64-slot queue is full.
    pub fn submit(*_, task_id: u32): bool {
        return _.queue.enqueue(task_id as u64, 0);
    }

    /// Starts the next pending task if `activeCount() < concurrency`, returning its task ID or `null`.
    pub fn startNext(*_): u32? {
        let task_opt: QueueTask? = _.queue.dispatchNext();
        if (task_opt == null) {
            return null;
        }
        let task: QueueTask = task_opt ?? plimEmptyTask();
        return task.id as u32;
    }

    /// Marks one active task as finished, freeing a concurrency slot. Returns `false` if none active.
    pub fn finishOne(*_): bool {
        return _.queue.markCompleted();
    }

    /// Number of currently running tasks.
    pub fn activeCount(): usize {
        return _.queue.activeCount();
    }

    /// Number of queued tasks waiting for a free concurrency slot.
    pub fn pendingCount(): usize {
        return _.queue.size();
    }

    /// Current maximum concurrency limit.
    pub fn concurrency(): usize {
        return _.queue.max_concurrency;
    }

    /// Clears all pending queued tasks (without affecting active tasks) and returns how many were dropped.
    pub fn clearQueue(*_): usize {
        let dropped: usize = _.queue.size();
        _.queue.clear();
        return dropped;
    }

    /// Dynamically updates the concurrency limit. Rejects `0` and returns `false`.
    pub fn setConcurrency(*_, new_limit: usize): bool {
        if (new_limit == 0) {
            return false;
        }
        _.queue.max_concurrency = new_limit;
        return true;
    }
}
