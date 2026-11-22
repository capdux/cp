// Origin: Pure-CP Stdlib (AI Agent Workflow DAG Scheduler & Parallel Fan-Out/Fan-In) | Created: 2026-11-22

fn satAddU8(a: u8, b: u8): u8 {
    if (a > 255 - b) {
        return 255;
    }
    return a + b;
}

/// Lifecycle state of a task node inside `AgentDag`.
pub enum TaskState {
    Pending,
    Ready,
    Running,
    Completed,
    Failed,
    Skipped,
    Cancelled,
}

/// Task node stored inside a caller-provided `DagTask` slice.
pub struct DagTask {
    id: u32,
    priority: i32,
    max_retries: u8,
    attempts: u8,
    in_degree: u16,
    state: TaskState,
    allow_failure: bool,
}

/// Directed dependency edge (`to` depends on `from`).
pub struct DagEdge {
    from: u32,
    to: u32,
}

/// Aggregate execution snapshot returned by `AgentDag.summary`.
pub struct DagSummary {
    pending: usize,
    ready: usize,
    running: usize,
    completed: usize,
    failed: usize,
    skipped: usize,
    cancelled: usize,
    is_done: bool,
}

/// Zero-allocation multi-agent workflow DAG scheduler with deterministic priority scheduling,
/// parallel Fan-Out batch claiming, retries, and failure cascade skipping.
pub struct AgentDag {
    tasks: []DagTask,
    edges: []DagEdge,
    task_count: usize,
    edge_count: usize,
    validated: bool,

    /// Initializes an empty `AgentDag` backed by caller-provided task and edge buffers.
    fn init(tasks_buf: *[]DagTask, edges_buf: *[]DagEdge): AgentDag {
        return AgentDag {
            tasks: tasks_buf.*,
            edges: edges_buf.*,
            task_count: 0,
            edge_count: 0,
            validated: false,
        };
    }

    /// Adds a new task node in `Pending` state. Returns its assigned task ID, or `null` if the task pool is full.
    fn addTask(*_, priority: i32, max_retries: u8, allow_failure: bool): u32? {
        if (_.task_count >= _.tasks.len) {
            return null;
        }
        const id = _.task_count as u32;
        _.tasks[_.task_count] = DagTask {
            id: id,
            priority: priority,
            max_retries: max_retries,
            attempts: 0,
            in_degree: 0,
            state: TaskState.Pending,
            allow_failure: allow_failure,
        };
        _.task_count += 1;
        _.validated = false;
        return id;
    }

    /// Adds a directed dependency edge `from_id -> to_id` (`to_id` depends on `from_id`).
    /// Rejects self-loops, out-of-range task IDs, duplicate edges, in-degree overflow, and full edge pools.
    fn addDependency(*_, from_id: u32, to_id: u32): bool {
        if (from_id == to_id) {
            return false;
        }
        if ((from_id as usize) >= _.task_count || (to_id as usize) >= _.task_count) {
            return false;
        }
        if (_.edge_count >= _.edges.len) {
            return false;
        }
        if (_.tasks[to_id as usize].in_degree == 0xFFFF) {
            return false;
        }

        let i: usize = 0;
        while (i < _.edge_count) {
            if (_.edges[i].from == from_id && _.edges[i].to == to_id) {
                return false;
            }
            i += 1;
        }

        _.edges[_.edge_count] = DagEdge {
            from: from_id,
            to: to_id,
        };
        _.edge_count += 1;
        _.tasks[to_id as usize].in_degree += 1;
        _.validated = false;
        return true;
    }

