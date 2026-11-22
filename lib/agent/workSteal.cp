// Origin: Pure-CP Stdlib (AI Agent Multi-Core Work-Stealing Scheduler & Load Balancer) | Created: 2026-11-22
// Zero-heap-allocation BEAM-inspired multi-core work-stealing deque pool:
//   - Local LIFO push/pop for CPU cache locality
//   - Remote FIFO head stealing from the busiest victim worker to minimize contention and balance load

/// Maximum number of worker cores supported by a fixed-layout `WorkStealPool`.
pub const MAX_WORK_STEAL_WORKERS: usize = 16;

/// Lightweight schedulable Agent task unit carrying BEAM-style reduction budget.
pub struct WorkTask {
    task_id: u64 = 0;
    agent_id: u32 = 0;
    reductions_budget: u32 = 2000;
}

/// Standalone fixed-capacity circular deque supporting local LIFO and remote FIFO stealing.
pub struct WorkerQueue {
    slots: []WorkTask;
    head: usize = 0;
    len: usize = 0;

    /// Initializes a single worker deque backed by caller-owned `slots`.
    static fn init(slots: *[]WorkTask): WorkerQueue {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = WorkTask{};
        }
        return WorkerQueue{
            slots: slots,
            head: 0,
            len: 0,
        };
    }

    /// Pushes `task` onto the local tail (LIFO end). Returns `false` if full or zero-capacity.
    fn pushLocal(*_, task: WorkTask): bool {
        if (_.slots.len == 0 || _.len >= _.slots.len) return false;
        const tail = (_.head + _.len) % _.slots.len;
        _.slots[tail] = task;
        _.len += 1;
        return true;
    }

    /// Pops the newest task from the local tail (LIFO) to preserve CPU cache locality.
    fn popLocal(*_): WorkTask? {
        if (_.slots.len == 0 || _.len == 0) return null;
        const tail = (_.head + _.len - 1) % _.slots.len;
        const task = _.slots[tail];
        _.slots[tail] = WorkTask{};
        _.len -= 1;
        return task;
    }

    /// Steals the oldest task from the remote head (FIFO) to minimize contention with local LIFO.
    fn stealFromHead(*_): WorkTask? {
        if (_.slots.len == 0 || _.len == 0) return null;
        const task = _.slots[_.head];
        _.slots[_.head] = WorkTask{};
        _.head = (_.head + 1) % _.slots.len;
        _.len -= 1;
        return task;
    }

    /// Returns the number of queued tasks.
    fn count(): usize {
        return _.len;
    }

    /// Returns the fixed capacity of the deque.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Returns `true` when no tasks are queued.
    fn isEmpty(): bool {
        return _.len == 0;
    }
}

/// Load-balancing and scheduling telemetry snapshot returned by `WorkStealPool.stats`.
pub struct WorkStealStats {
    total_pushed: u64 = 0;
    total_local_popped: u64 = 0;
    total_stolen: u64 = 0;
    failed_steals: u64 = 0;
    max_queue_len: usize = 0;
    min_queue_len: usize = 0;
}

/// Zero-heap-allocation multi-core work-stealing scheduler pool supporting up to 16 workers.
pub struct WorkStealPool {
    slots_flat: []WorkTask;
    num_workers: usize = 0;
    per_worker_cap: usize = 0;
    heads: [16]usize = [0; 16];
    lens: [16]usize = [0; 16];
    total_pushed: u64 = 0;
    total_local_popped: u64 = 0;
    total_stolen: u64 = 0;
    failed_steals: u64 = 0;

