// Source: cp/lib/queue/scheduler.cp (Priority, Delayed, Lease-Timeout & Dead-Letter Job Queue)
// Deterministic in-memory priority + delayed + lease-timeout + DLQ job queue engine
// backed by binary min-heaps for O(log N) scheduling of AI Agent background workloads.

const std = @import("std");
import {
    QueueError,
    JobPriority,
    JobState,
    RetryPolicy,
    Job,
} from "./job.cp";

/// Snapshot of current queue occupancy and cumulative scheduling counters.
pub struct QueueStats {
    pub pending: usize = 0;
    pub delayed: usize = 0;
    pub running: usize = 0;
    pub completed: usize = 0;
    pub dead_letter: usize = 0;
    pub total_enqueued: u64 = 0;
    pub total_retried: u64 = 0;
}

/// Internal lease tracking entry stored in the running lease min-heap.
struct LeaseEntry {
    idx: usize = 0;
    lease_until_ms: i64 = 0;
}

/// Binary min-heap of job indices ordered by `(priority DESC, id ASC)` for O(log N) ready dispatch.
struct ReadyHeap {
    items: std.ArrayList(usize) = .empty,

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.items.deinit(alloc);
    }

    static fn lessThan(jobs: []const Job, a_idx: usize, b_idx: usize): bool {
        const a = jobs[a_idx];
        const b = jobs[b_idx];
        const ap = a.priority as u8;
        const bp = b.priority as u8;
        if (ap != bp) {
            return ap > bp;
        }
        return a.id < b.id;
    }

    fn push(*_, alloc: std.mem.Allocator, jobs: []const Job, idx: usize): ! {
        _.items.append(alloc, idx) catch return QueueError.OutOfMemory;
        let child: usize = _.items.items.len - 1;
        while (child > 0) {
            const parent = (child - 1) / 2;
            if (!ReadyHeap.lessThan(jobs, _.items.items[child], _.items.items[parent])) {
                break;
            }
            const tmp = _.items.items[parent];
            _.items.items[parent] = _.items.items[child];
            _.items.items[child] = tmp;
            child = parent;
        }
    }

    fn pop(*_, jobs: []const Job): usize? {
        const len = _.items.items.len;
        if (len == 0) {
            return null;
        }
        const top = _.items.items[0];
        const last = _.items.items[len - 1];
        _.items.items.len = len - 1;
        if (_.items.items.len > 0) {
            _.items.items[0] = last;
            let curr: usize = 0;
            const n = _.items.items.len;
            while (true) {
                const left = curr * 2 + 1;
                if (left >= n) {
                    break;
                }
                let best = curr;
                if (ReadyHeap.lessThan(jobs, _.items.items[left], _.items.items[best])) {
                    best = left;
                }
                const right = left + 1;
                if (right < n && ReadyHeap.lessThan(jobs, _.items.items[right], _.items.items[best])) {
                    best = right;
                }
                if (best == curr) {
                    break;
                }
                const tmp = _.items.items[curr];
                _.items.items[curr] = _.items.items[best];
                _.items.items[best] = tmp;
                curr = best;
            }
        }
        return top;
    }
}

/// Binary min-heap of delayed job indices ordered by `(run_at_ms ASC, id ASC)`.
struct DelayedHeap {
    items: std.ArrayList(usize) = .empty,

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.items.deinit(alloc);
    }

    static fn lessThan(jobs: []const Job, a_idx: usize, b_idx: usize): bool {
        const a = jobs[a_idx];
        const b = jobs[b_idx];
        if (a.run_at_ms != b.run_at_ms) {
            return a.run_at_ms < b.run_at_ms;
        }
        return a.id < b.id;
    }

    fn push(*_, alloc: std.mem.Allocator, jobs: []const Job, idx: usize): ! {
        _.items.append(alloc, idx) catch return QueueError.OutOfMemory;
        let child: usize = _.items.items.len - 1;
        while (child > 0) {
            const parent = (child - 1) / 2;
            if (!DelayedHeap.lessThan(jobs, _.items.items[child], _.items.items[parent])) {
                break;
            }
            const tmp = _.items.items[parent];
            _.items.items[parent] = _.items.items[child];
            _.items.items[child] = tmp;
            child = parent;
        }
    }

    fn peek(): usize? {
        if (_.items.items.len == 0) {
            return null;
        }
        return _.items.items[0];
    }

    fn pop(*_, jobs: []const Job): usize? {
        const len = _.items.items.len;
        if (len == 0) {
            return null;
        }
        const top = _.items.items[0];
        const last = _.items.items[len - 1];
        _.items.items.len = len - 1;
        if (_.items.items.len > 0) {
            _.items.items[0] = last;
            let curr: usize = 0;
            const n = _.items.items.len;
            while (true) {
                const left = curr * 2 + 1;
                if (left >= n) {
                    break;
                }
                let best = curr;
                if (DelayedHeap.lessThan(jobs, _.items.items[left], _.items.items[best])) {
                    best = left;
                }
                const right = left + 1;
                if (right < n && DelayedHeap.lessThan(jobs, _.items.items[right], _.items.items[best])) {
                    best = right;
                }
                if (best == curr) {
                    break;
                }
                const tmp = _.items.items[curr];
                _.items.items[curr] = _.items.items[best];
                _.items.items[best] = tmp;
                curr = best;
            }
        }
        return top;
    }
}

