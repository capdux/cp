// Origin: Pure-CP Stdlib (Zero-Alloc Hierarchical Multi-Agent Budget Tree & Cascading Stop-Loss) | Created: 2026-11-22
// Pure CP zero-heap-allocation hierarchical budget delegation & cascading stop-loss tree for multi-agent systems:
//   - Fixed inline 64-node delegation tree (`root -> sub-agent -> tool/worker`, 0 heap allocation)
//   - Enforces triple hard limits (`max_tokens`, `max_cost_micro_usd`, `max_wall_ms`; `0` = unlimited)
//     tracking both `self_*` and `subtree_*` consumption plus `hard_reserved` child reservations
//   - Safe quota delegation (`spawnChildBudget` in `shared_pool` or `hard_reserved` mode)
//   - Atomic iterative ancestor-chain spend bubbling (`recordSpend`) with exact dimension & trigger-node reporting
//   - Unused reservation reclamation (`closeNodeAndReleaseUnused`) & iterative cascading cancellation (`cancelSubtree`)

// ============================================================================
// 1. Constants & Public Enums
// ============================================================================

/// Maximum number of nodes supported inline in a single `AgentBudgetTree`.
pub const BUDGET_TREE_MAX_NODES: usize = 64;

/// Sentinel parent / node identifier representing no parent or invalid node.
pub const BUDGET_TREE_NO_PARENT: u16 = 0xFFFF;

/// Lifecycle state of a budget node.
pub enum BudgetNodeState {
    active,
    closed,
    cancelled,
    exhausted,
}

/// Quota delegation policy for a child budget node relative to its parent.
pub enum BudgetAllocationMode {
    shared_pool,
    hard_reserved,
}

/// Specific resource dimension that breached a hard limit.
pub enum BudgetExceededDimension {
    none,
    tokens,
    cost_micro_usd,
    wall_ms,
}

/// Operation status returned by `AgentBudgetTree` methods.
pub enum BudgetTreeStatus {
    ok,
    node_not_found,
    node_inactive,
    tree_full,
    parent_insufficient_quota,
    exceeded_tokens,
    exceeded_cost,
    exceeded_wall_ms,
}

// ============================================================================
// 2. Public Data Models
// ============================================================================

/// Triple hard resource limits (`0` indicates unlimited on that dimension).
pub struct BudgetTreeLimits {
    max_tokens: u64 = 0;
    max_cost_micro_usd: u64 = 0;
    max_wall_ms: u64 = 0;
}

/// Single node inside `AgentBudgetTree`.
pub struct BudgetTreeNode {
    occupied: bool = false;
    node_id: u16 = 0;
    parent_id: u16 = BUDGET_TREE_NO_PARENT;
    depth: u16 = 0;
    state: BudgetNodeState = BudgetNodeState.active;
    alloc_mode: BudgetAllocationMode = BudgetAllocationMode.shared_pool;
    limits: BudgetTreeLimits = BudgetTreeLimits{};

    self_tokens: u64 = 0;
    self_cost_micro_usd: u64 = 0;
    self_wall_ms: u64 = 0;

    subtree_tokens: u64 = 0;
    subtree_cost_micro_usd: u64 = 0;
    subtree_wall_ms: u64 = 0;

    reserved_tokens: u64 = 0;
    reserved_cost_micro_usd: u64 = 0;
    reserved_wall_ms: u64 = 0;

    /// Remaining unallocated & unspent token capacity (`null` if unlimited).
    fn remainingTokens(): u64? {
        if (_.limits.max_tokens == 0) return null;
        const committed = satAddU64(_.subtree_tokens, _.reserved_tokens);
        return satSubU64(_.limits.max_tokens, committed);
    }

    /// Remaining unallocated & unspent micro-USD cost capacity (`null` if unlimited).
    fn remainingCostMicroUsd(): u64? {
        if (_.limits.max_cost_micro_usd == 0) return null;
        const committed = satAddU64(_.subtree_cost_micro_usd, _.reserved_cost_micro_usd);
        return satSubU64(_.limits.max_cost_micro_usd, committed);
    }

    /// Remaining unallocated & unspent wall-clock millisecond capacity (`null` if unlimited).
    fn remainingWallMs(): u64? {
        if (_.limits.max_wall_ms == 0) return null;
        const committed = satAddU64(_.subtree_wall_ms, _.reserved_wall_ms);
        return satSubU64(_.limits.max_wall_ms, committed);
    }
}

/// Result returned by `AgentBudgetTree.spawnChildBudget`.
pub struct BudgetSpawnResult {
    status: BudgetTreeStatus = BudgetTreeStatus.ok;
    child_id: u16 = BUDGET_TREE_NO_PARENT;
}

