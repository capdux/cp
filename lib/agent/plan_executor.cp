// Origin: Pure-CP Stdlib (Zero-Heap Hierarchical Plan-Execute-Replan AI Agent Task Tree Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation hierarchical Plan-Execute-Replan state machine for long-horizon AI Agents:
//   1. `PLAN_NO_PARENT`, `PlanStepState`, `PlanStepNode`, `PlanProgress`
//   2. `PlanExecutor`:
//      - Hierarchical task tree (`parent_id`) + explicit step ordering (`depends_on_step_id`)
//      - Deterministic leaf selection (`nextReadyStep` by ascending `step_id`)
//      - Automatic transient failure retry (`attempts < max_attempts` -> back to `Pending`/`Ready`)
//      - Permanent failure cascade (`Skipped` on downstream dependents) & bottom-up parent roll-up
//      - Dynamic replanning (`replanAppendStep` + cycle-checked `rebindDependency`) to recover mid-flight

/// Sentinel ID indicating no parent node or no prerequisite dependency step.
pub const PLAN_NO_PARENT: u32 = 0xFFFF_FFFF;

/// Execution lifecycle state of a single step in a hierarchical agent plan.
pub enum PlanStepState {
    Pending,
    Ready,
    Running,
    Succeeded,
    Failed,
    Skipped,
}

/// Single step node in the hierarchical task tree stored in caller-provided buffer.
pub struct PlanStepNode {
    active: bool = false;
    step_id: u32 = 0;
    parent_id: u32 = PLAN_NO_PARENT;
    depends_on_step_id: u32 = PLAN_NO_PARENT;
    title: string = "";
    state: PlanStepState = PlanStepState.Pending;
    attempts: u32 = 0;
    max_attempts: u32 = 1;
    result_note: string = "";
}

/// Aggregated progress and terminal status snapshot for `PlanExecutor`.
pub struct PlanProgress {
    total_steps: usize = 0;
    pending_steps: usize = 0;
    ready_steps: usize = 0;
    running_steps: usize = 0;
    succeeded_steps: usize = 0;
    failed_steps: usize = 0;
    skipped_steps: usize = 0;
    remaining_steps: usize = 0;
    completion_ratio: f64 = 0.0;
    replan_count: u32 = 0;

    /// Returns `true` if the plan has at least one step and zero remaining (`Pending`/`Ready`/`Running`) steps.
    fn isDone(): bool {
        return _.total_steps > 0 && _.remaining_steps == 0;
    }

    /// Returns `true` if the plan has at least one step and every step reached `Succeeded`.
    fn isAllSucceeded(): bool {
        return _.total_steps > 0 && _.succeeded_steps == _.total_steps;
    }
}

/// Zero-heap-allocation hierarchical Plan-Execute-Replan task tree execution engine.
pub struct PlanExecutor {
    nodes: []PlanStepNode;
    step_count: usize = 0;
    replan_count: u32 = 0;

    /// Initializes an empty plan executor over caller-provided `nodes_buf`.
    /// Returns `null` if `nodes_buf.len == 0`.
    static fn init(nodes_buf: *[]PlanStepNode): PlanExecutor? {
        if (nodes_buf.len == 0) return null;
        let i: usize = 0;
        while (i < nodes_buf.len) : (i += 1) {
            nodes_buf[i] = PlanStepNode{};
        }
        return PlanExecutor{
            nodes: nodes_buf,
            step_count: 0,
            replan_count: 0,
        };
    }

