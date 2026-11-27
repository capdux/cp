// @cap/p-queue — Zero-heap concurrency-limited priority task queue scheduler.
// Clean-room MIT port of Sindre Sorhus's `p-queue` v8 (75M+/week).

pub const PQ_MAX_TASKS: usize = 64;

pub struct QueueTask {
    pub id: u64,
    pub priority: i32, // higher priority runs first; ties broken by FIFO seq
    pub seq: u64,
}

/// Configuration options modeled on npm `p-queue` `Options`.
pub struct QueueOptions {
    pub concurrency: usize,
    pub auto_start: bool,
    pub interval_cap: usize,
    pub interval_ms: u64,

    pub static fn default(): QueueOptions {
        return QueueOptions {
            concurrency: 1,
            auto_start: true,
            interval_cap: 0,
            interval_ms: 0,
        };
    }
}

/// Zero-heap priority task queue with concurrency cap and pause/resume (`TaskQueue`).
pub struct TaskQueue {
    pub items: [64]QueueTask,
    pub count: usize,
    pub max_concurrency: usize,
    pub active: usize,
    pub paused: bool,
    pub next_seq: u64,
    pub completed_total: u64,

    pub static fn new(max_concurrency: usize): TaskQueue {
        let cap_conc = if (max_concurrency == 0) { 1 } else { max_concurrency };
        let empty = QueueTask { id: 0, priority: 0, seq: 0 };
        let arr: [64]QueueTask = undefined;
        let i: usize = 0;
        while (i < PQ_MAX_TASKS) { arr[i] = empty; i = i + 1; }
        return TaskQueue {
            items: arr,
            count: 0,
            max_concurrency: cap_conc,
            active: 0,
            paused: false,
            next_seq: 0,
            completed_total: 0,
        };
    }

    /// Creates a `TaskQueue` from `QueueOptions` (supports `auto_start: false`).
    pub static fn withOptions(opts: QueueOptions): TaskQueue {
        let q = TaskQueue.new(opts.concurrency);
        q.paused = !opts.auto_start;
        return q;
    }

    pub fn size(): usize { return _.count; }
    pub fn activeCount(): usize { return _.active; }
    /// npm `p-queue` `pending`: number of currently running (in-flight) tasks.
    pub fn pending(): usize { return _.active; }
    /// Current concurrency limit.
    pub fn concurrency(): usize { return _.max_concurrency; }
    /// Updates the concurrency limit (clamped to at least 1).
    pub fn setConcurrency(*_, max_concurrency: usize) {
        _.max_concurrency = if (max_concurrency == 0) { 1 } else { max_concurrency };
    }
    pub fn isPaused(): bool { return _.paused; }
    pub fn isIdle(): bool { return _.count == 0 && _.active == 0; }
    /// npm `p-queue` `onEmpty`: `true` when no tasks are waiting in the queue.
    pub fn onEmpty(): bool { return _.count == 0; }
    /// npm `p-queue` `onIdle`: `true` when queue is empty and no tasks are running.
    pub fn onIdle(): bool { return _.count == 0 && _.active == 0; }
    /// npm `p-queue` `onSizeLessThan`: `true` when queued task count is strictly `< limit`.
    pub fn onSizeLessThan(limit: usize): bool { return _.count < limit; }
    pub fn completedCount(): u64 { return _.completed_total; }
    /// Returns `true` if a queued task can be dispatched immediately.
    pub fn canDispatch(): bool {
        return !_.paused && _.count > 0 && _.active < _.max_concurrency;
    }

    pub fn pause(*_) { _.paused = true; }
    pub fn start(*_) { _.paused = false; }

    /// Enqueues a task `(id, priority)` maintaining descending priority + FIFO order.
    pub fn enqueue(*_, id: u64, priority: i32): bool {
        if (_.count >= PQ_MAX_TASKS) { return false; }
        let seq = _.next_seq;
        _.next_seq = _.next_seq + 1;
        let task = QueueTask { id: id, priority: priority, seq: seq };

        // Insert into sorted position (highest priority first, lowest seq first on tie)
        let pos = _.count;
        while (pos > 0) {
            let prev = _.items[pos - 1];
            let higher = (task.priority > prev.priority) ||
                (task.priority == prev.priority && task.seq < prev.seq);
            if (!higher) { break; }
            _.items[pos] = prev;
            pos = pos - 1;
        }
        _.items[pos] = task;
        _.count = _.count + 1;
        return true;
    }