/// Result returned by `AgentBudgetTree.recordSpend`.
pub struct BudgetSpendResult {
    status: BudgetTreeStatus = BudgetTreeStatus.ok;
    exceeded_dim: BudgetExceededDimension = BudgetExceededDimension.none;
    triggered_node_id: u16 = BUDGET_TREE_NO_PARENT;
    node_subtree_tokens: u64 = 0;
    node_subtree_cost_micro_usd: u64 = 0;
    node_subtree_wall_ms: u64 = 0;
}

/// Tree-wide telemetry snapshot returned by `AgentBudgetTree.stats`.
pub struct BudgetTreeStats {
    total_nodes: usize = 0;
    active_nodes: usize = 0;
    closed_nodes: usize = 0;
    cancelled_nodes: usize = 0;
    exhausted_nodes: usize = 0;
    root_subtree_tokens: u64 = 0;
    root_subtree_cost_micro_usd: u64 = 0;
    root_subtree_wall_ms: u64 = 0;
}

// ============================================================================
// 3. Internal Saturating Math Helpers
// ============================================================================

const U64_MAX: u64 = 0xFFFFFFFFFFFFFFFF;

fn satAddU64(a: u64, b: u64): u64 {
    if (a > U64_MAX - b) return U64_MAX;
    return a + b;
}

fn satSubU64(a: u64, b: u64): u64 {
    if (a <= b) return 0;
    return a - b;
}

fn minU64(a: u64, b: u64): u64 {
    return if (a < b) a else b;
}

// ============================================================================
// 4. Zero-Alloc Hierarchical Multi-Agent Budget Tree
// ============================================================================

/// Zero-heap-allocation hierarchical budget delegation tree with atomic ancestor spend
/// bubbling, hard-reservation reclamation, and iterative cascading stop-loss.
pub struct AgentBudgetTree {
    nodes: [BUDGET_TREE_MAX_NODES]BudgetTreeNode = [BudgetTreeNode{}; BUDGET_TREE_MAX_NODES];
    count: usize = 0;

    /// Initializes a new budget tree with root node `0` carrying `root_limits`.
    static fn init(root_limits: BudgetTreeLimits): AgentBudgetTree {
        let tree = AgentBudgetTree{};
        tree.nodes[0] = BudgetTreeNode{
            occupied: true,
            node_id: 0,
            parent_id: BUDGET_TREE_NO_PARENT,
            depth: 0,
            state: BudgetNodeState.active,
            alloc_mode: BudgetAllocationMode.shared_pool,
            limits: root_limits,
        };
        tree.count = 1;
        return tree;
    }

    /// Returns a value snapshot of `node_id`, or `null` if out of bounds / unoccupied.
    fn getNode(node_id: u16): BudgetTreeNode? {
        const idx: usize = node_id as usize;
        if (idx >= _.count || !_.nodes[idx].occupied) {
            return null;
        }
        return _.nodes[idx];
    }

    /// Returns `true` iff `node_id` and every ancestor up to root are currently `active`.
    fn isChainActive(node_id: u16): bool {
        let cur: u16 = node_id;
        let guard: usize = 0;
        while (cur != BUDGET_TREE_NO_PARENT && guard < BUDGET_TREE_MAX_NODES) : (guard += 1) {
            const idx: usize = cur as usize;
            if (idx >= _.count || !_.nodes[idx].occupied) {
                return false;
            }
            if (_.nodes[idx].state != BudgetNodeState.active) {
                return false;
            }
            cur = _.nodes[idx].parent_id;
        }
        return true;
    }

    /// Returns `true` iff `candidate_id` is `ancestor_id` or a descendant of `ancestor_id`.
    fn isInSubtree(candidate_id: u16, ancestor_id: u16): bool {
        let cur: u16 = candidate_id;
        let guard: usize = 0;
        while (cur != BUDGET_TREE_NO_PARENT && guard < BUDGET_TREE_MAX_NODES) : (guard += 1) {
            if (cur == ancestor_id) return true;
            const idx: usize = cur as usize;
            if (idx >= _.count || !_.nodes[idx].occupied) return false;
            cur = _.nodes[idx].parent_id;
        }
        return false;
    }

