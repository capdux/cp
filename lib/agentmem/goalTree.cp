// Origin: Pure-CP Stdlib (AI Agent Zero-Alloc Hierarchical Goal Tree & HTN Plan Engine) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation hierarchical goal decomposition & HTN plan status tree:
//   - Fixed-capacity inline tree (`GOAL_TREE_MAX_NODES = 64`, max depth `16`) supporting
//     `leaf`, `op_and` (all children), `op_or` (any alternative), and `op_seq` (ordered gate)
//   - Purely iterative bottom-up status & `progress_bps` (`0..=10000`) propagation with zero recursion:
//       * `op_and`: achieved when all achieved, failed if any child fails
//       * `op_or`: achieved when any child achieved (unstarted siblings auto-marked `skipped`),
//                  failed only when all children fail
//       * `op_seq`: unlocks children strictly in insertion order (later children stay `blocked`
//                   until predecessors achieve)
//   - Priority-ordered actionable leaf scheduler (`pickNextActionable` / `collectActionable`)
//     respecting `SEQ` predecessor gates and `OR` fallback semantics + bounded retries (`retryNode`)

/// Maximum number of goal nodes stored inline inside a single `GoalTree` (zero dynamic heap).
pub const GOAL_TREE_MAX_NODES: usize = 64;

/// Maximum allowed hierarchy depth (root is depth 0; children must satisfy `depth < GOAL_TREE_MAX_DEPTH`).
pub const GOAL_TREE_MAX_DEPTH: u8 = 16;

/// Maximum byte length of a goal node short name.
pub const GOAL_TREE_NAME_MAX_LEN: usize = 48;

/// Sentinel index representing no parent/child/sibling link.
pub const GOAL_NO_NODE: u16 = 0xFFFF;

/// Full completion progress in basis points (`100.00%`).
pub const GOAL_PROGRESS_FULL_BPS: u16 = 10000;

// ============================================================================
// 1. Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Decomposition operator for a goal tree node.
pub enum GoalOperator: u8 {
    leaf = 0,
    op_and = 1,
    op_or = 2,
    op_seq = 3,
}

/// Execution lifecycle status of a goal node.
pub enum GoalStatus: u8 {
    pending = 0,
    blocked = 1,
    active = 2,
    achieved = 3,
    failed = 4,
    skipped = 5,
}

/// Single inline goal node stored inside `GoalTree`.
pub struct GoalNode {
    occupied: bool = false;
    node_id: u32 = 0;
    parent_idx: u16 = GOAL_NO_NODE;
    first_child_idx: u16 = GOAL_NO_NODE;
    last_child_idx: u16 = GOAL_NO_NODE;
    next_sibling_idx: u16 = GOAL_NO_NODE;
    child_count: u16 = 0;
    depth: u8 = 0;
    operator: GoalOperator = GoalOperator.leaf;
    status: GoalStatus = GoalStatus.pending;
    priority: u16 = 0;
    retry_count: u16 = 0;
    max_retries: u16 = 0;
    /// Completion progress in basis points (`0..=10000`).
    progress_bps: u16 = 0;
    goal_hash: u64 = 0;
    name_buf: [GOAL_TREE_NAME_MAX_LEN]u8 = [0; GOAL_TREE_NAME_MAX_LEN];
    name_len: usize = 0;

    /// Returns the short goal name as a zero-copy read-only slice.
    fn nameSlice(): []const u8 {
        const n: usize = if (_.name_len <= GOAL_TREE_NAME_MAX_LEN) _.name_len else GOAL_TREE_NAME_MAX_LEN;
        return _.name_buf[0..n];
    }

    /// Returns `true` if this node is an executable atomic leaf goal.
    fn isLeaf(): bool {
        return _.operator == GoalOperator.leaf;
    }

