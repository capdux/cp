// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Priority Aging Work Queue & Backpressure Scheduler) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation fixed-capacity work queue featuring:
//   - Multi-priority scheduling (0..3) with starvation-free priority aging
//   - Configurable full-queue backpressure (reject_new | drop_lowest_priority | drop_oldest)
//   - Lazy task cancellation by task_id, automatic deadline expiry skipping, and batch sweepExpired
//   - Monotonic clock rollback guard and comprehensive telemetry stats

/// Maximum fixed task capacity supported by `WorkQueue`.
pub const WORK_QUEUE_MAX_CAPACITY: usize = 128;

/// Maximum valid task priority level (levels: 0=low, 1=normal, 2=high, 3=critical).
pub const WORK_QUEUE_MAX_PRIORITY: u8 = 3;

// ============================================================================
// Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Backpressure policy applied when the queue is full of live, non-expired tasks.
pub enum WorkBackpressurePolicy: u8 {
    /// Reject the incoming task without evicting any existing task.
    reject_new = 0,
    /// Evict the lowest-priority (oldest on ties) task if its priority <= incoming priority;
    /// otherwise reject the incoming lower-priority task.
    drop_lowest_priority = 1,
    /// Evict the oldest live task regardless of priority to make room for the incoming task.
    drop_oldest = 2,
}

/// Detailed result status returned by `WorkQueue.enqueue`.
pub enum WorkEnqueueStatus: u8 {
    accepted = 0,
    accepted_after_evict = 1,
    rejected_full = 2,
    rejected_invalid_id = 3,
    rejected_duplicate_id = 4,
    rejected_invalid_priority = 5,
    rejected_already_expired = 6,
}

/// Configuration for `WorkQueue`.
/// - `capacity`: active slot limit in `1..=WORK_QUEUE_MAX_CAPACITY`.
/// - `aging_step_ms`: wait duration (ms) per +1 effective priority boost (`0` disables aging).
/// - `backpressure`: full-queue eviction/rejection strategy.
pub struct WorkQueueConfig {
    capacity: usize = WORK_QUEUE_MAX_CAPACITY;
    aging_step_ms: u64 = 1000;
    backpressure: WorkBackpressurePolicy = WorkBackpressurePolicy.reject_new;

    /// Returns default configuration (capacity=128, aging_step_ms=1000, reject_new).
    static fn defaults(): WorkQueueConfig {
        return WorkQueueConfig{
            capacity: WORK_QUEUE_MAX_CAPACITY,
            aging_step_ms: 1000,
            backpressure: WorkBackpressurePolicy.reject_new,
        };
    }

    /// Validates that `capacity` is within `[1, WORK_QUEUE_MAX_CAPACITY]`.
    fn isValid(): bool {
        return _.capacity >= 1 && _.capacity <= WORK_QUEUE_MAX_CAPACITY;
    }
}

/// Fixed-size work item stored in a `WorkQueue` slot.
pub struct WorkTask {
    task_id: u64 = 0;
    priority: u8 = 0;
    enqueued_ms: u64 = 0;
    deadline_ms: u64 = 0;
    payload_id: u64 = 0;
    cancelled: bool = false;
    occupied: bool = false;
    seq: u64 = 0;

    static fn empty(): WorkTask {
        return WorkTask{
            task_id: 0,
            priority: 0,
            enqueued_ms: 0,
            deadline_ms: 0,
            payload_id: 0,
            cancelled: false,
            occupied: false,
            seq: 0,
        };
    }
}

/// Outcome of `WorkQueue.enqueue`.
pub struct WorkEnqueueOutcome {
    status: WorkEnqueueStatus = WorkEnqueueStatus.accepted;
    evicted_task_id: u64 = 0;

    /// Returns `true` if the task was accepted (either directly or after eviction).
    fn isAccepted(): bool {
        return _.status == WorkEnqueueStatus.accepted || _.status == WorkEnqueueStatus.accepted_after_evict;
    }
}

/// Dequeued (or peeked) work item with computed effective priority and wait duration.
pub struct WorkDequeueItem {
    task: WorkTask = WorkTask.empty();
    effective_priority: u8 = 0;
    wait_ms: u64 = 0;
}