    /// Spawns a child budget node under `parent_id` with `child_limits` and `alloc_mode`.
    /// Validates that `child_limits` fit within `parent_id`'s remaining quota on every bounded dimension.
    fn spawnChildBudget(
        *_,
        parent_id: u16,
        child_limits: BudgetTreeLimits,
        alloc_mode: BudgetAllocationMode,
    ): BudgetSpawnResult {
        const p_idx: usize = parent_id as usize;
        if (p_idx >= _.count || !_.nodes[p_idx].occupied) {
            return BudgetSpawnResult{ status: BudgetTreeStatus.node_not_found };
        }
        if (!_.isChainActive(parent_id)) {
            return BudgetSpawnResult{ status: BudgetTreeStatus.node_inactive };
        }
        if (_.count >= BUDGET_TREE_MAX_NODES) {
            return BudgetSpawnResult{ status: BudgetTreeStatus.tree_full };
        }

        const p = _.nodes[p_idx];
        // Validate each bounded parent dimension against child limits.
        if (rem_tok := p.remainingTokens()) {
            if (alloc_mode == BudgetAllocationMode.hard_reserved && child_limits.max_tokens == 0) {
                return BudgetSpawnResult{ status: BudgetTreeStatus.parent_insufficient_quota };
            }
            if (child_limits.max_tokens > rem_tok) {
                return BudgetSpawnResult{ status: BudgetTreeStatus.parent_insufficient_quota };
            }
        }
        if (rem_cost := p.remainingCostMicroUsd()) {
            if (alloc_mode == BudgetAllocationMode.hard_reserved && child_limits.max_cost_micro_usd == 0) {
                return BudgetSpawnResult{ status: BudgetTreeStatus.parent_insufficient_quota };
            }
            if (child_limits.max_cost_micro_usd > rem_cost) {
                return BudgetSpawnResult{ status: BudgetTreeStatus.parent_insufficient_quota };
            }
        }
        if (rem_wall := p.remainingWallMs()) {
            if (alloc_mode == BudgetAllocationMode.hard_reserved && child_limits.max_wall_ms == 0) {
                return BudgetSpawnResult{ status: BudgetTreeStatus.parent_insufficient_quota };
            }
            if (child_limits.max_wall_ms > rem_wall) {
                return BudgetSpawnResult{ status: BudgetTreeStatus.parent_insufficient_quota };
            }
        }

        // Apply hard reservation on parent if requested.
        if (alloc_mode == BudgetAllocationMode.hard_reserved) {
            _.nodes[p_idx].reserved_tokens = satAddU64(_.nodes[p_idx].reserved_tokens, child_limits.max_tokens);
            _.nodes[p_idx].reserved_cost_micro_usd = satAddU64(_.nodes[p_idx].reserved_cost_micro_usd, child_limits.max_cost_micro_usd);
            _.nodes[p_idx].reserved_wall_ms = satAddU64(_.nodes[p_idx].reserved_wall_ms, child_limits.max_wall_ms);
        }

        const child_id: u16 = _.count as u16;
        _.nodes[_.count] = BudgetTreeNode{
            occupied: true,
            node_id: child_id,
            parent_id: parent_id,
            depth: p.depth + 1,
            state: BudgetNodeState.active,
            alloc_mode: alloc_mode,
            limits: child_limits,
        };
        _.count += 1;

        return BudgetSpawnResult{
            status: BudgetTreeStatus.ok,
            child_id: child_id,
        };
    }