    /// Returns `true` if this node is in a terminal state (`achieved`, `failed`, or `skipped`).
    fn isTerminal(): bool {
        return _.status == GoalStatus.achieved
            || _.status == GoalStatus.failed
            || _.status == GoalStatus.skipped;
    }

    /// Returns `true` if this failed node has remaining retry budget.
    fn canRetry(): bool {
        return _.status == GoalStatus.failed && _.retry_count < _.max_retries;
    }
}

/// Zero-heap telemetry snapshot of a `GoalTree`.
pub struct GoalTreeStats {
    capacity: usize = GOAL_TREE_MAX_NODES;
    total_nodes: usize = 0;
    leaf_nodes: usize = 0;
    pending_nodes: usize = 0;
    blocked_nodes: usize = 0;
    active_nodes: usize = 0;
    achieved_nodes: usize = 0;
    failed_nodes: usize = 0;
    skipped_nodes: usize = 0;
    max_depth: u8 = 0;
    root_progress_bps: u16 = 0;
    total_transitions: u64 = 0;
    total_retries: u64 = 0;
}

fn copyNameBytes(dst: *[]u8, src: []const u8): usize {
    const n: usize = if (src.len <= dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) {
        dst[i] = src[i];
        i += 1;
    }
    return n;
}

// ============================================================================
// 2. Hierarchical Goal Tree Engine (100% Iterative, Zero Recursion)
// ============================================================================

/// Zero-heap hierarchical goal decomposition & HTN plan status tree engine.
pub struct GoalTree {
    nodes: [GOAL_TREE_MAX_NODES]GoalNode = [GoalNode{}; GOAL_TREE_MAX_NODES];
    node_count: usize = 0;
    root_idx: u16 = GOAL_NO_NODE;
    next_id: u32 = 1;
    total_transitions: u64 = 0;
    total_retries: u64 = 0;

    /// Initializes an empty `GoalTree`.
    static fn init(): GoalTree {
        return GoalTree{};
    }

    /// Resets the entire goal tree and clears all counters.
    fn reset(*_) {
        let i: usize = 0;
        while (i < GOAL_TREE_MAX_NODES) {
            _.nodes[i] = GoalNode{};
            i += 1;
        }
        _.node_count = 0;
        _.root_idx = GOAL_NO_NODE;
        _.next_id = 1;
        _.total_transitions = 0;
        _.total_retries = 0;
    }

    fn findIdxById(node_id: u32): u16? {
        if (node_id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.node_count && i < GOAL_TREE_MAX_NODES) {
            if (_.nodes[i].occupied && _.nodes[i].node_id == node_id) {
                return @as(u16, @intCast(i));
            }
            i += 1;
        }
        return null;
    }

    /// Returns a value copy of the goal node with `node_id`, or `null` if not found.
    fn getNode(node_id: u32): GoalNode? {
        const idx = _.findIdxById(node_id) ?? return null;
        return _.nodes[@as(usize, idx)];
    }

    /// Returns the root goal's `node_id`, or `null` if no root has been created.
    fn rootId(): u32? {
        if (_.root_idx == GOAL_NO_NODE || @as(usize, _.root_idx) >= _.node_count) {
            return null;
        }
        return _.nodes[@as(usize, _.root_idx)].node_id;
    }

    /// Returns the root goal's current `GoalStatus`, or `null` if empty.
    fn rootStatus(): GoalStatus? {
        if (_.root_idx == GOAL_NO_NODE || @as(usize, _.root_idx) >= _.node_count) {
            return null;
        }
        return _.nodes[@as(usize, _.root_idx)].status;
    }

    /// Returns the root goal's completion progress in basis points (`0..=10000`), or `0` if empty.
    fn rootProgressBps(): u16 {
        if (_.root_idx == GOAL_NO_NODE || @as(usize, _.root_idx) >= _.node_count) {
            return 0;
        }
        return _.nodes[@as(usize, _.root_idx)].progress_bps;
    }