/// Binary min-heap of active leases ordered by `(lease_until_ms ASC, idx ASC)`.
struct LeaseHeap {
    items: std.ArrayList(LeaseEntry) = .empty,

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.items.deinit(alloc);
    }

    static fn lessThan(a: LeaseEntry, b: LeaseEntry): bool {
        if (a.lease_until_ms != b.lease_until_ms) {
            return a.lease_until_ms < b.lease_until_ms;
        }
        return a.idx < b.idx;
    }

    fn push(*_, alloc: std.mem.Allocator, entry: LeaseEntry): ! {
        _.items.append(alloc, entry) catch return QueueError.OutOfMemory;
        let child: usize = _.items.items.len - 1;
        while (child > 0) {
            const parent = (child - 1) / 2;
            if (!LeaseHeap.lessThan(_.items.items[child], _.items.items[parent])) {
                break;
            }
            const tmp = _.items.items[parent];
            _.items.items[parent] = _.items.items[child];
            _.items.items[child] = tmp;
            child = parent;
        }
    }

    fn peek(): LeaseEntry? {
        if (_.items.items.len == 0) {
            return null;
        }
        return _.items.items[0];
    }

    fn pop(*_): LeaseEntry? {
        const len = _.items.items.len;
        if (len == 0) {
            return null;
        }
        const top = _.items.items[0];
        const last = _.items.items[len - 1];
        _.items.items.len = len - 1;
        if (_.items.items.len > 0) {
            _.items.items[0] = last;
            let curr: usize = 0;
            const n = _.items.items.len;
            while (true) {
                const left = curr * 2 + 1;
                if (left >= n) {
                    break;
                }
                let best = curr;
                if (LeaseHeap.lessThan(_.items.items[left], _.items.items[best])) {
                    best = left;
                }
                const right = left + 1;
                if (right < n && LeaseHeap.lessThan(_.items.items[right], _.items.items[best])) {
                    best = right;
                }
                if (best == curr) {
                    break;
                }
                const tmp = _.items.items[curr];
                _.items.items[curr] = _.items.items[best];
                _.items.items[best] = tmp;
                curr = best;
            }
        }
        return top;
    }
}

/// Deterministic priority, delayed, lease-timeout, and dead-letter background job queue.
pub struct JobQueue {
    allocator: std.mem.Allocator,
    default_retry: RetryPolicy = RetryPolicy.default(),
    lease_timeout_ms: i64 = 30000,
    max_active_jobs: usize = 0,
    next_id: u64 = 1,
    jobs: std.ArrayList(Job) = .empty,
    ready_heap: ReadyHeap = ReadyHeap{},
    delayed_heap: DelayedHeap = DelayedHeap{},
    lease_heap: LeaseHeap = LeaseHeap{},
    dead_letter_indices: std.ArrayList(usize) = .empty,
    pending_count: usize = 0,
    delayed_count: usize = 0,
    running_count: usize = 0,
    completed_count: usize = 0,
    dead_letter_count: usize = 0,
    total_enqueued_count: u64 = 0,
    total_retried_count: u64 = 0,