    /// npm `p-queue` `add`: alias for `enqueue(id, priority)`.
    pub fn add(*_, id: u64, priority: i32): bool {
        return _.enqueue(id, priority);
    }

    /// npm `p-queue` `addAll`: enqueues a slice of task IDs with a shared priority.
    /// Returns the number of tasks successfully enqueued.
    pub fn addBatch(*_, ids: []const u64, priority: i32): usize {
        let added: usize = 0;
        let i: usize = 0;
        while (i < ids.len) : (i += 1) {
            if (!_.enqueue(ids[i], priority)) { break; }
            added += 1;
        }
        return added;
    }

    /// Peeks at the next highest-priority pending task without removing it.
    pub fn peek(): QueueTask? {
        if (_.count == 0) { return null; }
        return _.items[0];
    }

    /// Removes a queued task by `id`. Returns `true` if found and removed.
    pub fn remove(*_, id: u64): bool {
        let idx: usize = 0;
        let found = false;
        while (idx < _.count) : (idx += 1) {
            if (_.items[idx].id == id) {
                found = true;
                break;
            }
        }
        if (!found) { return false; }
        let i = idx;
        while (i + 1 < _.count) : (i += 1) {
            _.items[i] = _.items[i + 1];
        }
        _.count = _.count - 1;
        return true;
    }

    /// npm `p-queue` v8 `setPriority`: updates the priority of a queued task by `id` and re-sorts it.
    pub fn setPriority(*_, id: u64, new_priority: i32): bool {
        let idx: usize = 0;
        let found = false;
        while (idx < _.count) : (idx += 1) {
            if (_.items[idx].id == id) {
                found = true;
                break;
            }
        }
        if (!found) { return false; }
        let task = _.items[idx];
        task.priority = new_priority;

        // Remove from old slot
        let i = idx;
        while (i + 1 < _.count) : (i += 1) {
            _.items[i] = _.items[i + 1];
        }
        _.count = _.count - 1;

        // Re-insert preserving descending priority + FIFO seq tie-breaking
        let pos = _.count;
        while (pos > 0) {
            let prev = _.items[pos - 1];
            let higher = (task.priority > prev.priority) ||
                (task.priority == prev.priority && task.seq < prev.seq);
            if (!higher) { break; }
            _.items[pos] = prev;
            pos = pos - 1;
        }
        _.items[pos] = task;
        _.count = _.count + 1;
        return true;
    }

    /// Dispatches the highest-priority pending task if not paused and `active < max_concurrency`.
    pub fn dispatchNext(*_): QueueTask? {
        if (_.paused || _.count == 0 || _.active >= _.max_concurrency) {
            return null;
        }
        let head = _.items[0];
        let i: usize = 0;
        while (i + 1 < _.count) {
            _.items[i] = _.items[i + 1];
            i = i + 1;
        }
        _.count = _.count - 1;
        _.active = _.active + 1;
        return head;
    }

    /// Dispatches up to `out.len` tasks while concurrency slots are available.
    pub fn drainBatch(*_, out: []QueueTask): usize {
        let n: usize = 0;
        while (n < out.len) {
            const next = _.dispatchNext();
            if (next == null) { break; }
            out[n] = next.?;
            n += 1;
        }
        return n;
    }

    /// Marks one active task as completed, freeing a concurrency slot.
    pub fn markCompleted(*_): bool {
        if (_.active == 0) { return false; }
        _.active = _.active - 1;
        _.completed_total = _.completed_total + 1;
        return true;
    }

    /// Counts queued tasks with `priority >= min_priority`.
    pub fn sizeByPriority(min_priority: i32): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (_.items[i].priority >= min_priority) { n = n + 1; }
            i = i + 1;
        }
        return n;
    }

    /// Counts queued tasks whose priority equals `priority` exactly.
    pub fn sizeByExactPriority(priority: i32): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.items[i].priority == priority) { n += 1; }
        }
        return n;
    }

    /// Clears all pending queued tasks (does not affect currently active tasks).
    pub fn clear(*_) {
        _.count = 0;
    }

    /// Resets cumulative completion counter.
    pub fn resetStats(*_) {
        _.completed_total = 0;
    }
}