    /// Sets the root goal node. Returns its assigned `node_id`, or `null` if root already exists or name invalid.
    fn setRoot(
        *_,
        name: []const u8,
        goal_hash: u64,
        operator: GoalOperator,
        priority: u16,
        max_retries: u16,
    ): u32? {
        if (_.root_idx != GOAL_NO_NODE || _.node_count >= GOAL_TREE_MAX_NODES) {
            return null;
        }
        if (name.len == 0 || name.len > GOAL_TREE_NAME_MAX_LEN) {
            return null;
        }

        const idx: usize = _.node_count;
        const id: u32 = _.next_id;
        _.next_id += 1;

        let node = GoalNode{
            occupied: true,
            node_id: id,
            parent_idx: GOAL_NO_NODE,
            depth: 0,
            operator: operator,
            status: GoalStatus.pending,
            priority: priority,
            max_retries: max_retries,
            goal_hash: goal_hash,
        };
        let dst: []u8 = node.name_buf[0..];
        node.name_len = copyNameBytes(*dst, name);

        _.nodes[idx] = node;
        _.root_idx = @as(u16, @intCast(idx));
        _.node_count += 1;
        return id;
    }

    /// Appends a child goal under a composite (`op_and` / `op_or` / `op_seq`) parent node.
    /// Returns the new child's `node_id`, or `null` on invalid parent, leaf parent, depth/capacity overflow, or bad name.
    fn addChild(
        *_,
        parent_id: u32,
        name: []const u8,
        goal_hash: u64,
        operator: GoalOperator,
        priority: u16,
        max_retries: u16,
    ): u32? {
        if (_.node_count >= GOAL_TREE_MAX_NODES) {
            return null;
        }
        if (name.len == 0 || name.len > GOAL_TREE_NAME_MAX_LEN) {
            return null;
        }
        const p_idx = _.findIdxById(parent_id) ?? return null;
        const p_u: usize = @as(usize, p_idx);

        // Leaf nodes cannot have children.
        if (_.nodes[p_u].operator == GoalOperator.leaf) {
            return null;
        }
        // Depth guard.
        if (_.nodes[p_u].depth + 1 >= GOAL_TREE_MAX_DEPTH) {
            return null;
        }

        // Determine initial status based on parent status & SEQ ordering.
        let init_status: GoalStatus = GoalStatus.pending;
        if (_.nodes[p_u].status == GoalStatus.achieved || _.nodes[p_u].status == GoalStatus.skipped) {
            init_status = GoalStatus.skipped;
        } else if (_.nodes[p_u].operator == GoalOperator.op_seq) {
            if (_.nodes[p_u].child_count > 0) {
                const last_u: usize = @as(usize, _.nodes[p_u].last_child_idx);
                if (_.nodes[last_u].status != GoalStatus.achieved) {
                    init_status = GoalStatus.blocked;
                }
            }
        }

        const idx: usize = _.node_count;
        const idx_u16: u16 = @as(u16, @intCast(idx));
        const id: u32 = _.next_id;
        _.next_id += 1;

        let node = GoalNode{
            occupied: true,
            node_id: id,
            parent_idx: p_idx,
            depth: _.nodes[p_u].depth + 1,
            operator: operator,
            status: init_status,
            priority: priority,
            max_retries: max_retries,
            goal_hash: goal_hash,
        };
        let dst: []u8 = node.name_buf[0..];
        node.name_len = copyNameBytes(*dst, name);

        _.nodes[idx] = node;
        _.node_count += 1;

        // Link into parent's intrusive singly-linked child list (preserving insertion order).
        if (_.nodes[p_u].first_child_idx == GOAL_NO_NODE) {
            _.nodes[p_u].first_child_idx = idx_u16;
            _.nodes[p_u].last_child_idx = idx_u16;
        } else {
            const tail_u: usize = @as(usize, _.nodes[p_u].last_child_idx);
            _.nodes[tail_u].next_sibling_idx = idx_u16;
            _.nodes[p_u].last_child_idx = idx_u16;
        }
        _.nodes[p_u].child_count += 1;

        _.propagateUp(p_idx);
        return id;
    }