    /// Initializes a `JobQueue` with `default_retry` policy and worker `lease_timeout_ms`.
    pub static fn init(
        alloc: std.mem.Allocator,
        default_retry: RetryPolicy,
        lease_timeout_ms: i64,
    ): JobQueue! {
        return JobQueue{
            allocator: alloc,
            default_retry: default_retry,
            lease_timeout_ms: lease_timeout_ms,
            max_active_jobs: 0,
            next_id: 1,
            jobs: .empty,
            ready_heap: ReadyHeap{},
            delayed_heap: DelayedHeap{},
            lease_heap: LeaseHeap{},
            dead_letter_indices: .empty,
            pending_count: 0,
            delayed_count: 0,
            running_count: 0,
            completed_count: 0,
            dead_letter_count: 0,
            total_enqueued_count: 0,
            total_retried_count: 0,
        };
    }

    /// Initializes a bounded `JobQueue` that returns `QueueError.QueueFull` when active
    /// (non-completed) jobs reach `max_active_jobs` (`0` disables the limit).
    pub static fn initWithCapacity(
        alloc: std.mem.Allocator,
        default_retry: RetryPolicy,
        lease_timeout_ms: i64,
        max_active_jobs: usize,
    ): JobQueue! {
        let q = JobQueue.init(alloc, default_retry, lease_timeout_ms)?;
        q.max_active_jobs = max_active_jobs;
        return q;
    }

    /// Releases all owned job strings (`topic`, `payload`, `last_error`) and internal buffers.
    pub fn deinit(*_) {
        for (job of _.jobs.items) {
            if (job.topic.len > 0) {
                _.allocator.free(job.topic);
            }
            if (job.payload.len > 0) {
                _.allocator.free(job.payload);
            }
            if (job.last_error.len > 0) {
                _.allocator.free(job.last_error);
            }
        }
        _.jobs.deinit(_.allocator);
        _.ready_heap.deinit(_.allocator);
        _.delayed_heap.deinit(_.allocator);
        _.lease_heap.deinit(_.allocator);
        _.dead_letter_indices.deinit(_.allocator);
        _.pending_count = 0;
        _.delayed_count = 0;
        _.running_count = 0;
        _.completed_count = 0;
        _.dead_letter_count = 0;
    }

    static fn dupeOwned(alloc: std.mem.Allocator, src: string): string! {
        if (src.len == 0) {
            return "";
        }
        return alloc.dupe(u8, src) catch return QueueError.OutOfMemory;
    }

    static fn freeOwned(alloc: std.mem.Allocator, s: string) {
        if (s.len > 0) {
            alloc.free(s);
        }
    }

    fn activeCount(): usize {
        return _.pending_count + _.delayed_count + _.running_count + _.dead_letter_count;
    }

    fn enqueueInternal(
        *_,
        topic: string,
        payload: string,
        priority: JobPriority,
        delay_ms: i64,
        now_ms: i64,
    ): u64! {
        if (_.max_active_jobs > 0 && _.activeCount() >= _.max_active_jobs) {
            return QueueError.QueueFull;
        }

        const owned_topic = JobQueue.dupeOwned(_.allocator, topic)?;
        errdefer JobQueue.freeOwned(_.allocator, owned_topic);
        const owned_payload = JobQueue.dupeOwned(_.allocator, payload)?;
        errdefer JobQueue.freeOwned(_.allocator, owned_payload);

        const is_delayed = delay_ms > 0;
        const job_id = _.next_id;
        const idx = _.jobs.items.len;
        const run_at = if (is_delayed) (now_ms + delay_ms) else now_ms;
        const init_state: JobState = if (is_delayed) .Delayed else .Pending;

        _.jobs.append(_.allocator, Job{
            id: job_id,
            topic: owned_topic,
            payload: owned_payload,
            priority: priority,
            state: init_state,
            attempts: 0,
            max_attempts: _.default_retry.max_attempts,
            run_at_ms: run_at,
            created_at_ms: now_ms,
            lease_until_ms: 0,
            last_error: "",
        }) catch return QueueError.OutOfMemory;

        if (is_delayed) {
            _.delayed_heap.push(_.allocator, _.jobs.items, idx) catch |e| {
                _.jobs.items.len -= 1;
                return e;
            };
            _.delayed_count += 1;
        } else {
            _.ready_heap.push(_.allocator, _.jobs.items, idx) catch |e| {
                _.jobs.items.len -= 1;
                return e;
            };
            _.pending_count += 1;
        }

        _.next_id += 1;
        _.total_enqueued_count += 1;
        return job_id;
    }

