// Origin: Pure-CP Stdlib (A2A Task Lifecycle State Machine & Lease Queue) | Created: 2026-11-22
// Pure CP zero-allocation A2A task lifecycle state machine and lease-based priority queue:
// - Strict state transition validation (`canTransition`) with irreversible terminal states
// - Idempotency-key deduplicated enqueueing (`enqueue`) preventing duplicate dispatch
// - Priority + earliest-deadline task claiming (`claimNext`) with lease grants
// - Worker lease renewal (`heartbeat`) and expired-lease reclamation (`reclaimExpiredLeases`)
//   automatically requeuing or dead-lettering tasks exceeding `max_attempts`
// - Full lifecycle transitions (`complete`, `fail`, `requestInput`, `resumeInput`, `cancel`)
//   and real-time `A2aQueueStats` aggregation

/// Lifecycle state of an A2A distributed agent task.
pub enum A2aTaskState {
    Submitted,
    Working,
    InputRequired,
    Completed,
    Failed,
    Canceled,
    DeadLetter,
}

/// Validates whether transitioning from `from` to `to` is permitted by the A2A task state machine.
/// Terminal states (`Completed`, `Failed`, `Canceled`, `DeadLetter`) are strictly irreversible.
pub fn canTransition(src: A2aTaskState, dst: A2aTaskState): bool {
    if (src == A2aTaskState.Submitted) {
        return dst == A2aTaskState.Working ||
            dst == A2aTaskState.Canceled ||
            dst == A2aTaskState.DeadLetter;
    }
    if (src == A2aTaskState.Working) {
        return dst == A2aTaskState.Completed ||
            dst == A2aTaskState.Failed ||
            dst == A2aTaskState.InputRequired ||
            dst == A2aTaskState.Canceled ||
            dst == A2aTaskState.Submitted ||
            dst == A2aTaskState.DeadLetter;
    }
    if (src == A2aTaskState.InputRequired) {
        return dst == A2aTaskState.Submitted ||
            dst == A2aTaskState.Working ||
            dst == A2aTaskState.Canceled ||
            dst == A2aTaskState.Failed;
    }
    // Terminal states cannot transition
    return false;
}

/// Fixed-layout descriptor for a queued or active A2A task.
pub struct A2aTaskItem {
    task_id: u64 = 0,
    idempotency_key_hash: u64 = 0,
    priority: i32 = 0,
    created_ms: i64 = 0,
    deadline_ms: i64 = 0,
    lease_until_ms: i64 = 0,
    attempts: u32 = 0,
    max_attempts: u32 = 3,
    worker_id: u32 = 0,
    state: A2aTaskState = A2aTaskState.Submitted,
    artifact_count: u32 = 0,
    occupied: bool = false,
}

/// Aggregate state counts across all tasks in `A2aTaskQueue`.
pub struct A2aQueueStats {
    total_tasks: usize = 0,
    submitted: usize = 0,
    working: usize = 0,
    input_required: usize = 0,
    completed: usize = 0,
    failed: usize = 0,
    canceled: usize = 0,
    dead_letter: usize = 0,
}

fn saturatingAddI64(a: i64, b: i64): i64 {
    const i64_max: i64 = 9223372036854775807;
    if (b > 0 && a > i64_max - b) {
        return i64_max;
    }
    return a + b;
}

/// Returns `true` if candidate `a` should be claimed before `b`:
/// 1. Higher `priority` first
/// 2. Earlier positive `deadline_ms` first (`deadline_ms == 0` treated as no deadline)
/// 3. Earlier `created_ms`, then smaller `task_id`
fn taskHasHigherOrder(a: A2aTaskItem, b: A2aTaskItem): bool {
    if (a.priority != b.priority) {
        return a.priority > b.priority;
    }
    const a_has_ddl = a.deadline_ms > 0;
    const b_has_ddl = b.deadline_ms > 0;
    if (a_has_ddl && b_has_ddl && a.deadline_ms != b.deadline_ms) {
        return a.deadline_ms < b.deadline_ms;
    }
    if (a_has_ddl != b_has_ddl) {
        return a_has_ddl;
    }
    if (a.created_ms != b.created_ms) {
        return a.created_ms < b.created_ms;
    }
    return a.task_id < b.task_id;
}

/// Zero-allocation fixed-pool A2A task lifecycle queue with leases and idempotency deduplication.
pub struct A2aTaskQueue {
    slots: *[]A2aTaskItem,