    /// Purely iterative subtree skip: marks `pending` / `blocked` nodes inside `root_sub_idx` as `skipped`.
    fn skipUnstartedSubtree(*_, root_sub_idx: u16) {
        let stack: [GOAL_TREE_MAX_NODES]u16 = [GOAL_NO_NODE; GOAL_TREE_MAX_NODES];
        let sp: usize = 0;
        stack[sp] = root_sub_idx;
        sp += 1;

        while (sp > 0) {
            sp -= 1;
            const cur_idx: u16 = stack[sp];
            if (cur_idx == GOAL_NO_NODE || @as(usize, cur_idx) >= _.node_count) {
                continue;
            }
            const cur_u: usize = @as(usize, cur_idx);
            if (_.nodes[cur_u].status == GoalStatus.pending || _.nodes[cur_u].status == GoalStatus.blocked) {
                _.nodes[cur_u].status = GoalStatus.skipped;
                _.nodes[cur_u].progress_bps = 0;
            }
            let c: u16 = _.nodes[cur_u].first_child_idx;
            while (c != GOAL_NO_NODE && sp < GOAL_TREE_MAX_NODES) {
                stack[sp] = c;
                sp += 1;
                c = _.nodes[@as(usize, c)].next_sibling_idx;
            }
        }
    }