/// Full telemetry snapshot for `WorkQueue`.
pub struct WorkQueueStats {
    pending_count: usize = 0;
    occupied_slots: usize = 0;
    capacity: usize = 0;
    total_enqueued: u64 = 0;
    total_dequeued: u64 = 0;
    total_rejected: u64 = 0;
    total_evicted: u64 = 0;
    total_cancelled: u64 = 0;
    total_expired: u64 = 0;
    total_aged_promotions: u64 = 0;
    last_clock_ms: u64 = 0;
}

// ============================================================================
// Internal Helpers
// ============================================================================

fn computeEffectivePriority(base_priority: u8, enqueued_ms: u64, now_ms: u64, aging_step_ms: u64): u8 {
    if (aging_step_ms == 0 || now_ms <= enqueued_ms) {
        return base_priority;
    }
    const wait_ms: u64 = now_ms - enqueued_ms;
    const boost: u64 = wait_ms / aging_step_ms;
    const max_p: u64 = WORK_QUEUE_MAX_PRIORITY as u64;
    const base_u64: u64 = base_priority as u64;
    if (boost >= max_p || base_u64 + boost >= max_p) {
        return WORK_QUEUE_MAX_PRIORITY;
    }
    return (base_u64 + boost) as u8;
}

fn isTaskExpired(task: WorkTask, now_ms: u64): bool {
    return task.deadline_ms > 0 && now_ms >= task.deadline_ms;
}

// ============================================================================
// Core Multi-Priority Aging Work Queue Engine
// ============================================================================

/// Zero-heap-allocation fixed-capacity multi-priority work queue with starvation aging,
/// configurable backpressure, lazy task cancellation, deadline expiration, and clock-rollback safety.
pub struct WorkQueue {
    config: WorkQueueConfig = WorkQueueConfig.defaults();
    slots: [WORK_QUEUE_MAX_CAPACITY]WorkTask = [WorkTask.empty(); WORK_QUEUE_MAX_CAPACITY];
    next_seq: u64 = 1;
    high_water_ms: u64 = 0;
    total_enqueued: u64 = 0;
    total_dequeued: u64 = 0;
    total_rejected: u64 = 0;
    total_evicted: u64 = 0;
    total_cancelled: u64 = 0;
    total_expired: u64 = 0;
    total_aged_promotions: u64 = 0;

    /// Initializes a `WorkQueue` with the given configuration; returns `null` if invalid.
    static fn init(config: WorkQueueConfig): WorkQueue? {
        if (!config.isValid()) {
            return null;
        }
        return WorkQueue{
            config: config,
            slots: [WorkTask.empty(); WORK_QUEUE_MAX_CAPACITY],
            next_seq: 1,
            high_water_ms: 0,
            total_enqueued: 0,
            total_dequeued: 0,
            total_rejected: 0,
            total_evicted: 0,
            total_cancelled: 0,
            total_expired: 0,
            total_aged_promotions: 0,
        };
    }

    /// Advances the internal monotonic clock high-water mark to guard against clock rollback.
    fn effectiveNow(*_, now_ms: u64): u64 {
        if (now_ms > _.high_water_ms) {
            _.high_water_ms = now_ms;
        }
        return _.high_water_ms;
    }