    /// Initializes a task queue over caller-provided `slots`, marking all slots unoccupied.
    pub static fn init(slots: *[]A2aTaskItem): A2aTaskQueue {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i].occupied = false;
        }
        return A2aTaskQueue{
            slots: slots,
        };
    }

    fn findSlotById(task_id: u64): usize? {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].task_id == task_id) {
                return i;
            }
        }
        return null;
    }

    fn findSlotByIdempotency(key_hash: u64): usize? {
        if (key_hash == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].idempotency_key_hash == key_hash) {
                return i;
            }
        }
        return null;
    }

    /// Enqueues a new A2A task. If `idempotency_key_hash != 0` and already exists in the queue,
    /// returns the existing task's `task_id` without inserting a duplicate.
    /// Returns `null` if `task_id` collides with a different task or the queue pool is full.
    pub fn enqueue(
        *_,
        task_id: u64,
        idempotency_key_hash: u64,
        priority: i32,
        created_ms: i64,
        deadline_ms: i64,
        max_attempts: u32,
    ): u64? {
        if (dup_idx := _.findSlotByIdempotency(idempotency_key_hash)) {
            return _.slots[dup_idx].task_id;
        }
        if (_.findSlotById(task_id) != null) {
            return null;
        }

        const eff_max: u32 = if (max_attempts == 0) 1 else max_attempts;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) {
                _.slots[i] = A2aTaskItem{
                    task_id: task_id,
                    idempotency_key_hash: idempotency_key_hash,
                    priority: priority,
                    created_ms: created_ms,
                    deadline_ms: deadline_ms,
                    lease_until_ms: 0,
                    attempts: 0,
                    max_attempts: eff_max,
                    worker_id: 0,
                    state: A2aTaskState.Submitted,
                    artifact_count: 0,
                    occupied: true,
                };
                return task_id;
            }
        }
        return null;
    }

    /// Claims the highest-priority (tie-broken by earliest `deadline_ms`, `created_ms`, `task_id`)
    /// `Submitted` task for `worker_id` and grants a lease until `now_ms + lease_ms`.
    /// Returns the claimed `task_id`, or `null` if no claimable task exists or args are invalid.
    pub fn claimNext(*_, worker_id: u32, now_ms: i64, lease_ms: i64): u64? {
        if (worker_id == 0 || lease_ms <= 0) {
            return null;
        }

        let best_idx: usize = 0;
        let found: bool = false;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            const item = _.slots[i];
            if (item.occupied && item.state == A2aTaskState.Submitted) {
                if (!found || taskHasHigherOrder(item, _.slots[best_idx])) {
                    found = true;
                    best_idx = i;
                }
            }
        }

        if (!found) {
            return null;
        }

        _.slots[best_idx].state = A2aTaskState.Working;
        _.slots[best_idx].worker_id = worker_id;
        _.slots[best_idx].attempts += 1;
        _.slots[best_idx].lease_until_ms = saturatingAddI64(now_ms, lease_ms);
        return _.slots[best_idx].task_id;
    }

    /// Renews the active lease for `task_id` held by `worker_id` if not yet expired at `now_ms`.
    pub fn heartbeat(*_, task_id: u64, worker_id: u32, now_ms: i64, lease_ms: i64): bool {
        if (worker_id == 0 || lease_ms <= 0) {
            return false;
        }
        if (idx := _.findSlotById(task_id)) {
            if (_.slots[idx].state != A2aTaskState.Working) {
                return false;
            }
            if (_.slots[idx].worker_id != worker_id) {
                return false;
            }
            if (now_ms >= _.slots[idx].lease_until_ms) {
                return false;
            }
            _.slots[idx].lease_until_ms = saturatingAddI64(now_ms, lease_ms);
            return true;
        }
        return false;
    }

    /// Scans all `Working` tasks whose lease has expired (`now_ms >= lease_until_ms`).
    /// Tasks with `attempts < max_attempts` are requeued to `Submitted`; tasks that have
    /// exhausted `max_attempts` transition to `DeadLetter`.
    /// Returns the total number of tasks reclaimed.
    pub fn reclaimExpiredLeases(*_, now_ms: i64): usize {
        let reclaimed: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied &&
                _.slots[i].state == A2aTaskState.Working &&
                now_ms >= _.slots[i].lease_until_ms) {
                _.slots[i].worker_id = 0;
                _.slots[i].lease_until_ms = 0;
                if (_.slots[i].attempts >= _.slots[i].max_attempts) {
                    _.slots[i].state = A2aTaskState.DeadLetter;
                } else {
                    _.slots[i].state = A2aTaskState.Submitted;
                }
                reclaimed += 1;
            }
        }
        return reclaimed;
    }

    /// Marks an active `Working` task held by `worker_id` as `Completed` with `artifact_count`.
    pub fn complete(*_, task_id: u64, worker_id: u32, artifact_count: u32): bool {
        if (idx := _.findSlotById(task_id)) {
            if (!canTransition(_.slots[idx].state, A2aTaskState.Completed)) {
                return false;
            }
            if (_.slots[idx].worker_id != worker_id) {
                return false;
            }
            _.slots[idx].state = A2aTaskState.Completed;
            _.slots[idx].artifact_count = artifact_count;
            _.slots[idx].lease_until_ms = 0;
            return true;
        }
        return false;
    }

    /// Fails an active `Working` task held by `worker_id`.
    /// When `retryable == true`, requeues to `Submitted` if `attempts < max_attempts`,
    /// otherwise transitions to `DeadLetter`. When `retryable == false`, transitions to `Failed`.
    pub fn fail(*_, task_id: u64, worker_id: u32, retryable: bool): bool {
        if (idx := _.findSlotById(task_id)) {
            if (_.slots[idx].state != A2aTaskState.Working || _.slots[idx].worker_id != worker_id) {
                return false;
            }
            _.slots[idx].worker_id = 0;
            _.slots[idx].lease_until_ms = 0;
            if (retryable) {
                if (_.slots[idx].attempts >= _.slots[idx].max_attempts) {
                    _.slots[idx].state = A2aTaskState.DeadLetter;
                } else {
                    _.slots[idx].state = A2aTaskState.Submitted;
                }
            } else {
                _.slots[idx].state = A2aTaskState.Failed;
            }
            return true;
        }
        return false;
    }

    /// Suspends a `Working` task held by `worker_id` into `InputRequired` (human/peer clarification).
    pub fn requestInput(*_, task_id: u64, worker_id: u32): bool {
        if (idx := _.findSlotById(task_id)) {
            if (!canTransition(_.slots[idx].state, A2aTaskState.InputRequired)) {
                return false;
            }
            if (_.slots[idx].worker_id != worker_id) {
                return false;
            }
            _.slots[idx].state = A2aTaskState.InputRequired;
            _.slots[idx].lease_until_ms = 0;
            return true;
        }
        return false;
    }

    /// Resumes an `InputRequired` task back to `Submitted` so a worker can claim and continue it.
    pub fn resumeInput(*_, task_id: u64): bool {
        if (idx := _.findSlotById(task_id)) {
            if (_.slots[idx].state != A2aTaskState.InputRequired) {
                return false;
            }
            _.slots[idx].state = A2aTaskState.Submitted;
            _.slots[idx].worker_id = 0;
            return true;
        }
        return false;
    }

    /// Cancels a non-terminal task (`Submitted`, `Working`, or `InputRequired`).
    pub fn cancel(*_, task_id: u64): bool {
        if (idx := _.findSlotById(task_id)) {
            if (!canTransition(_.slots[idx].state, A2aTaskState.Canceled)) {
                return false;
            }
            _.slots[idx].state = A2aTaskState.Canceled;
            _.slots[idx].worker_id = 0;
            _.slots[idx].lease_until_ms = 0;
            return true;
        }
        return false;
    }

    /// Returns a copy of `task_id`'s descriptor, or `null` if not found.
    pub fn findTask(task_id: u64): A2aTaskItem? {
        if (idx := _.findSlotById(task_id)) {
            return _.slots[idx];
        }
        return null;
    }

    /// Computes aggregate task counts grouped by lifecycle state.
    pub fn stats(): A2aQueueStats {
        let st = A2aQueueStats{};
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            const item = _.slots[i];
            if (!item.occupied) {
                continue;
            }
            st.total_tasks += 1;
            if (item.state == A2aTaskState.Submitted) {
                st.submitted += 1;
            } else if (item.state == A2aTaskState.Working) {
                st.working += 1;
            } else if (item.state == A2aTaskState.InputRequired) {
                st.input_required += 1;
            } else if (item.state == A2aTaskState.Completed) {
                st.completed += 1;
            } else if (item.state == A2aTaskState.Failed) {
                st.failed += 1;
            } else if (item.state == A2aTaskState.Canceled) {
                st.canceled += 1;
            } else {
                st.dead_letter += 1;
            }
        }
        return st;
    }
}