    /// Initializes a work-stealing pool over a caller-owned flat buffer `slots_flat` of size
    /// at least `num_workers * per_worker_cap`.
    /// Returns `null` if `num_workers == 0`, `num_workers > 16`, `per_worker_cap == 0`, or
    /// `slots_flat.len < num_workers * per_worker_cap`.
    static fn init(
        num_workers: usize,
        slots_flat: *[]WorkTask,
        per_worker_cap: usize,
    ): WorkStealPool? {
        if (num_workers == 0 || num_workers > MAX_WORK_STEAL_WORKERS) return null;
        if (per_worker_cap == 0) return null;
        const required = num_workers * per_worker_cap;
        if (slots_flat.len < required) return null;

        let i: usize = 0;
        while (i < required) : (i += 1) {
            slots_flat[i] = WorkTask{};
        }

        return WorkStealPool{
            slots_flat: slots_flat,
            num_workers: num_workers,
            per_worker_cap: per_worker_cap,
            heads: [0; 16],
            lens: [0; 16],
            total_pushed: 0,
            total_local_popped: 0,
            total_stolen: 0,
            failed_steals: 0,
        };
    }

    /// Submits `task` onto `worker_idx`'s local deque tail.
    /// Returns `false` if `worker_idx` is out of bounds or that worker's deque is full.
    fn submit(*_, worker_idx: usize, task: WorkTask): bool {
        if (worker_idx >= _.num_workers) return false;
        if (_.lens[worker_idx] >= _.per_worker_cap) return false;

        const base = worker_idx * _.per_worker_cap;
        const tail = (_.heads[worker_idx] + _.lens[worker_idx]) % _.per_worker_cap;
        _.slots_flat[base + tail] = task;
        _.lens[worker_idx] += 1;
        _.total_pushed += 1;
        return true;
    }

    /// Acquires the next task for `worker_idx`:
    /// 1. Pops from `worker_idx`'s local tail (LIFO) if non-empty.
    /// 2. Otherwise steals from the head (FIFO) of the busiest peer worker queue.
    /// 3. Returns `null` (and increments `failed_steals`) if all queues are empty.
    fn nextTask(*_, worker_idx: usize): WorkTask? {
        if (worker_idx >= _.num_workers) return null;

        // 1. Local LIFO pop
        if (_.lens[worker_idx] > 0) {
            const base = worker_idx * _.per_worker_cap;
            const tail = (_.heads[worker_idx] + _.lens[worker_idx] - 1) % _.per_worker_cap;
            const task = _.slots_flat[base + tail];
            _.slots_flat[base + tail] = WorkTask{};
            _.lens[worker_idx] -= 1;
            _.total_local_popped += 1;
            return task;
        }

        // 2. Steal FIFO from the busiest peer worker
        let victim: usize? = null;
        let max_len: usize = 0;
        let v: usize = 0;
        while (v < _.num_workers) : (v += 1) {
            if (v == worker_idx) continue;
            if (_.lens[v] > max_len) {
                max_len = _.lens[v];
                victim = v;
            }
        }

        if (v_idx := victim) {
            const base = v_idx * _.per_worker_cap;
            const head = _.heads[v_idx];
            const task = _.slots_flat[base + head];
            _.slots_flat[base + head] = WorkTask{};
            _.heads[v_idx] = (head + 1) % _.per_worker_cap;
            _.lens[v_idx] -= 1;
            _.total_stolen += 1;
            return task;
        }

        _.failed_steals += 1;
        return null;
    }

    /// Returns the current queue length of `worker_idx`, or `0` if out of bounds.
    fn queueLen(worker_idx: usize): usize {
        if (worker_idx >= _.num_workers) return 0;
        return _.lens[worker_idx];
    }

    /// Returns a load-balancing and scheduling telemetry snapshot across all workers.
    fn stats(): WorkStealStats {
        let max_q: usize = 0;
        let min_q: usize = 0;
        let i: usize = 0;
        while (i < _.num_workers) : (i += 1) {
            const q = _.lens[i];
            if (i == 0) {
                max_q = q;
                min_q = q;
            } else {
                if (q > max_q) max_q = q;
                if (q < min_q) min_q = q;
            }
        }

        return WorkStealStats{
            total_pushed: _.total_pushed,
            total_local_popped: _.total_local_popped,
            total_stolen: _.total_stolen,
            failed_steals: _.failed_steals,
            max_queue_len: max_q,
            min_queue_len: min_q,
        };
    }
}