    /// Enqueues a new task into the work queue, applying backpressure if full.
    /// - Rejects `task_id == 0`, `priority > 3`, duplicate active `task_id`, or already-expired `deadline_ms`.
    /// - Automatically reclaims cancelled or expired slots before triggering backpressure eviction.
    fn enqueue(
        *_,
        task_id: u64,
        priority: u8,
        now_ms: u64,
        deadline_ms: u64,
        payload_id: u64,
    ): WorkEnqueueOutcome {
        const eff_now: u64 = _.effectiveNow(now_ms);

        if (task_id == 0) {
            _.total_rejected += 1;
            return WorkEnqueueOutcome{
                status: WorkEnqueueStatus.rejected_invalid_id,
                evicted_task_id: 0,
            };
        }
        if (priority > WORK_QUEUE_MAX_PRIORITY) {
            _.total_rejected += 1;
            return WorkEnqueueOutcome{
                status: WorkEnqueueStatus.rejected_invalid_priority,
                evicted_task_id: 0,
            };
        }
        if (deadline_ms > 0 && eff_now >= deadline_ms) {
            _.total_rejected += 1;
            return WorkEnqueueOutcome{
                status: WorkEnqueueStatus.rejected_already_expired,
                evicted_task_id: 0,
            };
        }

        // Check for duplicate task_id across live (non-cancelled, non-expired) occupied slots
        let free_idx: usize = _.config.capacity;
        let i: usize = 0;
        while (i < _.config.capacity) : (i += 1) {
            const s = _.slots[i];
            if (!s.occupied) {
                if (free_idx == _.config.capacity) {
                    free_idx = i;
                }
                continue;
            }
            if (s.cancelled) {
                continue;
            }
            if (isTaskExpired(s, eff_now)) {
                continue;
            }
            if (s.task_id == task_id) {
                _.total_rejected += 1;
                return WorkEnqueueOutcome{
                    status: WorkEnqueueStatus.rejected_duplicate_id,
                    evicted_task_id: 0,
                };
            }
        }

        // If no immediate free slot, reclaim cancelled or expired slots first
        if (free_idx == _.config.capacity) {
            let r: usize = 0;
            while (r < _.config.capacity) : (r += 1) {
                const s = _.slots[r];
                if (!s.occupied) {
                    if (free_idx == _.config.capacity) {
                        free_idx = r;
                    }
                    continue;
                }
                if (s.cancelled) {
                    _.slots[r] = WorkTask.empty();
                    if (free_idx == _.config.capacity) {
                        free_idx = r;
                    }
                } else if (isTaskExpired(s, eff_now)) {
                    _.slots[r] = WorkTask.empty();
                    _.total_expired += 1;
                    if (free_idx == _.config.capacity) {
                        free_idx = r;
                    }
                }
            }
        }

        // Direct insert if a slot is available
        if (free_idx < _.config.capacity) {
            _.slots[free_idx] = WorkTask{
                task_id: task_id,
                priority: priority,
                enqueued_ms: eff_now,
                deadline_ms: deadline_ms,
                payload_id: payload_id,
                cancelled: false,
                occupied: true,
                seq: _.next_seq,
            };
            _.next_seq += 1;
            _.total_enqueued += 1;
            return WorkEnqueueOutcome{
                status: WorkEnqueueStatus.accepted,
                evicted_task_id: 0,
            };
        }

        // Queue is genuinely full of live tasks -> apply backpressure policy
        if (_.config.backpressure == WorkBackpressurePolicy.reject_new) {
            _.total_rejected += 1;
            return WorkEnqueueOutcome{
                status: WorkEnqueueStatus.rejected_full,
                evicted_task_id: 0,
            };
        }

        let victim_idx: usize = _.config.capacity;
        if (_.config.backpressure == WorkBackpressurePolicy.drop_lowest_priority) {
            let j: usize = 0;
            while (j < _.config.capacity) : (j += 1) {
                const cand = _.slots[j];
                if (!cand.occupied) continue;
                if (victim_idx == _.config.capacity) {
                    victim_idx = j;
                } else {
                    const cur = _.slots[victim_idx];
                    if (cand.priority < cur.priority ||
                        (cand.priority == cur.priority && cand.enqueued_ms < cur.enqueued_ms) ||
                        (cand.priority == cur.priority && cand.enqueued_ms == cur.enqueued_ms && cand.seq < cur.seq))
                    {
                        victim_idx = j;
                    }
                }
            }
            // Do not evict a strictly higher-priority task for a lower-priority incoming task
            if (victim_idx == _.config.capacity || _.slots[victim_idx].priority > priority) {
                _.total_rejected += 1;
                return WorkEnqueueOutcome{
                    status: WorkEnqueueStatus.rejected_full,
                    evicted_task_id: 0,
                };
            }
        } else {
            // drop_oldest: evict task with earliest enqueued_ms (tie-break by smallest seq)
            let j: usize = 0;
            while (j < _.config.capacity) : (j += 1) {
                const cand = _.slots[j];
                if (!cand.occupied) continue;
                if (victim_idx == _.config.capacity) {
                    victim_idx = j;
                } else {
                    const cur = _.slots[victim_idx];
                    if (cand.enqueued_ms < cur.enqueued_ms ||
                        (cand.enqueued_ms == cur.enqueued_ms && cand.seq < cur.seq))
                    {
                        victim_idx = j;
                    }
                }
            }
        }

        const evicted_id: u64 = _.slots[victim_idx].task_id;
        _.slots[victim_idx] = WorkTask{
            task_id: task_id,
            priority: priority,
            enqueued_ms: eff_now,
            deadline_ms: deadline_ms,
            payload_id: payload_id,
            cancelled: false,
            occupied: true,
            seq: _.next_seq,
        };
        _.next_seq += 1;
        _.total_evicted += 1;
        _.total_enqueued += 1;
        return WorkEnqueueOutcome{
            status: WorkEnqueueStatus.accepted_after_evict,
            evicted_task_id: evicted_id,
        };
    }