    /// Atomically validates and bubbles `(tokens, cost_micro_usd, wall_ms)` up the ancestor chain.
    /// If any node along the path would breach its hard limit, marks that node `exhausted`,
    /// leaves spend counters unmodified, and returns the exact `exceeded_dim` and `triggered_node_id`.
    fn recordSpend(
        *_,
        node_id: u16,
        tokens: u64,
        cost_micro_usd: u64,
        wall_ms: u64,
    ): BudgetSpendResult {
        const start_idx: usize = node_id as usize;
        if (start_idx >= _.count || !_.nodes[start_idx].occupied) {
            return BudgetSpendResult{ status: BudgetTreeStatus.node_not_found };
        }
        if (!_.isChainActive(node_id)) {
            return BudgetSpendResult{ status: BudgetTreeStatus.node_inactive };
        }

        // Phase 1: Dry-run validation across the entire ancestor chain (node -> ... -> root).
        let cur: u16 = node_id;
        let prev_was_reserved: bool = false;
        let guard: usize = 0;
        while (cur != BUDGET_TREE_NO_PARENT && guard < BUDGET_TREE_MAX_NODES) : (guard += 1) {
            const idx: usize = cur as usize;
            const nd = _.nodes[idx];

            const next_sub_tok = satAddU64(nd.subtree_tokens, tokens);
            const next_res_tok = if (prev_was_reserved) satSubU64(nd.reserved_tokens, minU64(nd.reserved_tokens, tokens)) else nd.reserved_tokens;
            if (nd.limits.max_tokens > 0 && satAddU64(next_sub_tok, next_res_tok) > nd.limits.max_tokens) {
                _.nodes[idx].state = BudgetNodeState.exhausted;
                return BudgetSpendResult{
                    status: BudgetTreeStatus.exceeded_tokens,
                    exceeded_dim: BudgetExceededDimension.tokens,
                    triggered_node_id: cur,
                    node_subtree_tokens: _.nodes[start_idx].subtree_tokens,
                    node_subtree_cost_micro_usd: _.nodes[start_idx].subtree_cost_micro_usd,
                    node_subtree_wall_ms: _.nodes[start_idx].subtree_wall_ms,
                };
            }

            const next_sub_cost = satAddU64(nd.subtree_cost_micro_usd, cost_micro_usd);
            const next_res_cost = if (prev_was_reserved) satSubU64(nd.reserved_cost_micro_usd, minU64(nd.reserved_cost_micro_usd, cost_micro_usd)) else nd.reserved_cost_micro_usd;
            if (nd.limits.max_cost_micro_usd > 0 && satAddU64(next_sub_cost, next_res_cost) > nd.limits.max_cost_micro_usd) {
                _.nodes[idx].state = BudgetNodeState.exhausted;
                return BudgetSpendResult{
                    status: BudgetTreeStatus.exceeded_cost,
                    exceeded_dim: BudgetExceededDimension.cost_micro_usd,
                    triggered_node_id: cur,
                    node_subtree_tokens: _.nodes[start_idx].subtree_tokens,
                    node_subtree_cost_micro_usd: _.nodes[start_idx].subtree_cost_micro_usd,
                    node_subtree_wall_ms: _.nodes[start_idx].subtree_wall_ms,
                };
            }

            const next_sub_wall = satAddU64(nd.subtree_wall_ms, wall_ms);
            const next_res_wall = if (prev_was_reserved) satSubU64(nd.reserved_wall_ms, minU64(nd.reserved_wall_ms, wall_ms)) else nd.reserved_wall_ms;
            if (nd.limits.max_wall_ms > 0 && satAddU64(next_sub_wall, next_res_wall) > nd.limits.max_wall_ms) {
                _.nodes[idx].state = BudgetNodeState.exhausted;
                return BudgetSpendResult{
                    status: BudgetTreeStatus.exceeded_wall_ms,
                    exceeded_dim: BudgetExceededDimension.wall_ms,
                    triggered_node_id: cur,
                    node_subtree_tokens: _.nodes[start_idx].subtree_tokens,
                    node_subtree_cost_micro_usd: _.nodes[start_idx].subtree_cost_micro_usd,
                    node_subtree_wall_ms: _.nodes[start_idx].subtree_wall_ms,
                };
            }

            prev_was_reserved = (nd.alloc_mode == BudgetAllocationMode.hard_reserved);
            cur = nd.parent_id;
        }

        // Phase 2: Commit spend to `node_id` and bubble up to root.
        _.nodes[start_idx].self_tokens = satAddU64(_.nodes[start_idx].self_tokens, tokens);
        _.nodes[start_idx].self_cost_micro_usd = satAddU64(_.nodes[start_idx].self_cost_micro_usd, cost_micro_usd);
        _.nodes[start_idx].self_wall_ms = satAddU64(_.nodes[start_idx].self_wall_ms, wall_ms);

        cur = node_id;
        prev_was_reserved = false;
        guard = 0;
        while (cur != BUDGET_TREE_NO_PARENT && guard < BUDGET_TREE_MAX_NODES) : (guard += 1) {
            const idx: usize = cur as usize;
            _.nodes[idx].subtree_tokens = satAddU64(_.nodes[idx].subtree_tokens, tokens);
            _.nodes[idx].subtree_cost_micro_usd = satAddU64(_.nodes[idx].subtree_cost_micro_usd, cost_micro_usd);
            _.nodes[idx].subtree_wall_ms = satAddU64(_.nodes[idx].subtree_wall_ms, wall_ms);

            if (prev_was_reserved) {
                _.nodes[idx].reserved_tokens = satSubU64(_.nodes[idx].reserved_tokens, minU64(_.nodes[idx].reserved_tokens, tokens));
                _.nodes[idx].reserved_cost_micro_usd = satSubU64(_.nodes[idx].reserved_cost_micro_usd, minU64(_.nodes[idx].reserved_cost_micro_usd, cost_micro_usd));
                _.nodes[idx].reserved_wall_ms = satSubU64(_.nodes[idx].reserved_wall_ms, minU64(_.nodes[idx].reserved_wall_ms, wall_ms));
            }

            prev_was_reserved = (_.nodes[idx].alloc_mode == BudgetAllocationMode.hard_reserved);
            cur = _.nodes[idx].parent_id;
        }

        return BudgetSpendResult{
            status: BudgetTreeStatus.ok,
            exceeded_dim: BudgetExceededDimension.none,
            triggered_node_id: BUDGET_TREE_NO_PARENT,
            node_subtree_tokens: _.nodes[start_idx].subtree_tokens,
            node_subtree_cost_micro_usd: _.nodes[start_idx].subtree_cost_micro_usd,
            node_subtree_wall_ms: _.nodes[start_idx].subtree_wall_ms,
        };
    }

