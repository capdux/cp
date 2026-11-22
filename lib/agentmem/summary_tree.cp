// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Hierarchical Conversation Summary Tree & Context Compactor) | Created: 2026-11-22
// Pure CP zero-heap-allocation hierarchical summary tree and token-budget context frontier planner
// for long-horizon AI Agents (MemGPT / Letta style progressive context compaction):
//   - Inline 128-node tree pool (`HierarchicalSummaryTree`) supporting raw leaf turns & multi-level summaries
//   - Overlap-free recursive `compact_range` folding contiguous top-level nodes into parent summaries
//   - Zero-overlap `select_context_frontier` prioritizing pinned turns, recent raw turns, highest non-overlapping
//     summaries (with automatic drill-down), and remaining high-importance history in ascending chronological order

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum total nodes (raw leaves + compacted summaries) in `HierarchicalSummaryTree`.
pub const SUMMARY_TREE_MAX_NODES: usize = 128;

/// Sentinel index indicating no parent or empty selection slot.
pub const SUMMARY_NO_NODE: u16 = 0xFFFF;

/// Kind of node in the hierarchical summary tree.
pub enum SummaryNodeKind {
    raw_turn,
    compacted_summary,
}

/// Single node in the hierarchical summary tree (`level == 0` for raw turns, `level >= 1` for summaries).
pub struct SummaryTreeNode {
    node_id: u16 = 0;
    kind: SummaryNodeKind = SummaryNodeKind.raw_turn;
    level: u8 = 0;
    start_turn: u32 = 0;
    end_turn: u32 = 0;
    token_count: u32 = 0;
    importance_bps: u16 = 0;
    parent_idx: u16 = SUMMARY_NO_NODE;
    covered_by_summary: bool = false;
    pinned: bool = false;
    active: bool = false;
}

/// Chronologically ordered context frontier selected under a strict token budget.
pub struct ContextFrontierPlan {
    selected_node_indices: [SUMMARY_TREE_MAX_NODES]u16 = [SUMMARY_NO_NODE; SUMMARY_TREE_MAX_NODES];
    selected_count: usize = 0;
    total_tokens: u32 = 0;
    raw_turns_kept: usize = 0;
    summaries_used: usize = 0;
    dropped_nodes: usize = 0;
    covered_turns: u32 = 0;
}

/// Aggregate tree size, hierarchy depth, and token compression statistics.
pub struct SummaryTreeStats {
    total_nodes: usize = 0;
    raw_leaf_count: usize = 0;
    summary_node_count: usize = 0;
    max_level: u8 = 0;
    raw_tokens_total: u64 = 0;
    compacted_tokens_total: u64 = 0;
}

fn rangesOverlap(a_start: u32, a_end: u32, b_start: u32, b_end: u32): bool {
    return !(a_end < b_start || b_end < a_start);
}

// ============================================================================
// 2. Zero-Alloc Hierarchical Summary Tree Engine
// ============================================================================

/// Zero-heap-allocation hierarchical conversation summary tree with progressive compaction.
pub struct HierarchicalSummaryTree {
    nodes: [SUMMARY_TREE_MAX_NODES]SummaryTreeNode = [SummaryTreeNode{}; SUMMARY_TREE_MAX_NODES];
    node_count: usize = 0;
    has_leaf: bool = false;
    last_turn_seq: u32 = 0;

    /// Initializes an empty `HierarchicalSummaryTree`.
    static fn init(): HierarchicalSummaryTree {
        return HierarchicalSummaryTree{
            nodes: [SummaryTreeNode{}; SUMMARY_TREE_MAX_NODES],
            node_count: 0,
            has_leaf: false,
            last_turn_seq: 0,
        };
    }

    /// Appends a raw conversation turn leaf (`level = 0`).
    /// Enforces strictly increasing `turn_seq`, `importance_bps <= 10000`, and 128-node capacity.
    fn append_raw_turn(
        *_,
        turn_seq: u32,
        token_count: u32,
        importance_bps: u16,
        pinned: bool,
    ): u16? {
        if (_.node_count >= SUMMARY_TREE_MAX_NODES) return null;
        if (importance_bps > 10000) return null;
        if (_.has_leaf && turn_seq <= _.last_turn_seq) return null;

        const idx: usize = _.node_count;
        const id_u16: u16 = @intCast(idx);
        _.nodes[idx] = SummaryTreeNode{
            node_id: id_u16,
            kind: SummaryNodeKind.raw_turn,
            level: 0,
            start_turn: turn_seq,
            end_turn: turn_seq,
            token_count: token_count,
            importance_bps: importance_bps,
            parent_idx: SUMMARY_NO_NODE,
            covered_by_summary: false,
            pinned: pinned,
            active: true,
        };
        _.node_count += 1;
        _.has_leaf = true;
        _.last_turn_seq = turn_seq;
        return id_u16;
    }