    /// Dequeues the highest effective-priority live task at `now_ms`.
    /// Automatically skips and reclaims cancelled and deadline-expired tasks.
    /// Applies starvation aging: each `aging_step_ms` waited boosts effective priority by +1 (capped at 3).
    fn dequeueNext(*_, now_ms: u64): WorkDequeueItem? {
        const eff_now: u64 = _.effectiveNow(now_ms);
        let best_idx: usize = _.config.capacity;
        let best_eff_prio: u8 = 0;

        let i: usize = 0;
        while (i < _.config.capacity) : (i += 1) {
            const s = _.slots[i];
            if (!s.occupied) continue;

            if (s.cancelled) {
                _.slots[i] = WorkTask.empty();
                continue;
            }
            if (isTaskExpired(s, eff_now)) {
                _.slots[i] = WorkTask.empty();
                _.total_expired += 1;
                continue;
            }

            const eff_p: u8 = computeEffectivePriority(s.priority, s.enqueued_ms, eff_now, _.config.aging_step_ms);
            if (best_idx == _.config.capacity) {
                best_idx = i;
                best_eff_prio = eff_p;
            } else {
                const cur = _.slots[best_idx];
                if (eff_p > best_eff_prio ||
                    (eff_p == best_eff_prio && s.enqueued_ms < cur.enqueued_ms) ||
                    (eff_p == best_eff_prio && s.enqueued_ms == cur.enqueued_ms && s.seq < cur.seq))
                {
                    best_idx = i;
                    best_eff_prio = eff_p;
                }
            }
        }

        if (best_idx == _.config.capacity) {
            return null;
        }

        const chosen = _.slots[best_idx];
        _.slots[best_idx] = WorkTask.empty();
        _.total_dequeued += 1;
        if (best_eff_prio > chosen.priority) {
            _.total_aged_promotions += 1;
        }
        const wait_ms: u64 = if (eff_now > chosen.enqueued_ms) (eff_now - chosen.enqueued_ms) else 0;
        return WorkDequeueItem{
            task: chosen,
            effective_priority: best_eff_prio,
            wait_ms: wait_ms,
        };
    }

    /// Read-only peek of the next task that would be dequeued at `now_ms` without mutating queue state.
    fn peekNext(now_ms: u64): WorkDequeueItem? {
        const eff_now: u64 = if (now_ms > _.high_water_ms) now_ms else _.high_water_ms;
        let best_idx: usize = _.config.capacity;
        let best_eff_prio: u8 = 0;

        let i: usize = 0;
        while (i < _.config.capacity) : (i += 1) {
            const s = _.slots[i];
            if (!s.occupied || s.cancelled || isTaskExpired(s, eff_now)) {
                continue;
            }
            const eff_p: u8 = computeEffectivePriority(s.priority, s.enqueued_ms, eff_now, _.config.aging_step_ms);
            if (best_idx == _.config.capacity) {
                best_idx = i;
                best_eff_prio = eff_p;
            } else {
                const cur = _.slots[best_idx];
                if (eff_p > best_eff_prio ||
                    (eff_p == best_eff_prio && s.enqueued_ms < cur.enqueued_ms) ||
                    (eff_p == best_eff_prio && s.enqueued_ms == cur.enqueued_ms && s.seq < cur.seq))
                {
                    best_idx = i;
                    best_eff_prio = eff_p;
                }
            }
        }

        if (best_idx == _.config.capacity) {
            return null;
        }
        const chosen = _.slots[best_idx];
        const wait_ms: u64 = if (eff_now > chosen.enqueued_ms) (eff_now - chosen.enqueued_ms) else 0;
        return WorkDequeueItem{
            task: chosen,
            effective_priority: best_eff_prio,
            wait_ms: wait_ms,
        };
    }