    /// Internal helper: releases any unused `hard_reserved` quota from `node_idx` back to its parent.
    fn releaseUnusedReservationToParent(*_, node_idx: usize) {
        const nd = _.nodes[node_idx];
        if (nd.alloc_mode != BudgetAllocationMode.hard_reserved || nd.parent_id == BUDGET_TREE_NO_PARENT) {
            return;
        }
        const p_idx: usize = nd.parent_id as usize;
        if (p_idx >= _.count || !_.nodes[p_idx].occupied) {
            return;
        }

        const unused_tok = satSubU64(nd.limits.max_tokens, nd.subtree_tokens);
        const unused_cost = satSubU64(nd.limits.max_cost_micro_usd, nd.subtree_cost_micro_usd);
        const unused_wall = satSubU64(nd.limits.max_wall_ms, nd.subtree_wall_ms);

        _.nodes[p_idx].reserved_tokens = satSubU64(_.nodes[p_idx].reserved_tokens, minU64(_.nodes[p_idx].reserved_tokens, unused_tok));
        _.nodes[p_idx].reserved_cost_micro_usd = satSubU64(_.nodes[p_idx].reserved_cost_micro_usd, minU64(_.nodes[p_idx].reserved_cost_micro_usd, unused_cost));
        _.nodes[p_idx].reserved_wall_ms = satSubU64(_.nodes[p_idx].reserved_wall_ms, minU64(_.nodes[p_idx].reserved_wall_ms, unused_wall));
    }

    /// Marks `node_id` as `closed` and returns any unused `hard_reserved` quota back to its parent.
    fn closeNodeAndReleaseUnused(*_, node_id: u16): BudgetTreeStatus {
        const idx: usize = node_id as usize;
        if (idx >= _.count || !_.nodes[idx].occupied) {
            return BudgetTreeStatus.node_not_found;
        }
        if (_.nodes[idx].state != BudgetNodeState.active && _.nodes[idx].state != BudgetNodeState.exhausted) {
            return BudgetTreeStatus.node_inactive;
        }
        _.releaseUnusedReservationToParent(idx);
        _.nodes[idx].state = BudgetNodeState.closed;
        return BudgetTreeStatus.ok;
    }

    /// Iteratively cancels `node_id` and all active/exhausted descendants (zero recursion),
    /// returning unused `hard_reserved` quotas to their parent and reporting total cancelled count.
    fn cancelSubtree(*_, node_id: u16): usize {
        const root_idx: usize = node_id as usize;
        if (root_idx >= _.count || !_.nodes[root_idx].occupied) {
            return 0;
        }

        let cancelled_count: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (!_.nodes[i].occupied) continue;
            if (!_.isInSubtree(i as u16, node_id)) continue;
            if (_.nodes[i].state == BudgetNodeState.active || _.nodes[i].state == BudgetNodeState.exhausted) {
                _.releaseUnusedReservationToParent(i);
                _.nodes[i].state = BudgetNodeState.cancelled;
                cancelled_count += 1;
            }
        }
        return cancelled_count;
    }

    /// Returns a tree-wide telemetry snapshot.
    fn stats(): BudgetTreeStats {
        let st = BudgetTreeStats{
            total_nodes: _.count,
        };
        if (_.count > 0 && _.nodes[0].occupied) {
            st.root_subtree_tokens = _.nodes[0].subtree_tokens;
            st.root_subtree_cost_micro_usd = _.nodes[0].subtree_cost_micro_usd;
            st.root_subtree_wall_ms = _.nodes[0].subtree_wall_ms;
        }
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (!_.nodes[i].occupied) continue;
            const s = _.nodes[i].state;
            if (s == BudgetNodeState.active) {
                st.active_nodes += 1;
            } else if (s == BudgetNodeState.closed) {
                st.closed_nodes += 1;
            } else if (s == BudgetNodeState.cancelled) {
                st.cancelled_nodes += 1;
            } else if (s == BudgetNodeState.exhausted) {
                st.exhausted_nodes += 1;
            }
        }
        return st;
    }
}
