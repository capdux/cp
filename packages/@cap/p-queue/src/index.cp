// @cap/p-queue — Zero-heap concurrency-limited priority task queue scheduler.
// Clean-room MIT port of Sindre Sorhus's `p-queue` v8 (75M+/week).

pub const PQ_MAX_TASKS: usize = 64;

pub struct QueueTask {
    pub id: u64,
    pub priority: i32, // higher priority runs first; ties broken by FIFO seq
    pub seq: u64,
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

    pub fn size(): usize { return _.count; }
    pub fn activeCount(): usize { return _.active; }
    pub fn isPaused(): bool { return _.paused; }
    pub fn isIdle(): bool { return _.count == 0 && _.active == 0; }
    pub fn completedCount(): u64 { return _.completed_total; }

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

    /// Clears all pending queued tasks (does not affect currently active tasks).
    pub fn clear(*_) {
        _.count = 0;
    }
}