    /// Folds all active top-level nodes contained within `[start_turn, end_turn]` into a new
    /// higher-level `compacted_summary` parent node.
    /// - Rejects invalid ranges (`start_turn > end_turn`), `importance_bps > 10000`, full tree,
    ///   empty target ranges, or partial overlaps with any existing top-level node.
    /// - Marks folded children with `parent_idx = new_node_id` and `covered_by_summary = true`.
    fn compact_range(
        *_,
        start_turn: u32,
        end_turn: u32,
        summary_tokens: u32,
        importance_bps: u16,
    ): u16? {
        if (_.node_count >= SUMMARY_TREE_MAX_NODES) return null;
        if (start_turn > end_turn || importance_bps > 10000) return null;

        // Phase 1: validate no partial overlaps with any active top-level node & collect children.
        let child_indices: [SUMMARY_TREE_MAX_NODES]usize = [0; SUMMARY_TREE_MAX_NODES];
        let child_count: usize = 0;
        let max_child_level: u8 = 0;
        let actual_start: u32 = 0;
        let actual_end: u32 = 0;
        let any_pinned: bool = false;

        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            const nd = _.nodes[i];
            if (!nd.active || nd.covered_by_summary) continue;

            if (!rangesOverlap(nd.start_turn, nd.end_turn, start_turn, end_turn)) {
                continue;
            }

            // Partial overlap with an existing top-level node is forbidden.
            if (nd.start_turn < start_turn || nd.end_turn > end_turn) {
                return null;
            }

            if (child_count == 0 || nd.start_turn < actual_start) {
                actual_start = nd.start_turn;
            }
            if (child_count == 0 || nd.end_turn > actual_end) {
                actual_end = nd.end_turn;
            }
            if (nd.level > max_child_level) {
                max_child_level = nd.level;
            }
            if (nd.pinned) {
                any_pinned = true;
            }
            child_indices[child_count] = i;
            child_count += 1;
        }

        if (child_count == 0) return null;

        // Phase 2: allocate parent summary node and link children.
        const new_usize: usize = _.node_count;
        const new_id: u16 = @intCast(new_usize);
        const new_level: u8 = if (max_child_level < 255) max_child_level + 1 else 255;

        _.nodes[new_usize] = SummaryTreeNode{
            node_id: new_id,
            kind: SummaryNodeKind.compacted_summary,
            level: new_level,
            start_turn: actual_start,
            end_turn: actual_end,
            token_count: summary_tokens,
            importance_bps: importance_bps,
            parent_idx: SUMMARY_NO_NODE,
            covered_by_summary: false,
            pinned: any_pinned,
            active: true,
        };
        _.node_count += 1;

        let c: usize = 0;
        while (c < child_count) : (c += 1) {
            const ci: usize = child_indices[c];
            _.nodes[ci].parent_idx = new_id;
            _.nodes[ci].covered_by_summary = true;
        }

