// Origin: Pure-CP Stdlib (Zero-Alloc A2A Multi-Agent Task DAG Scheduler & Failure-Propagation Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation A2A multi-agent sub-task DAG dependency scheduler:
//   - Fixed-capacity task nodes (`TASK_GRAPH_MAX_NODES = 64`) and directed dependency edges
//     (`TASK_GRAPH_MAX_EDGES = 128`) with iterative cycle detection on every edge insertion
//   - Priority-ordered concurrency-bounded wave dispatch (`collectReadyTasks`)
//   - Iterative failure-propagation cascade (`markFailed` -> `skipped_dep_failed`) respecting
//     per-node `allow_partial_fail` graceful degradation boundaries, plus `cancelSubtree`
//   - Real-time `TaskGraphSummary` progress rollup & terminal completion classification

// ============================================================================
// Constants & Models
// ============================================================================

pub const TASK_GRAPH_MAX_NODES: usize = 64;
pub const TASK_GRAPH_MAX_EDGES: usize = 128;

/// Execution lifecycle state of a sub-task node in `A2aTaskGraph`.
pub enum TaskGraphNodeState {
    pending,
    ready,
    running,
    succeeded,
    failed,
    skipped_dep_failed,
    cancelled,
}

/// Overall execution status of the entire DAG.
pub enum TaskGraphStatus {
    in_progress,
    completed_ok,
    completed_partial,
    failed,
}

/// Operation status returned by DAG mutation methods.
pub enum TaskGraphOpStatus {
    ok,
    duplicate_task_id,
    task_not_found,
    capacity_full,
    self_loop,
    cycle_detected,
    duplicate_edge,
    invalid_transition,
}

/// Single sub-task node assigned to a peer agent.
pub struct TaskGraphNode {
    task_id: u64 = 0;
    assigned_agent_id: u32 = 0;
    priority: u32 = 0;
    state: TaskGraphNodeState = TaskGraphNodeState.pending;
    allow_partial_fail: bool = false;
    occupied: bool = false;
}

/// Directed dependency edge (`dep_idx` must complete before `target_idx`).
pub struct TaskGraphEdge {
    dep_idx: u16 = 0;
    target_idx: u16 = 0;
    occupied: bool = false;
}

/// Aggregate progress counts and overall DAG completion status.
pub struct TaskGraphSummary {
    total: usize = 0;
    pending: usize = 0;
    ready: usize = 0;
    running: usize = 0;
    succeeded: usize = 0;
    failed: usize = 0;
    skipped_dep_failed: usize = 0;
    cancelled: usize = 0;
    overall: TaskGraphStatus = TaskGraphStatus.completed_ok;
}

fn isTerminalState(st: TaskGraphNodeState): bool {
    return st == TaskGraphNodeState.succeeded
        || st == TaskGraphNodeState.failed
        || st == TaskGraphNodeState.skipped_dep_failed
        || st == TaskGraphNodeState.cancelled;
}

fn isHigherPriorityTask(a: TaskGraphNode, b: TaskGraphNode): bool {
    if (a.priority > b.priority) return true;
    if (a.priority < b.priority) return false;
    return a.task_id < b.task_id;
}

// ============================================================================
// Zero-Alloc A2A Multi-Agent Task Graph Scheduler
// ============================================================================

/// Zero-heap-allocation multi-agent sub-task DAG dependency scheduler and failure-propagation engine.
pub struct A2aTaskGraph {
    nodes: [TASK_GRAPH_MAX_NODES]TaskGraphNode = [TaskGraphNode {}; TASK_GRAPH_MAX_NODES];
    edges: [TASK_GRAPH_MAX_EDGES]TaskGraphEdge = [TaskGraphEdge {}; TASK_GRAPH_MAX_EDGES];
    node_count: usize = 0;
    edge_count: usize = 0;

    static fn init(): A2aTaskGraph {
        return A2aTaskGraph {};
    }

    fn nodeCount(): usize {
        return _.node_count;
    }

    fn edgeCount(): usize {
        return _.edge_count;
    }