    /// Lazily cancels a queued task by `task_id`.
    /// Returns `true` if a live (non-cancelled) task was found and marked cancelled; `false` otherwise.
    fn cancelTask(*_, task_id: u64): bool {
        if (task_id == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.config.capacity) : (i += 1) {
            if (_.slots[i].occupied && !_.slots[i].cancelled && _.slots[i].task_id == task_id) {
                _.slots[i].cancelled = true;
                _.total_cancelled += 1;
                return true;
            }
        }
        return false;
    }

    /// Batch-sweeps all expired tasks (and reclaims any cancelled tombstones) at `now_ms`.
    /// Returns the number of newly expired tasks swept in this pass.
    fn sweepExpired(*_, now_ms: u64): usize {
        const eff_now: u64 = _.effectiveNow(now_ms);
        let expired_now: usize = 0;
        let i: usize = 0;
        while (i < _.config.capacity) : (i += 1) {
            const s = _.slots[i];
            if (!s.occupied) continue;
            if (s.cancelled) {
                _.slots[i] = WorkTask.empty();
            } else if (isTaskExpired(s, eff_now)) {
                _.slots[i] = WorkTask.empty();
                _.total_expired += 1;
                expired_now += 1;
            }
        }
        return expired_now;
    }

    /// Finds an occupied task snapshot by `task_id` (returns `null` if absent).
    fn findTask(task_id: u64): WorkTask? {
        if (task_id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.config.capacity) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].task_id == task_id) {
                return _.slots[i];
            }
        }
        return null;
    }

    /// Returns the number of live (occupied and non-cancelled) tasks currently queued.
    fn len(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.config.capacity) : (i += 1) {
            if (_.slots[i].occupied && !_.slots[i].cancelled) {
                count += 1;
            }
        }
        return count;
    }

    /// Returns `true` when no live (non-cancelled) tasks are queued.
    fn isEmpty(): bool {
        return _.len() == 0;
    }

    /// Returns `true` when live non-cancelled tasks reach configured capacity.
    fn isFull(): bool {
        return _.len() >= _.config.capacity;
    }

    /// Returns the configured queue capacity.
    fn capacity(): usize {
        return _.config.capacity;
    }

    /// Clears all queued tasks while preserving configuration and monotonic clock high-water mark.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.config.capacity) : (i += 1) {
            _.slots[i] = WorkTask.empty();
        }
    }

    /// Returns a comprehensive telemetry snapshot of queue occupancy and lifecycle counters.
    fn stats(): WorkQueueStats {
        let pending: usize = 0;
        let occupied: usize = 0;
        let i: usize = 0;
        while (i < _.config.capacity) : (i += 1) {
            if (_.slots[i].occupied) {
                occupied += 1;
                if (!_.slots[i].cancelled) {
                    pending += 1;
                }
            }
        }
        return WorkQueueStats{
            pending_count: pending,
            occupied_slots: occupied,
            capacity: _.config.capacity,
            total_enqueued: _.total_enqueued,
            total_dequeued: _.total_dequeued,
            total_rejected: _.total_rejected,
            total_evicted: _.total_evicted,
            total_cancelled: _.total_cancelled,
            total_expired: _.total_expired,
            total_aged_promotions: _.total_aged_promotions,
            last_clock_ms: _.high_water_ms,
        };
    }
}