        return new_id;
    }

    fn conflictsWithSelected(
        cand_idx: usize,
        selected_flags: []const bool,
    ): bool {
        const cand = _.nodes[cand_idx];
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            if (!selected_flags[i]) continue;
            const sel = _.nodes[i];
            if (rangesOverlap(cand.start_turn, cand.end_turn, sel.start_turn, sel.end_turn)) {
                return true;
            }
        }
        return false;
    }

    /// Selects a zero-overlap, chronologically sorted context frontier fitting within `token_budget`:
    /// 1. Highest priority: `pinned` raw turn leaves (newest first).
    /// 2. Recent window: up to `min_recent_raw_turns` most recent raw turn leaves.
    /// 3. Historical summaries: highest-level non-overlapping `compacted_summary` nodes (auto drill-down).
    /// 4. Remaining uncovered raw turns by descending `(importance_bps, start_turn)`.
    fn select_context_frontier(
        token_budget: u32,
        min_recent_raw_turns: usize,
    ): ContextFrontierPlan {
        let plan = ContextFrontierPlan{};
        if (_.node_count == 0) return plan;

        let selected: [SUMMARY_TREE_MAX_NODES]bool = [false; SUMMARY_TREE_MAX_NODES];
        let used_tokens: u32 = 0;

        // Pass 1: Pinned raw turns (newest first)
        {
            let rev: usize = _.node_count;
            while (rev > 0) {
                rev -= 1;
                const nd = _.nodes[rev];
                if (!nd.active || nd.kind != SummaryNodeKind.raw_turn || !nd.pinned) continue;
                if (used_tokens + nd.token_count <= token_budget && !_.conflictsWithSelected(rev, selected[0.._.node_count])) {
                    selected[rev] = true;
                    used_tokens += nd.token_count;
                }
            }
        }

        // Pass 2: Most recent `min_recent_raw_turns` raw leaves
        if (min_recent_raw_turns > 0) {
            let seen_recent: usize = 0;
            let rev: usize = _.node_count;
            while (rev > 0 && seen_recent < min_recent_raw_turns) {
                rev -= 1;
                const nd = _.nodes[rev];
                if (!nd.active || nd.kind != SummaryNodeKind.raw_turn) continue;
                seen_recent += 1;
                if (selected[rev]) continue;
                if (used_tokens + nd.token_count <= token_budget && !_.conflictsWithSelected(rev, selected[0.._.node_count])) {
                    selected[rev] = true;
                    used_tokens += nd.token_count;
                }
            }
        }

        // Pass 3: Highest-level non-overlapping compacted summaries (greedy by level desc, importance desc, start_turn desc)
        while (true) {
            let best_idx: usize = _.node_count;
            let i: usize = 0;
            while (i < _.node_count) : (i += 1) {
                if (selected[i]) continue;
                const nd = _.nodes[i];
                if (!nd.active || nd.kind != SummaryNodeKind.compacted_summary) continue;
                if (used_tokens + nd.token_count > token_budget) continue;
                if (_.conflictsWithSelected(i, selected[0.._.node_count])) continue;

                if (best_idx == _.node_count) {
                    best_idx = i;
                } else {
                    const b = _.nodes[best_idx];
                    const better: bool = (nd.level > b.level) ||
                        (nd.level == b.level && nd.importance_bps > b.importance_bps) ||
                        (nd.level == b.level && nd.importance_bps == b.importance_bps && nd.start_turn > b.start_turn);
                    if (better) {
                        best_idx = i;
                    }
                }
            }
            if (best_idx == _.node_count) break;
            selected[best_idx] = true;
            used_tokens += _.nodes[best_idx].token_count;
        }

        // Pass 4: Remaining uncovered raw turns by descending (importance_bps, start_turn)
        while (true) {
            let best_idx: usize = _.node_count;
            let i: usize = 0;
            while (i < _.node_count) : (i += 1) {
                if (selected[i]) continue;
                const nd = _.nodes[i];
                if (!nd.active || nd.kind != SummaryNodeKind.raw_turn) continue;
                if (used_tokens + nd.token_count > token_budget) continue;
                if (_.conflictsWithSelected(i, selected[0.._.node_count])) continue;

                if (best_idx == _.node_count) {
                    best_idx = i;
                } else {
                    const b = _.nodes[best_idx];
                    const better: bool = (nd.importance_bps > b.importance_bps) ||
                        (nd.importance_bps == b.importance_bps && nd.start_turn > b.start_turn);
                    if (better) {
                        best_idx = i;
                    }
                }
            }
            if (best_idx == _.node_count) break;
            selected[best_idx] = true;
            used_tokens += _.nodes[best_idx].token_count;
        }

        // Collect selected indices & sort chronologically by ascending `start_turn`
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            if (!selected[i]) continue;
            plan.selected_node_indices[count] = @intCast(i);
            count += 1;
            const nd = _.nodes[i];
            if (nd.kind == SummaryNodeKind.raw_turn) {
                plan.raw_turns_kept += 1;
            } else {
                plan.summaries_used += 1;
            }
            plan.covered_turns += (nd.end_turn - nd.start_turn) + 1;
        }
        plan.selected_count = count;
        plan.total_tokens = used_tokens;

        // Insertion sort selected_node_indices[0..count] by ascending start_turn
        let a: usize = 1;
        while (a < count) : (a += 1) {
            const key_idx: u16 = plan.selected_node_indices[a];
            const key_start: u32 = _.nodes[key_idx as usize].start_turn;
            let pos: usize = a;
            while (pos > 0 && _.nodes[plan.selected_node_indices[pos - 1] as usize].start_turn > key_start) {
                plan.selected_node_indices[pos] = plan.selected_node_indices[pos - 1];
                pos -= 1;
            }
            plan.selected_node_indices[pos] = key_idx;
        }

        // Count raw leaf turns not covered by any selected node
        let dropped: usize = 0;
        let r: usize = 0;
        while (r < _.node_count) : (r += 1) {
            const nd = _.nodes[r];
            if (!nd.active || nd.kind != SummaryNodeKind.raw_turn) continue;
            if (!_.conflictsWithSelected(r, selected[0.._.node_count])) {
                dropped += 1;
            }
        }
        plan.dropped_nodes = dropped;
        return plan;
    }

    /// Returns a snapshot of tree size, hierarchy depth, and raw vs compacted token totals.
    fn stats(): SummaryTreeStats {
        let st = SummaryTreeStats{
            total_nodes: _.node_count,
            raw_leaf_count: 0,
            summary_node_count: 0,
            max_level: 0,
            raw_tokens_total: 0,
            compacted_tokens_total: 0,
        };
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            const nd = _.nodes[i];
            if (!nd.active) continue;
            if (nd.kind == SummaryNodeKind.raw_turn) {
                st.raw_leaf_count += 1;
                st.raw_tokens_total += nd.token_count as u64;
            } else {
                st.summary_node_count += 1;
            }
            if (nd.level > st.max_level) {
                st.max_level = nd.level;
            }
            if (!nd.covered_by_summary) {
                st.compacted_tokens_total += nd.token_count as u64;
            }
        }
        return st;
    }
}