    /// Validates that the dependency graph is acyclic using Kahn's topological sort over caller-provided scratch buffers.
    /// On success, promotes all `Pending` root tasks (`in_degree == 0`) to `Ready` and returns `true`.
    /// Returns `false` if a cycle is detected or scratch buffers are smaller than `task_count`.
    fn validateAcyclic(*_, indeg_s: *[]u16, queue_s: *[]u32): bool {
        if (indeg_s.*.len < _.task_count || queue_s.*.len < _.task_count) {
            return false;
        }
        if (_.task_count == 0) {
            _.validated = true;
            return true;
        }

        let i: usize = 0;
        while (i < _.task_count) {
            indeg_s.*[i] = 0;
            i += 1;
        }

        let e: usize = 0;
        while (e < _.edge_count) {
            const dst = _.edges[e].to as usize;
            indeg_s.*[dst] += 1;
            e += 1;
        }

        let head: usize = 0;
        let tail: usize = 0;
        let n: usize = 0;
        while (n < _.task_count) {
            if (indeg_s.*[n] == 0) {
                queue_s.*[tail] = n as u32;
                tail += 1;
            }
            n += 1;
        }

        let visited: usize = 0;
        while (head < tail) {
            const u = queue_s.*[head];
            head += 1;
            visited += 1;

            let k: usize = 0;
            while (k < _.edge_count) {
                if (_.edges[k].from == u) {
                    const v = _.edges[k].to as usize;
                    indeg_s.*[v] -= 1;
                    if (indeg_s.*[v] == 0) {
                        queue_s.*[tail] = v as u32;
                        tail += 1;
                    }
                }
                k += 1;
            }
        }

        if (visited != _.task_count) {
            _.validated = false;
            return false;
        }

        let r: usize = 0;
        while (r < _.task_count) {
            if (_.tasks[r].state == TaskState.Pending && _.tasks[r].in_degree == 0) {
                _.tasks[r].state = TaskState.Ready;
            }
            r += 1;
        }
        _.validated = true;
        return true;
    }

    /// Deterministically claims the next `Ready` task with the highest `priority` (tie-breaking by smallest `id`)
    /// and transitions it to `Running`. Returns `null` if no `Ready` task exists or the DAG is not validated.
    fn claimNextReady(*_): u32? {
        if (!_.validated) {
            return null;
        }

        let found: bool = false;
        let best_idx: usize = 0;
        let best_priority: i32 = 0;

        let i: usize = 0;
        while (i < _.task_count) {
            if (_.tasks[i].state == TaskState.Ready) {
                if (!found || _.tasks[i].priority > best_priority) {
                    found = true;
                    best_idx = i;
                    best_priority = _.tasks[i].priority;
                }
            }
            i += 1;
        }

        if (!found) {
            return null;
        }

        _.tasks[best_idx].state = TaskState.Running;
        return _.tasks[best_idx].id;
    }

    /// Claims up to `out_ids.*.len` parallel Fan-Out `Ready` tasks in deterministic priority/id order.
    /// Returns the number of claimed tasks written into `out_ids.*`.
    fn claimReadyBatch(*_, out_ids: *[]u32): usize {
        let count: usize = 0;
        while (count < out_ids.*.len) {
            if (id := _.claimNextReady()) {
                out_ids.*[count] = id;
                count += 1;
            } else {
                break;
            }
        }
        return count;
    }

    fn unlockSuccessors(*_, id: u32) {
        let e: usize = 0;
        while (e < _.edge_count) {
            if (_.edges[e].from == id) {
                const dst = _.edges[e].to as usize;
                if (_.tasks[dst].in_degree > 0) {
                    _.tasks[dst].in_degree -= 1;
                }
                if (_.tasks[dst].in_degree == 0 && _.tasks[dst].state == TaskState.Pending) {
                    _.tasks[dst].state = TaskState.Ready;
                }
            }
            e += 1;
        }
    }

    /// Marks a `Running` task as `Completed`, decrements downstream `in_degree`, and promotes
    /// any `Pending` successor whose `in_degree` reaches 0 to `Ready`.
    fn completeTask(*_, id: u32): bool {
        if ((id as usize) >= _.task_count) {
            return false;
        }
        const idx = id as usize;
        if (_.tasks[idx].state != TaskState.Running) {
            return false;
        }

        _.tasks[idx].state = TaskState.Completed;
        _.tasks[idx].attempts = satAddU8(_.tasks[idx].attempts, 1);
        _.unlockSuccessors(id);
        return true;
    }