    /// Purely iterative bottom-up recomputation of status, SEQ gating, OR skipping, and `progress_bps`.
    fn propagateUp(*_, start_idx: u16) {
        let cur: u16 = start_idx;
        let guard: usize = 0;
        while (cur != GOAL_NO_NODE && @as(usize, cur) < _.node_count && guard < GOAL_TREE_MAX_NODES) {
            guard += 1;
            const u: usize = @as(usize, cur);
            const op: GoalOperator = _.nodes[u].operator;

            if (op == GoalOperator.leaf) {
                cur = _.nodes[u].parent_idx;
                continue;
            }

            let total: u32 = 0;
            let achieved_cnt: u32 = 0;
            let failed_cnt: u32 = 0;
            let active_cnt: u32 = 0;
            let sum_progress: u32 = 0;
            let max_progress: u16 = 0;

            if (op == GoalOperator.op_seq) {
                // First pass: update SEQ predecessor blocking/unblocking in exact insertion order.
                let prev_achieved: bool = true;
                let c_seq: u16 = _.nodes[u].first_child_idx;
                while (c_seq != GOAL_NO_NODE && @as(usize, c_seq) < _.node_count) {
                    const cu: usize = @as(usize, c_seq);
                    if (prev_achieved) {
                        if (_.nodes[cu].status == GoalStatus.blocked) {
                            _.nodes[cu].status = GoalStatus.pending;
                        }
                        if (_.nodes[cu].status != GoalStatus.achieved) {
                            prev_achieved = false;
                        }
                    } else {
                        if (_.nodes[cu].status == GoalStatus.pending) {
                            _.nodes[cu].status = GoalStatus.blocked;
                        }
                    }
                    c_seq = _.nodes[cu].next_sibling_idx;
                }
            }

            let c: u16 = _.nodes[u].first_child_idx;
            while (c != GOAL_NO_NODE && @as(usize, c) < _.node_count) {
                const cu: usize = @as(usize, c);
                total += 1;
                const st: GoalStatus = _.nodes[cu].status;
                if (st == GoalStatus.achieved) {
                    achieved_cnt += 1;
                    sum_progress += @as(u32, GOAL_PROGRESS_FULL_BPS);
                    if (GOAL_PROGRESS_FULL_BPS > max_progress) {
                        max_progress = GOAL_PROGRESS_FULL_BPS;
                    }
                } else if (st == GoalStatus.failed) {
                    failed_cnt += 1;
                } else {
                    if (st == GoalStatus.active) {
                        active_cnt += 1;
                    }
                    sum_progress += @as(u32, _.nodes[cu].progress_bps);
                    if (_.nodes[cu].progress_bps > max_progress) {
                        max_progress = _.nodes[cu].progress_bps;
                    }
                }
                c = _.nodes[cu].next_sibling_idx;
            }

            if (op == GoalOperator.op_and || op == GoalOperator.op_seq) {
                if (total == 0) {
                    _.nodes[u].progress_bps = 0;
                    if (_.nodes[u].status != GoalStatus.blocked && _.nodes[u].status != GoalStatus.skipped) {
                        _.nodes[u].status = GoalStatus.pending;
                    }
                } else if (failed_cnt > 0) {
                    _.nodes[u].status = GoalStatus.failed;
                    _.nodes[u].progress_bps = @as(u16, @intCast(sum_progress / total));
                } else if (achieved_cnt == total) {
                    _.nodes[u].status = GoalStatus.achieved;
                    _.nodes[u].progress_bps = GOAL_PROGRESS_FULL_BPS;
                } else {
                    _.nodes[u].progress_bps = @as(u16, @intCast(sum_progress / total));
                    if (_.nodes[u].status != GoalStatus.blocked && _.nodes[u].status != GoalStatus.skipped) {
                        if (active_cnt > 0 || achieved_cnt > 0) {
                            _.nodes[u].status = GoalStatus.active;
                        } else {
                            _.nodes[u].status = GoalStatus.pending;
                        }
                    }
                }
            } else if (op == GoalOperator.op_or) {
                if (total == 0) {
                    _.nodes[u].progress_bps = 0;
                    if (_.nodes[u].status != GoalStatus.blocked && _.nodes[u].status != GoalStatus.skipped) {
                        _.nodes[u].status = GoalStatus.pending;
                    }
                } else if (achieved_cnt > 0) {
                    _.nodes[u].status = GoalStatus.achieved;
                    _.nodes[u].progress_bps = GOAL_PROGRESS_FULL_BPS;
                    // Auto-skip all unstarted (`pending` / `blocked`) alternative branches.
                    let c_skip: u16 = _.nodes[u].first_child_idx;
                    while (c_skip != GOAL_NO_NODE && @as(usize, c_skip) < _.node_count) {
                        const csu: usize = @as(usize, c_skip);
                        if (_.nodes[csu].status == GoalStatus.pending || _.nodes[csu].status == GoalStatus.blocked) {
                            _.skipUnstartedSubtree(c_skip);
                        }
                        c_skip = _.nodes[csu].next_sibling_idx;
                    }
                } else if (failed_cnt == total) {
                    _.nodes[u].status = GoalStatus.failed;
                    _.nodes[u].progress_bps = 0;
                } else {
                    _.nodes[u].progress_bps = max_progress;
                    if (_.nodes[u].status != GoalStatus.blocked && _.nodes[u].status != GoalStatus.skipped) {
                        if (active_cnt > 0) {
                            _.nodes[u].status = GoalStatus.active;
                        } else {
                            _.nodes[u].status = GoalStatus.pending;
                        }
                    }
                }
            }

            cur = _.nodes[u].parent_idx;
        }
    }