    /// Internal lookup returning the buffer index for an active `step_id`.
    fn findIndex(step_id: u32): usize? {
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (_.nodes[i].active && _.nodes[i].step_id == step_id) {
                return i;
            }
        }
        return null;
    }

    /// Returns `true` if any active step has `parent_id == step_id`.
    fn hasChildren(step_id: u32): bool {
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (_.nodes[i].active && _.nodes[i].parent_id == step_id) {
                return true;
            }
        }
        return false;
    }

    /// Checks whether a leaf node's own dependency and its entire ancestor chain's dependencies are satisfied.
    fn arePrerequisitesMet(idx: usize): bool {
        let cur_idx: usize = idx;
        let hops: usize = 0;
        while (hops <= _.nodes.len) : (hops += 1) {
            const node: PlanStepNode = _.nodes[cur_idx];
            if (node.depends_on_step_id != PLAN_NO_PARENT) {
                const dep_idx: usize = _.findIndex(node.depends_on_step_id) ?? return false;
                if (_.nodes[dep_idx].state != PlanStepState.Succeeded) {
                    return false;
                }
            }
            if (node.parent_id == PLAN_NO_PARENT) {
                return true;
            }
            const p_idx: usize = _.findIndex(node.parent_id) ?? return false;
            const p_state: PlanStepState = _.nodes[p_idx].state;
            if (p_state == PlanStepState.Failed || p_state == PlanStepState.Skipped) {
                return false;
            }
            cur_idx = p_idx;
        }
        return false;
    }

    /// Fixed-point reconciliation:
    ///   1. Bottom-up parent container roll-up from children states.
    ///   2. Downstream `Skipped` cascade when prerequisite dependency or parent is `Failed`/`Skipped`.
    ///   3. `Pending <-> Ready` refresh for runnable leaf steps.
    fn refreshStates(*_) {
        let iter: usize = 0;
        while (iter <= _.nodes.len) : (iter += 1) {
            let changed: bool = false;

            // 1. Cascade Skipped to any Pending/Ready node whose dependency or parent failed/skipped
            let i: usize = 0;
            while (i < _.nodes.len) : (i += 1) {
                if (!_.nodes[i].active) continue;
                const st: PlanStepState = _.nodes[i].state;
                if (st != PlanStepState.Pending && st != PlanStepState.Ready) continue;

                let should_skip: bool = false;
                if (_.nodes[i].depends_on_step_id != PLAN_NO_PARENT) {
                    if (dep_i := _.findIndex(_.nodes[i].depends_on_step_id)) {
                        const ds: PlanStepState = _.nodes[dep_i].state;
                        if (ds == PlanStepState.Failed || ds == PlanStepState.Skipped) {
                            should_skip = true;
                        }
                    }
                }
                if (!should_skip && _.nodes[i].parent_id != PLAN_NO_PARENT) {
                    if (par_i := _.findIndex(_.nodes[i].parent_id)) {
                        const ps: PlanStepState = _.nodes[par_i].state;
                        if (ps == PlanStepState.Skipped) {
                            should_skip = true;
                        }
                    }
                }

                if (should_skip) {
                    _.nodes[i].state = PlanStepState.Skipped;
                    changed = true;
                }
            }

            // 2. Roll up non-leaf parent nodes from their direct children
            i = 0;
            while (i < _.nodes.len) : (i += 1) {
                if (!_.nodes[i].active) continue;
                const pid: u32 = _.nodes[i].step_id;

                let child_total: usize = 0;
                let child_active: usize = 0;
                let child_succ: usize = 0;
                let child_fail: usize = 0;
                let child_skip: usize = 0;

                let j: usize = 0;
                while (j < _.nodes.len) : (j += 1) {
                    if (!_.nodes[j].active || _.nodes[j].parent_id != pid) continue;
                    child_total += 1;
                    const cs: PlanStepState = _.nodes[j].state;
                    if (cs == PlanStepState.Succeeded) {
                        child_succ += 1;
                    } else if (cs == PlanStepState.Failed) {
                        child_fail += 1;
                    } else if (cs == PlanStepState.Skipped) {
                        child_skip += 1;
                    } else {
                        child_active += 1;
                    }
                }

                if (child_total == 0) continue;

                let desired: PlanStepState = PlanStepState.Pending;
                if (child_active > 0) {
                    desired = PlanStepState.Pending;
                } else if (child_succ == child_total) {
                    desired = PlanStepState.Succeeded;
                } else if (child_fail > 0) {
                    desired = PlanStepState.Failed;
                } else if (child_skip == child_total) {
                    desired = PlanStepState.Skipped;
                } else {
                    desired = PlanStepState.Failed;
                }

                if (_.nodes[i].state != desired) {
                    _.nodes[i].state = desired;
                    changed = true;
                }
            }

            if (!changed) break;
        }

        // 3. Mark runnable leaf steps as Ready, non-runnable leaves as Pending
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (!_.nodes[i].active) continue;
            const st: PlanStepState = _.nodes[i].state;
            if (st != PlanStepState.Pending && st != PlanStepState.Ready) continue;

            if (!_.hasChildren(_.nodes[i].step_id) && _.arePrerequisitesMet(i)) {
                _.nodes[i].state = PlanStepState.Ready;
            } else {
                _.nodes[i].state = PlanStepState.Pending;
            }
        }
    }

    /// Adds a step to the hierarchical task tree.
    /// Rejects `step_id == PLAN_NO_PARENT`, `max_attempts == 0`, duplicate `step_id`,
    /// self-parent/self-dependency, non-existent `parent_id`/`depends_on_step_id`,
    /// attaching a child to a `Running` parent, or a full node buffer.
    fn addStep(
        *_,
        step_id: u32,
        parent_id: u32,
        depends_on_step_id: u32,
        title: string,
        max_attempts: u32,
    ): bool {
        if (step_id == PLAN_NO_PARENT || max_attempts == 0) return false;
        if (parent_id == step_id || depends_on_step_id == step_id) return false;
        if (_.step_count >= _.nodes.len) return false;
        if (_.findIndex(step_id) != null) return false;

        if (parent_id != PLAN_NO_PARENT) {
            const p_idx: usize = _.findIndex(parent_id) ?? return false;
            if (_.nodes[p_idx].state == PlanStepState.Running) return false;
        }
        if (depends_on_step_id != PLAN_NO_PARENT) {
            if (_.findIndex(depends_on_step_id) == null) return false;
        }

        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (!_.nodes[i].active) {
                free_idx = i;
                break;
            }
        }
        const slot: usize = free_idx ?? return false;

        _.nodes[slot] = PlanStepNode{
            active: true,
            step_id: step_id,
            parent_id: parent_id,
            depends_on_step_id: depends_on_step_id,
            title: title,
            state: PlanStepState.Pending,
            attempts: 0,
            max_attempts: max_attempts,
            result_note: "",
        };
        _.step_count += 1;
        _.refreshStates();
        return true;
    }

    /// Dynamically injects a recovery/replan step during execution and increments `replan_count`.
    /// If attached to a `Failed` or `Skipped` parent, that parent is automatically re-opened to `Pending`.
    fn replanAppendStep(
        *_,
        step_id: u32,
        parent_id: u32,
        depends_on_step_id: u32,
        title: string,
        max_attempts: u32,
    ): bool {
        if (!_.addStep(step_id, parent_id, depends_on_step_id, title, max_attempts)) {
            return false;
        }
        _.replan_count += 1;
        return true;
    }

    /// Rebinds a `Skipped`, `Pending`, or `Ready` step's prerequisite dependency onto `new_depends_on_step_id`
    /// (e.g., pointing a previously skipped downstream step at a newly injected recovery step).
    /// Rejects missing nodes, terminal `Succeeded`/`Failed`/`Running` target steps, or dependency cycles.
    fn rebindDependency(*_, step_id: u32, new_depends_on_step_id: u32): bool {
        const idx: usize = _.findIndex(step_id) ?? return false;
        if (new_depends_on_step_id == step_id) return false;

        const st: PlanStepState = _.nodes[idx].state;
        if (st != PlanStepState.Skipped && st != PlanStepState.Pending && st != PlanStepState.Ready) {
            return false;
        }

        if (new_depends_on_step_id != PLAN_NO_PARENT) {
            if (_.findIndex(new_depends_on_step_id) == null) return false;
            // Cycle guard: ensure new_depends_on_step_id does not transitively depend on step_id
            let cur: u32 = new_depends_on_step_id;
            let hops: usize = 0;
            while (cur != PLAN_NO_PARENT && hops <= _.nodes.len) : (hops += 1) {
                if (cur == step_id) return false;
                const c_idx: usize = _.findIndex(cur) ?? break;
                cur = _.nodes[c_idx].depends_on_step_id;
            }
        }

        _.nodes[idx].depends_on_step_id = new_depends_on_step_id;
        _.nodes[idx].state = PlanStepState.Pending;
        _.replan_count += 1;
        _.refreshStates();
        return true;
    }

    /// Returns the smallest `step_id` among all executable leaf steps whose prerequisites are `Succeeded`.
    /// Returns `null` if no leaf step is currently ready.
    fn nextReadyStep(): u32? {
        let best_id: u32? = null;
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            const n: PlanStepNode = _.nodes[i];
            if (!n.active) continue;
            if (n.state != PlanStepState.Ready && n.state != PlanStepState.Pending) continue;
            if (_.hasChildren(n.step_id)) continue;
            if (!_.arePrerequisitesMet(i)) continue;

            if (cur_best := best_id) {
                if (n.step_id < cur_best) {
                    best_id = n.step_id;
                }
            } else {
                best_id = n.step_id;
            }
        }
        return best_id;
    }

    /// Transitions a ready executable leaf step into `Running` and increments its `attempts` counter.
    fn startStep(*_, step_id: u32): bool {
        const idx: usize = _.findIndex(step_id) ?? return false;
        const st: PlanStepState = _.nodes[idx].state;
        if (st != PlanStepState.Ready && st != PlanStepState.Pending) return false;
        if (_.hasChildren(step_id)) return false;
        if (!_.arePrerequisitesMet(idx)) return false;

        _.nodes[idx].state = PlanStepState.Running;
        _.nodes[idx].attempts += 1;
        return true;
    }

    /// Completes a currently `Running` step with `succeeded` and `note`:
    /// - On success: marks `Succeeded`, rolls up parent containers, and unlocks downstream steps.
    /// - On failure with `attempts < max_attempts`: returns step to `Pending`/`Ready` for automatic retry.
    /// - On permanent failure (`attempts >= max_attempts`): marks `Failed`, cascades `Skipped` to
    ///   downstream dependent steps, and rolls up parent containers.
    fn completeStep(*_, step_id: u32, succeeded: bool, note: string): bool {
        const idx: usize = _.findIndex(step_id) ?? return false;
        if (_.nodes[idx].state != PlanStepState.Running) return false;

        _.nodes[idx].result_note = note;
        if (succeeded) {
            _.nodes[idx].state = PlanStepState.Succeeded;
        } else if (_.nodes[idx].attempts < _.nodes[idx].max_attempts) {
            _.nodes[idx].state = PlanStepState.Pending;
        } else {
            _.nodes[idx].state = PlanStepState.Failed;
        }
        _.refreshStates();
        return true;
    }

    /// Looks up a step node by `step_id`.
    fn getStep(step_id: u32): PlanStepNode? {
        const idx: usize = _.findIndex(step_id) ?? return null;
        return _.nodes[idx];
    }

    /// Computes a full progress summary including completion ratio and terminal state predicates.
    fn progress(): PlanProgress {
        let p = PlanProgress{
            total_steps: _.step_count,
            pending_steps: 0,
            ready_steps: 0,
            running_steps: 0,
            succeeded_steps: 0,
            failed_steps: 0,
            skipped_steps: 0,
            remaining_steps: 0,
            completion_ratio: 0.0,
            replan_count: _.replan_count,
        };

        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            const n: PlanStepNode = _.nodes[i];
            if (!n.active) continue;
            if (n.state == PlanStepState.Pending) {
                p.pending_steps += 1;
            } else if (n.state == PlanStepState.Ready) {
                p.ready_steps += 1;
            } else if (n.state == PlanStepState.Running) {
                p.running_steps += 1;
            } else if (n.state == PlanStepState.Succeeded) {
                p.succeeded_steps += 1;
            } else if (n.state == PlanStepState.Failed) {
                p.failed_steps += 1;
            } else {
                p.skipped_steps += 1;
            }
        }

        p.remaining_steps = p.pending_steps + p.ready_steps + p.running_steps;
        const done: usize = p.succeeded_steps + p.failed_steps + p.skipped_steps;
        if (p.total_steps > 0) {
            p.completion_ratio = (done as f64) / (p.total_steps as f64);
        }
        return p;
    }
}