    /// Enqueues an immediately ready job and returns its monotonically assigned `id`.
    pub fn enqueue(
        *_,
        topic: string,
        payload: string,
        priority: JobPriority,
        now_ms: i64,
    ): u64! {
        return _.enqueueInternal(topic, payload, priority, 0, now_ms);
    }

    /// Enqueues a job scheduled to run at `now_ms + delay_ms` and returns its `id`.
    /// When `delay_ms <= 0`, the job is placed directly into `Pending`.
    pub fn enqueueDelayed(
        *_,
        topic: string,
        payload: string,
        priority: JobPriority,
        delay_ms: i64,
        now_ms: i64,
    ): u64! {
        return _.enqueueInternal(topic, payload, priority, delay_ms, now_ms);
    }

    fn promoteDueJobs(*_, now_ms: i64) {
        // 1. Promote due Delayed jobs (`now_ms >= run_at_ms`) -> Pending
        while (top_idx := _.delayed_heap.peek()) {
            const job = _.jobs.items[top_idx];
            if (job.state != .Delayed) {
                * = _.delayed_heap.pop(_.jobs.items);
                continue;
            }
            if (now_ms < job.run_at_ms) {
                break;
            }
            * = _.delayed_heap.pop(_.jobs.items);
            _.jobs.items[top_idx].state = .Pending;
            _.delayed_count -= 1;
            _.pending_count += 1;
            _.ready_heap.push(_.allocator, _.jobs.items, top_idx) catch {};
        }

        // 2. Reclaim expired-lease Running zombie jobs (`now_ms >= lease_until_ms`) -> Pending
        while (top_lease := _.lease_heap.peek()) {
            const job = _.jobs.items[top_lease.idx];
            if (job.state != .Running || job.lease_until_ms != top_lease.lease_until_ms) {
                * = _.lease_heap.pop();
                continue;
            }
            if (now_ms < top_lease.lease_until_ms) {
                break;
            }
            * = _.lease_heap.pop();
            _.jobs.items[top_lease.idx].state = .Pending;
            _.jobs.items[top_lease.idx].lease_until_ms = 0;
            _.running_count -= 1;
            _.pending_count += 1;
            _.ready_heap.push(_.allocator, _.jobs.items, top_lease.idx) catch {};
        }
    }

    /// Promotes due `Delayed` jobs and expired-lease `Running` zombie jobs to `Pending`,
    /// then dequeues the highest-priority ready job (FIFO by `id` on ties), marks it `Running`,
    /// sets `lease_until_ms = now_ms + lease_timeout_ms`, and returns a copy of the `Job`.
    pub fn dequeue(*_, now_ms: i64): Job? {
        _.promoteDueJobs(now_ms);

        while (idx := _.ready_heap.pop(_.jobs.items)) {
            if (_.jobs.items[idx].state != .Pending) {
                continue;
            }
            const lease_until = now_ms + _.lease_timeout_ms;
            _.jobs.items[idx].state = .Running;
            _.jobs.items[idx].lease_until_ms = lease_until;
            _.pending_count -= 1;
            _.running_count += 1;
            _.lease_heap.push(_.allocator, LeaseEntry{
                idx: idx,
                lease_until_ms: lease_until,
            }) catch {};
            return _.jobs.items[idx];
        }
        return null;
    }

    fn findJobIndex(job_id: u64): usize? {
        if (job_id == 0) {
            return null;
        }
        const idx = (job_id - 1) as usize;
        if (idx >= _.jobs.items.len) {
            return null;
        }
        return idx;
    }

    /// Marks an actively `Running` job as `Completed`.
    pub fn ack(*_, job_id: u64): ! {
        const idx = _.findJobIndex(job_id) ?? return QueueError.JobNotFound;
        if (_.jobs.items[idx].state != .Running) {
            return QueueError.InvalidState;
        }
        _.jobs.items[idx].state = .Completed;
        _.jobs.items[idx].lease_until_ms = 0;
        _.running_count -= 1;
        _.completed_count += 1;
    }