    /// Checks iteratively whether all ancestors of `idx` allow execution (no blocked/terminal ancestor or SEQ gate violation).
    fn isAncestorChainRunnable(idx: u16): bool {
        let child: u16 = idx;
        let parent: u16 = _.nodes[@as(usize, idx)].parent_idx;
        let guard: usize = 0;
        while (parent != GOAL_NO_NODE && @as(usize, parent) < _.node_count && guard < GOAL_TREE_MAX_NODES) {
            guard += 1;
            const pu: usize = @as(usize, parent);
            const pst: GoalStatus = _.nodes[pu].status;
            if (pst == GoalStatus.blocked
                || pst == GoalStatus.achieved
                || pst == GoalStatus.failed
                || pst == GoalStatus.skipped)
            {
                return false;
            }
            if (_.nodes[pu].operator == GoalOperator.op_seq) {
                // Verify all earlier siblings in this SEQ parent are achieved.
                let sib: u16 = _.nodes[pu].first_child_idx;
                while (sib != GOAL_NO_NODE && sib != child) {
                    if (_.nodes[@as(usize, sib)].status != GoalStatus.achieved) {
                        return false;
                    }
                    sib = _.nodes[@as(usize, sib)].next_sibling_idx;
                }
            }
            child = parent;
            parent = _.nodes[pu].parent_idx;
        }
        return true;
    }

    /// Marks an executable `pending` leaf goal as `active`. Returns `false` if blocked, terminal, or non-leaf.
    fn markActive(*_, node_id: u32): bool {
        const idx = _.findIdxById(node_id) ?? return false;
        const u: usize = @as(usize, idx);
        if (!_.nodes[u].isLeaf() || _.nodes[u].status != GoalStatus.pending) {
            return false;
        }
        if (!_.isAncestorChainRunnable(idx)) {
            return false;
        }
        _.nodes[u].status = GoalStatus.active;
        _.total_transitions += 1;
        _.propagateUp(_.nodes[u].parent_idx);
        return true;
    }

    /// Marks a `pending` or `active` leaf goal as `achieved` (`progress_bps = 10000`) and propagates up.
    fn markAchieved(*_, node_id: u32): bool {
        const idx = _.findIdxById(node_id) ?? return false;
        const u: usize = @as(usize, idx);
        if (!_.nodes[u].isLeaf()) {
            return false;
        }
        if (_.nodes[u].status != GoalStatus.pending && _.nodes[u].status != GoalStatus.active) {
            return false;
        }
        if (!_.isAncestorChainRunnable(idx)) {
            return false;
        }
        _.nodes[u].status = GoalStatus.achieved;
        _.nodes[u].progress_bps = GOAL_PROGRESS_FULL_BPS;
        _.total_transitions += 1;
        _.propagateUp(_.nodes[u].parent_idx);
        return true;
    }

    /// Marks a `pending` or `active` leaf goal as `failed` (`progress_bps = 0`) and propagates up.
    fn markFailed(*_, node_id: u32): bool {
        const idx = _.findIdxById(node_id) ?? return false;
        const u: usize = @as(usize, idx);
        if (!_.nodes[u].isLeaf()) {
            return false;
        }
        if (_.nodes[u].status != GoalStatus.pending && _.nodes[u].status != GoalStatus.active) {
            return false;
        }
        if (!_.isAncestorChainRunnable(idx)) {
            return false;
        }
        _.nodes[u].status = GoalStatus.failed;
        _.nodes[u].progress_bps = 0;
        _.total_transitions += 1;
        _.propagateUp(_.nodes[u].parent_idx);
        return true;
    }

    /// Retries a `failed` leaf goal if `retry_count < max_retries`, restoring it to `pending` and re-evaluating ancestors.
    fn retryNode(*_, node_id: u32): bool {
        const idx = _.findIdxById(node_id) ?? return false;
        const u: usize = @as(usize, idx);
        if (!_.nodes[u].isLeaf() || !_.nodes[u].canRetry()) {
            return false;
        }
        _.nodes[u].retry_count += 1;
        _.nodes[u].status = GoalStatus.pending;
        _.nodes[u].progress_bps = 0;
        _.total_retries += 1;
        _.total_transitions += 1;
        _.propagateUp(_.nodes[u].parent_idx);
        return true;
    }