    /// Handles failure of a `Running` task:
    /// - If `attempts < max_retries`, increments `attempts` and returns it to `Ready` for retry.
    /// - Otherwise marks it `Failed`:
    ///   - If `allow_failure` is `true`, unlocks downstream dependencies as if satisfied.
    ///   - If `allow_failure` is `false`, cascades `Skipped` to all reachable unfinished downstream tasks.
    /// Returns the task's resulting `TaskState`, or `null` if `id` is out of bounds or not `Running`.
    fn failTask(*_, id: u32): TaskState? {
        if ((id as usize) >= _.task_count) {
            return null;
        }
        const idx = id as usize;
        if (_.tasks[idx].state != TaskState.Running) {
            return null;
        }

        if (_.tasks[idx].attempts < _.tasks[idx].max_retries) {
            _.tasks[idx].attempts += 1;
            _.tasks[idx].state = TaskState.Ready;
            return TaskState.Ready;
        }

        _.tasks[idx].attempts = satAddU8(_.tasks[idx].attempts, 1);
        _.tasks[idx].state = TaskState.Failed;

        if (_.tasks[idx].allow_failure) {
            _.unlockSuccessors(id);
            return TaskState.Failed;
        }

        // Zero-alloc fixed-point cascade: mark all reachable Pending/Ready descendants as Skipped.
        let changed: bool = false;
        let e: usize = 0;
        while (e < _.edge_count) {
            if (_.edges[e].from == id) {
                const dst = _.edges[e].to as usize;
                if (_.tasks[dst].state == TaskState.Pending || _.tasks[dst].state == TaskState.Ready) {
                    _.tasks[dst].state = TaskState.Skipped;
                    changed = true;
                }
            }
            e += 1;
        }

        while (changed) {
            changed = false;
            let k: usize = 0;
            while (k < _.edge_count) {
                const src = _.edges[k].from as usize;
                if (_.tasks[src].state == TaskState.Skipped) {
                    const dst = _.edges[k].to as usize;
                    if (_.tasks[dst].state == TaskState.Pending || _.tasks[dst].state == TaskState.Ready) {
                        _.tasks[dst].state = TaskState.Skipped;
                        changed = true;
                    }
                }
                k += 1;
            }
        }

        return TaskState.Failed;
    }

    /// Cancels all unfinished tasks (`Pending`, `Ready`, `Running`) and returns the number cancelled.
    fn cancelAllPending(*_): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.task_count) {
            match (_.tasks[i].state) {
                TaskState.Pending => {
                    _.tasks[i].state = TaskState.Cancelled;
                    count += 1;
                }
                TaskState.Ready => {
                    _.tasks[i].state = TaskState.Cancelled;
                    count += 1;
                }
                TaskState.Running => {
                    _.tasks[i].state = TaskState.Cancelled;
                    count += 1;
                }
                _ => {}
            }
            i += 1;
        }
        return count;
    }

    /// Returns the state of task `id`, or `null` if out of bounds.
    fn taskState(id: u32): TaskState? {
        if ((id as usize) >= _.task_count) {
            return null;
        }
        return _.tasks[id as usize].state;
    }

    /// Returns a copy of task `id`, or `null` if out of bounds.
    fn taskAt(id: u32): DagTask? {
        if ((id as usize) >= _.task_count) {
            return null;
        }
        return _.tasks[id as usize];
    }

    /// Computes a full state summary snapshot of all tasks in the DAG.
    fn summary(): DagSummary {
        let pending: usize = 0;
        let ready: usize = 0;
        let running: usize = 0;
        let completed: usize = 0;
        let failed: usize = 0;
        let skipped: usize = 0;
        let cancelled: usize = 0;

        let i: usize = 0;
        while (i < _.task_count) {
            match (_.tasks[i].state) {
                TaskState.Pending => pending += 1,
                TaskState.Ready => ready += 1,
                TaskState.Running => running += 1,
                TaskState.Completed => completed += 1,
                TaskState.Failed => failed += 1,
                TaskState.Skipped => skipped += 1,
                TaskState.Cancelled => cancelled += 1,
            }
            i += 1;
        }

        return DagSummary {
            pending: pending,
            ready: ready,
            running: running,
            completed: completed,
            failed: failed,
            skipped: skipped,
            cancelled: cancelled,
            is_done: pending == 0 && ready == 0 && running == 0,
        };
    }
}