    /// Negatively acknowledges a `Running` job with `err_msg` at `now_ms`.
    /// Retries with exponential backoff into `Delayed` while `attempts < max_attempts`,
    /// or moves the job to `DeadLetter` once `attempts` reaches `max_attempts`.
    pub fn nack(*_, job_id: u64, err_msg: string, now_ms: i64): ! {
        const idx = _.findJobIndex(job_id) ?? return QueueError.JobNotFound;
        if (_.jobs.items[idx].state != .Running) {
            return QueueError.InvalidState;
        }

        const owned_err = JobQueue.dupeOwned(_.allocator, err_msg)?;
        const next_attempts = _.jobs.items[idx].attempts + 1;
        const will_dlq = next_attempts >= _.jobs.items[idx].max_attempts;

        if (will_dlq) {
            _.dead_letter_indices.ensureUnusedCapacity(_.allocator, 1) catch {
                JobQueue.freeOwned(_.allocator, owned_err);
                return QueueError.OutOfMemory;
            };
        } else {
            _.delayed_heap.items.ensureUnusedCapacity(_.allocator, 1) catch {
                JobQueue.freeOwned(_.allocator, owned_err);
                return QueueError.OutOfMemory;
            };
        }

        JobQueue.freeOwned(_.allocator, _.jobs.items[idx].last_error);
        _.jobs.items[idx].last_error = owned_err;
        _.jobs.items[idx].lease_until_ms = 0;
        _.running_count -= 1;

        if (will_dlq) {
            _.jobs.items[idx].attempts = next_attempts;
            _.jobs.items[idx].state = .DeadLetter;
            _.dead_letter_indices.appendAssumeCapacity(idx);
            _.dead_letter_count += 1;
        } else {
            const attempt_idx = _.jobs.items[idx].attempts;
            const delay_ms = _.default_retry.nextDelayMs(attempt_idx);
            _.jobs.items[idx].attempts = next_attempts;
            _.jobs.items[idx].state = .Delayed;
            _.jobs.items[idx].run_at_ms = now_ms + delay_ms;
            _.delayed_count += 1;
            _.total_retried_count += 1;
            _.delayed_heap.push(_.allocator, _.jobs.items, idx) catch unreachable;
        }
    }

    /// Returns a caller-owned slice of all jobs currently in `DeadLetter` state.
    /// Caller must free the returned slice with `alloc.free(list)`.
    pub fn deadLetters(alloc: std.mem.Allocator): ([]Job)! {
        let out: std.ArrayList(Job) = .empty;
        errdefer out.deinit(alloc);

        for (idx of _.dead_letter_indices.items) {
            const job = _.jobs.items[idx];
            if (job.state == .DeadLetter) {
                out.append(alloc, job) catch return QueueError.OutOfMemory;
            }
        }
        return out.toOwnedSlice(alloc) catch return QueueError.OutOfMemory;
    }

    /// Replays a `DeadLetter` job back into `Pending` state with `attempts` reset to `0`.
    pub fn requeueDeadLetter(*_, job_id: u64, now_ms: i64): ! {
        const idx = _.findJobIndex(job_id) ?? return QueueError.JobNotFound;
        if (_.jobs.items[idx].state != .DeadLetter) {
            return QueueError.InvalidState;
        }

        _.ready_heap.items.ensureUnusedCapacity(_.allocator, 1) catch return QueueError.OutOfMemory;

        JobQueue.freeOwned(_.allocator, _.jobs.items[idx].last_error);
        _.jobs.items[idx].last_error = "";
        _.jobs.items[idx].attempts = 0;
        _.jobs.items[idx].state = .Pending;
        _.jobs.items[idx].run_at_ms = now_ms;
        _.jobs.items[idx].lease_until_ms = 0;

        // Remove idx from dead_letter_indices
        let i: usize = 0;
        while (i < _.dead_letter_indices.items.len) : (i += 1) {
            if (_.dead_letter_indices.items[i] == idx) {
                * = _.dead_letter_indices.swapRemove(i);
                break;
            }
        }

        _.dead_letter_count -= 1;
        _.pending_count += 1;
        _.ready_heap.push(_.allocator, _.jobs.items, idx) catch unreachable;
    }

    /// Looks up a job record by `job_id`, returning `null` if unknown.
    pub fn getJob(job_id: u64): Job? {
        const idx = _.findJobIndex(job_id) ?? return null;
        return _.jobs.items[idx];
    }

    /// Returns a snapshot of current queue state counts and cumulative counters.
    pub fn stats(): QueueStats {
        return QueueStats{
            pending: _.pending_count,
            delayed: _.delayed_count,
            running: _.running_count,
            completed: _.completed_count,
            dead_letter: _.dead_letter_count,
            total_enqueued: _.total_enqueued_count,
            total_retried: _.total_retried_count,
        };
    }
}