    /// Picks the highest-priority currently actionable (`pending` & runnable) leaf goal `node_id`.
    /// Deterministic tie-break: higher `priority` first, then smaller `node_id`.
    fn pickNextActionable(): u32? {
        let best_id: u32 = 0;
        let best_prio: u16 = 0;
        let found: bool = false;

        let i: usize = 0;
        while (i < _.node_count && i < GOAL_TREE_MAX_NODES) {
            if (_.nodes[i].occupied
                && _.nodes[i].isLeaf()
                && _.nodes[i].status == GoalStatus.pending
                && _.isAncestorChainRunnable(@as(u16, @intCast(i))))
            {
                const prio: u16 = _.nodes[i].priority;
                const id: u32 = _.nodes[i].node_id;
                if (!found || prio > best_prio || (prio == best_prio && id < best_id)) {
                    found = true;
                    best_prio = prio;
                    best_id = id;
                }
            }
            i += 1;
        }

        if (!found) {
            return null;
        }
        return best_id;
    }

    /// Collects all currently actionable leaf goal IDs into `out_ids`, sorted by `(priority desc, node_id asc)`.
    fn collectActionable(out_ids: *[]u32): usize {
        let cands: [GOAL_TREE_MAX_NODES]usize = [0; GOAL_TREE_MAX_NODES];
        let count: usize = 0;

        let i: usize = 0;
        while (i < _.node_count && i < GOAL_TREE_MAX_NODES) {
            if (_.nodes[i].occupied
                && _.nodes[i].isLeaf()
                && _.nodes[i].status == GoalStatus.pending
                && _.isAncestorChainRunnable(@as(u16, @intCast(i))))
            {
                cands[count] = i;
                count += 1;
            }
            i += 1;
        }

        // Deterministic insertion sort by (priority desc, node_id asc).
        let s: usize = 1;
        while (s < count) {
            const key_idx: usize = cands[s];
            const key_prio: u16 = _.nodes[key_idx].priority;
            const key_id: u32 = _.nodes[key_idx].node_id;
            let j: usize = s;
            while (j > 0) {
                const prev_idx: usize = cands[j - 1];
                const prev_prio: u16 = _.nodes[prev_idx].priority;
                const prev_id: u32 = _.nodes[prev_idx].node_id;
                if (key_prio > prev_prio || (key_prio == prev_prio && key_id < prev_id)) {
                    cands[j] = cands[j - 1];
                    j -= 1;
                } else {
                    break;
                }
            }
            cands[j] = key_idx;
            s += 1;
        }

        const out_n: usize = if (count < out_ids.len) count else out_ids.len;
        let k: usize = 0;
        while (k < out_n) {
            out_ids[k] = _.nodes[cands[k]].node_id;
            k += 1;
        }
        return out_n;
    }

    /// Returns a zero-heap telemetry snapshot of the entire goal tree.
    fn stats(): GoalTreeStats {
        let st = GoalTreeStats{
            capacity: GOAL_TREE_MAX_NODES,
            total_nodes: _.node_count,
            root_progress_bps: _.rootProgressBps(),
            total_transitions: _.total_transitions,
            total_retries: _.total_retries,
        };

        let i: usize = 0;
        while (i < _.node_count && i < GOAL_TREE_MAX_NODES) {
            if (_.nodes[i].occupied) {
                if (_.nodes[i].isLeaf()) {
                    st.leaf_nodes += 1;
                }
                if (_.nodes[i].depth > st.max_depth) {
                    st.max_depth = _.nodes[i].depth;
                }
                const ns: GoalStatus = _.nodes[i].status;
                if (ns == GoalStatus.pending) {
                    st.pending_nodes += 1;
                } else if (ns == GoalStatus.blocked) {
                    st.blocked_nodes += 1;
                } else if (ns == GoalStatus.active) {
                    st.active_nodes += 1;
                } else if (ns == GoalStatus.achieved) {
                    st.achieved_nodes += 1;
                } else if (ns == GoalStatus.failed) {
                    st.failed_nodes += 1;
                } else if (ns == GoalStatus.skipped) {
                    st.skipped_nodes += 1;
                }
            }
            i += 1;
        }
        return st;
    }
}
