// @cap/fastq — Zero-allocation fast fixed-concurrency work queue in pure CP
// Clean-room MIT implementation of npm `fastq` (#253, 192M/wk, by Matteo Collina).

pub const FASTQ_CAP: usize = 64;

/// Zero-heap ring-buffer work queue supporting concurrency cap, push/unshift, pause/resume, and kill.
pub struct FastQueue {
    buf: [64]u64,
    head: usize,
    tail: usize,
    queued: usize,
    active: usize,
    concurrency: usize,
    paused: bool,

    pub static fn new(concurrency: usize): FastQueue {
        let c = concurrency;
        if (c == 0) { c = 1; }
        return FastQueue {
            buf: undefined,
            head: 0,
            tail: 0,
            queued: 0,
            active: 0,
            concurrency: c,
            paused: false,
        };
    }

    pub fn length(): usize {
        return _.queued;
    }

    pub fn running(): usize {
        return _.active;
    }

    pub fn idle(): bool {
        return _.queued == 0 && _.active == 0;
    }

    pub fn isPaused(): bool {
        return _.paused;
    }

    pub fn setConcurrency(*_, c: usize) {
        if (c == 0) {
            _.concurrency = 1;
        } else {
            _.concurrency = c;
        }
    }

    pub fn pause(*_) {
        _.paused = true;
    }

    pub fn unpause(*_) {
        _.paused = false;
    }

    /// Appends `task_id` to the back of the queue. Returns `false` if full.
    pub fn push(*_, task_id: u64): bool {
        if (_.queued >= FASTQ_CAP) {
            return false;
        }
        _.buf[_.tail] = task_id;
        _.tail = (_.tail + 1) % FASTQ_CAP;
        _.queued += 1;
        return true;
    }

    /// Prepends `task_id` to the front of the queue (high priority). Returns `false` if full.
    pub fn unshift(*_, task_id: u64): bool {
        if (_.queued >= FASTQ_CAP) {
            return false;
        }
        if (_.head == 0) {
            _.head = FASTQ_CAP - 1;
        } else {
            _.head -= 1;
        }
        _.buf[_.head] = task_id;
        _.queued += 1;
        return true;
    }

    /// Pops the next runnable task if not paused and `running() < concurrency`.
    pub fn nextTask(*_): u64? {
        if (_.paused || _.active >= _.concurrency || _.queued == 0) {
            return null;
        }
        const val = _.buf[_.head];
        _.head = (_.head + 1) % FASTQ_CAP;
        _.queued -= 1;
        _.active += 1;
        return val;
    }

    /// Marks one running task as finished, returning `true` if queue is now completely drained (`idle()`).
    pub fn completeTask(*_): bool {
        if (_.active > 0) {
            _.active -= 1;
        }
        return _.queued == 0 && _.active == 0;
    }

    /// Clears all pending queued tasks without touching currently running tasks. Returns dropped count.
    pub fn kill(*_): usize {
        const dropped = _.queued;
        _.head = 0;
        _.tail = 0;
        _.queued = 0;
        return dropped;
    }
}

export {
    FASTQ_CAP,
    FastQueue,
};