    fn findNodeIndex(task_id: u64): usize? {
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            if (_.nodes[i].occupied && _.nodes[i].task_id == task_id) {
                return i;
            }
        }
        return null;
    }

    /// Returns a copy of the task node matching `task_id`, or `null` if unknown.
    fn getTask(task_id: u64): TaskGraphNode? {
        if (idx := _.findNodeIndex(task_id)) {
            return _.nodes[idx];
        }
        return null;
    }

    /// Iterative BFS reachability check: returns `true` if `dst_idx` is reachable from `src_idx`.
    fn isReachable(src_idx: usize, dst_idx: usize): bool {
        let visited: [TASK_GRAPH_MAX_NODES]bool = [false; TASK_GRAPH_MAX_NODES];
        let queue: [TASK_GRAPH_MAX_NODES]usize = [0; TASK_GRAPH_MAX_NODES];
        let head: usize = 0;
        let tail: usize = 0;

        visited[src_idx] = true;
        queue[tail] = src_idx;
        tail += 1;

        while (head < tail) {
            const cur: usize = queue[head];
            head += 1;
            if (cur == dst_idx) {
                return true;
            }
            let e: usize = 0;
            while (e < _.edge_count) : (e += 1) {
                if (!_.edges[e].occupied) continue;
                if ((_.edges[e].dep_idx as usize) == cur) {
                    const nxt: usize = _.edges[e].target_idx as usize;
                    if (!visited[nxt]) {
                        visited[nxt] = true;
                        queue[tail] = nxt;
                        tail += 1;
                    }
                }
            }
        }
        return false;
    }

    /// Re-evaluates whether `node_idx` (when `pending` or `ready`) should be `ready`, `pending`,
    /// or `skipped_dep_failed` based on its incoming dependencies.
    fn refreshNodeReadiness(*_, node_idx: usize) {
        const st = _.nodes[node_idx].state;
        if (st != TaskGraphNodeState.pending && st != TaskGraphNodeState.ready) {
            return;
        }

        let all_terminal: bool = true;
        let any_dep_failed: bool = false;

        let e: usize = 0;
        while (e < _.edge_count) : (e += 1) {
            if (!_.edges[e].occupied) continue;
            if ((_.edges[e].target_idx as usize) == node_idx) {
                const dep_st = _.nodes[_.edges[e].dep_idx as usize].state;
                if (!isTerminalState(dep_st)) {
                    all_terminal = false;
                } else if (dep_st != TaskGraphNodeState.succeeded) {
                    any_dep_failed = true;
                }
            }
        }

        if (any_dep_failed && !_.nodes[node_idx].allow_partial_fail) {
            _.nodes[node_idx].state = TaskGraphNodeState.skipped_dep_failed;
            return;
        }
        if (all_terminal) {
            _.nodes[node_idx].state = TaskGraphNodeState.ready;
        } else {
            _.nodes[node_idx].state = TaskGraphNodeState.pending;
        }
    }

    /// Iterative BFS failure propagation from `failed_idx` to all downstream non-terminal nodes.
    fn propagateFailureFrom(*_, failed_idx: usize) {
        let visited: [TASK_GRAPH_MAX_NODES]bool = [false; TASK_GRAPH_MAX_NODES];
        let queue: [TASK_GRAPH_MAX_NODES]usize = [0; TASK_GRAPH_MAX_NODES];
        let head: usize = 0;
        let tail: usize = 0;

        visited[failed_idx] = true;
        queue[tail] = failed_idx;
        tail += 1;

        while (head < tail) {
            const cur: usize = queue[head];
            head += 1;

            let e: usize = 0;
            while (e < _.edge_count) : (e += 1) {
                if (!_.edges[e].occupied) continue;
                if ((_.edges[e].dep_idx as usize) == cur) {
                    const nxt: usize = _.edges[e].target_idx as usize;
                    const nxt_st = _.nodes[nxt].state;
                    if (nxt_st == TaskGraphNodeState.pending || nxt_st == TaskGraphNodeState.ready) {
                        if (!_.nodes[nxt].allow_partial_fail) {
                            _.nodes[nxt].state = TaskGraphNodeState.skipped_dep_failed;
                            if (!visited[nxt]) {
                                visited[nxt] = true;
                                queue[tail] = nxt;
                                tail += 1;
                            }
                        } else {
                            // Partial-fail tolerant node: check if all its dependencies are now terminal.
                            _.refreshNodeReadiness(nxt);
                        }
                    }
                }
            }
        }
    }

    /// Adds a new sub-task node to the DAG. Starts in `ready` state until dependencies are attached.
    fn addTask(
        *_,
        task_id: u64,
        assigned_agent_id: u32,
        priority: u32,
        allow_partial_fail: bool,
    ): TaskGraphOpStatus {
        if (_.findNodeIndex(task_id) != null) {
            return TaskGraphOpStatus.duplicate_task_id;
        }
        if (_.node_count >= TASK_GRAPH_MAX_NODES) {
            return TaskGraphOpStatus.capacity_full;
        }
        _.nodes[_.node_count] = TaskGraphNode {
            task_id: task_id,
            assigned_agent_id: assigned_agent_id,
            priority: priority,
            state: TaskGraphNodeState.ready,
            allow_partial_fail: allow_partial_fail,
            occupied: true,
        };
        _.node_count += 1;
        return TaskGraphOpStatus.ok;
    }

    /// Adds a directed dependency edge `dep_task_id -> target_task_id` with iterative cycle detection.
    /// Rejects self-loops, unknown tasks, duplicate edges, full edge table, or cycles.
    fn addDependency(*_, dep_task_id: u64, target_task_id: u64): TaskGraphOpStatus {
        if (dep_task_id == target_task_id) {
            return TaskGraphOpStatus.self_loop;
        }
        const dep_idx = _.findNodeIndex(dep_task_id) ?? return TaskGraphOpStatus.task_not_found;
        const tgt_idx = _.findNodeIndex(target_task_id) ?? return TaskGraphOpStatus.task_not_found;

        if (_.edge_count >= TASK_GRAPH_MAX_EDGES) {
            return TaskGraphOpStatus.capacity_full;
        }

        // Check duplicate edge.
        let e: usize = 0;
        while (e < _.edge_count) : (e += 1) {
            if (_.edges[e].occupied
                && (_.edges[e].dep_idx as usize) == dep_idx
                && (_.edges[e].target_idx as usize) == tgt_idx)
            {
                return TaskGraphOpStatus.duplicate_edge;
            }
        }

        // Cycle check: adding `dep -> tgt` creates a cycle iff `dep` is reachable from `tgt`.
        if (_.isReachable(tgt_idx, dep_idx)) {
            return TaskGraphOpStatus.cycle_detected;
        }

        _.edges[_.edge_count] = TaskGraphEdge {
            dep_idx: dep_idx as u16,
            target_idx: tgt_idx as u16,
            occupied: true,
        };
        _.edge_count += 1;

        _.refreshNodeReadiness(tgt_idx);
        if (_.nodes[tgt_idx].state == TaskGraphNodeState.skipped_dep_failed) {
            _.propagateFailureFrom(tgt_idx);
        }
        return TaskGraphOpStatus.ok;
    }

    /// Collects up to `max_concurrency - current_running` ready tasks ordered by descending
    /// `priority` (tie-broken by ascending `task_id`) into `out_task_ids`.
    fn collectReadyTasks(max_concurrency: usize, out_task_ids: *[]u64): usize {
        if (max_concurrency == 0 || out_task_ids.len == 0) {
            return 0;
        }

        let running_count: usize = 0;
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            if (_.nodes[i].occupied && _.nodes[i].state == TaskGraphNodeState.running) {
                running_count += 1;
            }
        }
        if (running_count >= max_concurrency) {
            return 0;
        }

        const avail_slots: usize = max_concurrency - running_count;
        const limit: usize = if (avail_slots < out_task_ids.len) avail_slots else out_task_ids.len;

        let used: [TASK_GRAPH_MAX_NODES]bool = [false; TASK_GRAPH_MAX_NODES];
        let written: usize = 0;
        while (written < limit) : (written += 1) {
            let best: usize = _.node_count;
            let j: usize = 0;
            while (j < _.node_count) : (j += 1) {
                if (used[j] || !_.nodes[j].occupied || _.nodes[j].state != TaskGraphNodeState.ready) {
                    continue;
                }
                if (best == _.node_count || isHigherPriorityTask(_.nodes[j], _.nodes[best])) {
                    best = j;
                }
            }
            if (best == _.node_count) {
                break;
            }
            used[best] = true;
            out_task_ids[written] = _.nodes[best].task_id;
        }
        return written;
    }

    /// Transitions a `ready` task into `running`.
    fn markRunning(*_, task_id: u64): TaskGraphOpStatus {
        const idx = _.findNodeIndex(task_id) ?? return TaskGraphOpStatus.task_not_found;
        if (_.nodes[idx].state != TaskGraphNodeState.ready) {
            return TaskGraphOpStatus.invalid_transition;
        }
        _.nodes[idx].state = TaskGraphNodeState.running;
        return TaskGraphOpStatus.ok;
    }

    /// Marks a `running` task as `succeeded` and promotes newly unblocked downstream tasks to `ready`.
    fn markSucceeded(*_, task_id: u64): TaskGraphOpStatus {
        const idx = _.findNodeIndex(task_id) ?? return TaskGraphOpStatus.task_not_found;
        if (_.nodes[idx].state != TaskGraphNodeState.running) {
            return TaskGraphOpStatus.invalid_transition;
        }
        _.nodes[idx].state = TaskGraphNodeState.succeeded;

        let e: usize = 0;
        while (e < _.edge_count) : (e += 1) {
            if (!_.edges[e].occupied) continue;
            if ((_.edges[e].dep_idx as usize) == idx) {
                _.refreshNodeReadiness(_.edges[e].target_idx as usize);
            }
        }
        return TaskGraphOpStatus.ok;
    }

    /// Marks a `running` task as `failed` and iteratively cascades `skipped_dep_failed` to
    /// downstream nodes that do not set `allow_partial_fail`.
    fn markFailed(*_, task_id: u64): TaskGraphOpStatus {
        const idx = _.findNodeIndex(task_id) ?? return TaskGraphOpStatus.task_not_found;
        if (_.nodes[idx].state != TaskGraphNodeState.running) {
            return TaskGraphOpStatus.invalid_transition;
        }
        _.nodes[idx].state = TaskGraphNodeState.failed;
        _.propagateFailureFrom(idx);
        return TaskGraphOpStatus.ok;
    }

    /// Iteratively cancels `root_task_id` and all non-terminal downstream tasks reachable from it.
    /// Returns the number of tasks transitioned to `cancelled`.
    fn cancelSubtree(*_, root_task_id: u64): usize {
        const root_idx = _.findNodeIndex(root_task_id) ?? return 0;

        let visited: [TASK_GRAPH_MAX_NODES]bool = [false; TASK_GRAPH_MAX_NODES];
        let queue: [TASK_GRAPH_MAX_NODES]usize = [0; TASK_GRAPH_MAX_NODES];
        let head: usize = 0;
        let tail: usize = 0;
        let cancelled_n: usize = 0;

        visited[root_idx] = true;
        queue[tail] = root_idx;
        tail += 1;

        while (head < tail) {
            const cur: usize = queue[head];
            head += 1;
            if (!isTerminalState(_.nodes[cur].state)) {
                _.nodes[cur].state = TaskGraphNodeState.cancelled;
                cancelled_n += 1;
            }
            let e: usize = 0;
            while (e < _.edge_count) : (e += 1) {
                if (!_.edges[e].occupied) continue;
                if ((_.edges[e].dep_idx as usize) == cur) {
                    const nxt: usize = _.edges[e].target_idx as usize;
                    if (!visited[nxt]) {
                        visited[nxt] = true;
                        queue[tail] = nxt;
                        tail += 1;
                    }
                }
            }
        }
        return cancelled_n;
    }

    /// Computes live progress counts and overall DAG completion status.
    fn summary(): TaskGraphSummary {
        let sum = TaskGraphSummary {
            total: _.node_count,
        };
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            const st = _.nodes[i].state;
            if (st == TaskGraphNodeState.pending) {
                sum.pending += 1;
            } else if (st == TaskGraphNodeState.ready) {
                sum.ready += 1;
            } else if (st == TaskGraphNodeState.running) {
                sum.running += 1;
            } else if (st == TaskGraphNodeState.succeeded) {
                sum.succeeded += 1;
            } else if (st == TaskGraphNodeState.failed) {
                sum.failed += 1;
            } else if (st == TaskGraphNodeState.skipped_dep_failed) {
                sum.skipped_dep_failed += 1;
            } else {
                sum.cancelled += 1;
            }
        }

        if (sum.pending > 0 || sum.ready > 0 || sum.running > 0) {
            sum.overall = TaskGraphStatus.in_progress;
        } else if (sum.succeeded == sum.total) {
            sum.overall = TaskGraphStatus.completed_ok;
        } else if (sum.succeeded > 0) {
            sum.overall = TaskGraphStatus.completed_partial;
        } else {
            sum.overall = TaskGraphStatus.failed;
        }
        return sum;
    }

    /// Resets all tasks and edges.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            _.nodes[i] = TaskGraphNode {};
        }
        let e: usize = 0;
        while (e < _.edge_count) : (e += 1) {
            _.edges[e] = TaskGraphEdge {};
        }
        _.node_count = 0;
        _.edge_count = 0;
    }
}
