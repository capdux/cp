// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/summary_tree.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Hierarchical Summary Tree & Context Compactor:
//   1. Raw turn append, monotonic turn_seq enforcement & basic stats
//   2. Single-level & multi-level recursive compact_range folding + compression ratio stats
//   3. select_context_frontier: recent raw turns + historical summaries seamlessly stitched in chronological order
//   4. Zero overlap invariant: never selects both a parent summary and any of its covered descendants (auto drill-down)
//   5. Pinned critical turns retain highest priority under tight token budget
//   6. Extreme tight budget graceful pruning, dropped_nodes accounting & ascending start_turn order
//   7. Boundary guards: invalid range, partial-overlap compaction rejection, bad importance & 128-node capacity cap

import { test, expect_eq } from "cap:test";
import {
    SUMMARY_TREE_MAX_NODES,
    SUMMARY_NO_NODE,
    SummaryNodeKind,
    ContextFrontierPlan,
    SummaryTreeStats,
    HierarchicalSummaryTree,
} from "cap:agentmem";

// ============================================================================
// Test 1: Raw Turn Append & Strict Monotonic turn_seq Guard
// ============================================================================
test("summary_tree: raw turn append & monotonic turn_seq validation", () => {
    expect_eq(SUMMARY_TREE_MAX_NODES, 128);
    expect_eq(SUMMARY_NO_NODE, 0xFFFF);

    let tree = HierarchicalSummaryTree.init();
    const id0 = tree.append_raw_turn(1, 120, 5000, false);
    const id1 = tree.append_raw_turn(2, 150, 6000, true);
    expect_eq(id0 != null, true);
    expect_eq(id1 != null, true);
    expect_eq(id0 ?? 99, 0);
    expect_eq(id1 ?? 99, 1);

    // Non-monotonic (equal or decreasing) turn_seq must be rejected
    expect_eq(tree.append_raw_turn(2, 100, 5000, false) == null, true);
    expect_eq(tree.append_raw_turn(1, 100, 5000, false) == null, true);
    // importance_bps > 10000 rejected
    expect_eq(tree.append_raw_turn(3, 100, 10001, false) == null, true);

    const st: SummaryTreeStats = tree.stats();
    expect_eq(st.total_nodes, 2);
    expect_eq(st.raw_leaf_count, 2);
    expect_eq(st.summary_node_count, 0);
    expect_eq(st.max_level, 0);
    expect_eq(st.raw_tokens_total, 270);
    expect_eq(st.compacted_tokens_total, 270);
});

// ============================================================================
// Test 2: Single-Level & Multi-Level Recursive Compaction + Compression Ratio
// ============================================================================
test("summary_tree: single & multi-level recursive compact_range folding", () => {
    let tree = HierarchicalSummaryTree.init();
    // Append 8 raw turns (1..8), 100 tokens each = 800 raw tokens
    let t: u32 = 1;
    while (t <= 8) : (t += 1) {
        expect_eq(tree.append_raw_turn(t, 100, 5000, false) != null, true);
    }

    // Level-1 compactions: [1..4] -> 60 tokens, [5..8] -> 60 tokens
    const s1_opt = tree.compact_range(1, 4, 60, 7000);
    const s2_opt = tree.compact_range(5, 8, 60, 7500);
    expect_eq(s1_opt != null, true);
    expect_eq(s2_opt != null, true);
    const s1 = s1_opt ?? 0;
    const s2 = s2_opt ?? 0;
    expect_eq(tree.nodes[s1 as usize].level, 1);
    expect_eq(tree.nodes[s2 as usize].level, 1);
    expect_eq(tree.nodes[0].parent_idx, s1);
    expect_eq(tree.nodes[0].covered_by_summary, true);

    const st1 = tree.stats();
    expect_eq(st1.raw_tokens_total, 800);
    expect_eq(st1.compacted_tokens_total, 120); // 60 + 60
    expect_eq(st1.max_level, 1);

    // Level-2 recursive compaction: [1..8] folds s1 and s2 into 45 tokens
    const s_root_opt = tree.compact_range(1, 8, 45, 9000);
    expect_eq(s_root_opt != null, true);
    const s_root = s_root_opt ?? 0;
    expect_eq(tree.nodes[s_root as usize].level, 2);
    expect_eq(tree.nodes[s1 as usize].parent_idx, s_root);
    expect_eq(tree.nodes[s1 as usize].covered_by_summary, true);
    expect_eq(tree.nodes[s2 as usize].parent_idx, s_root);
    expect_eq(tree.nodes[s2 as usize].covered_by_summary, true);

    const st2 = tree.stats();
    expect_eq(st2.total_nodes, 11);
    expect_eq(st2.raw_leaf_count, 8);
    expect_eq(st2.summary_node_count, 3);
    expect_eq(st2.max_level, 2);
    expect_eq(st2.raw_tokens_total, 800);
    expect_eq(st2.compacted_tokens_total, 45);
});

// ============================================================================
// Test 3: Context Frontier Stitches Historical Summaries + Recent Raw Turns
// ============================================================================
test("summary_tree: select_context_frontier stitches summaries & recent raw turns chronologically", () => {
    let tree = HierarchicalSummaryTree.init();
    // Turns 1..6 (100 tokens each): compact [1..4] into 50-token summary, keep 5..6 raw
    let t: u32 = 1;
    while (t <= 6) : (t += 1) {
        expect_eq(tree.append_raw_turn(t, 100, 4000, false) != null, true);
    }
    const sum_1_4 = tree.compact_range(1, 4, 50, 8000) ?? return;

    // Budget = 260, min_recent_raw_turns = 2 -> keeps raw 5 (100), raw 6 (100), and summary [1..4] (50) = 250 tokens
    const plan: ContextFrontierPlan = tree.select_context_frontier(260, 2);
    expect_eq(plan.selected_count, 3);
    expect_eq(plan.total_tokens, 250);
    expect_eq(plan.raw_turns_kept, 2);
    expect_eq(plan.summaries_used, 1);
    expect_eq(plan.dropped_nodes, 0);
    expect_eq(plan.covered_turns, 6);

    // Strictly ascending chronological order: [sum(1..4), turn 5, turn 6]
    expect_eq(plan.selected_node_indices[0], sum_1_4);
    expect_eq(tree.nodes[plan.selected_node_indices[1] as usize].start_turn, 5);
    expect_eq(tree.nodes[plan.selected_node_indices[2] as usize].start_turn, 6);
});

// ============================================================================
// Test 4: Zero Overlap Invariant & Automatic Multi-Level Summary Drill-Down
// ============================================================================
test("summary_tree: zero overlap between parent summary & child leaves with auto drill-down", () => {
    let tree = HierarchicalSummaryTree.init();
    let t: u32 = 1;
    while (t <= 8) : (t += 1) {
        expect_eq(tree.append_raw_turn(t, 100, 4000, false) != null, true);
    }
    const s1 = tree.compact_range(1, 4, 40, 7000) ?? return; // L1 [1..4]
    * = tree.compact_range(5, 8, 40, 7000);                  // L1 [5..8]
    * = tree.compact_range(1, 8, 30, 9000);                  // L2 [1..8]

    // Request min_recent_raw_turns = 2 (keeps raw turns 7 & 8 = 200 tokens), total budget = 350:
    // L2 [1..8] conflicts with turns 7 & 8 -> auto drills down to L1 [1..4] (40 tokens)!
    // Remaining budget = 110 -> fits raw turn 6 (100 tokens) without overlapping [1..4] or [7..8].
    const plan = tree.select_context_frontier(350, 2);
    expect_eq(plan.total_tokens <= 350, true);
    expect_eq(plan.summaries_used, 1);
    expect_eq(plan.selected_node_indices[0], s1);

    // Verify pairwise non-overlapping & strictly ascending start_turn across all selected nodes
    let i: usize = 0;
    while (i + 1 < plan.selected_count) : (i += 1) {
        const a = tree.nodes[plan.selected_node_indices[i] as usize];
        const b = tree.nodes[plan.selected_node_indices[i + 1] as usize];
        expect_eq(a.end_turn < b.start_turn, true);
    }
});

// ============================================================================
// Test 5: Pinned Turns Retain Highest Priority Under Tight Budget
// ============================================================================
test("summary_tree: pinned critical turn prioritized over non-pinned turns under tight budget", () => {
    let tree = HierarchicalSummaryTree.init();
    // Turn 1 is pinned (e.g. critical user constraint, 80 tokens)
    const pin_id = tree.append_raw_turn(1, 80, 9500, true) ?? return;
    * = tree.append_raw_turn(2, 80, 5000, false);
    * = tree.append_raw_turn(3, 80, 5000, false);
    * = tree.append_raw_turn(4, 80, 5000, false);

    // Budget = 160, min_recent_raw_turns = 2:
    // Pinned turn 1 takes 80 tokens first; then most recent turn 4 takes remaining 80 tokens.
    const plan = tree.select_context_frontier(160, 2);
    expect_eq(plan.selected_count, 2);
    expect_eq(plan.total_tokens, 160);
    expect_eq(plan.selected_node_indices[0], pin_id);
    expect_eq(tree.nodes[plan.selected_node_indices[1] as usize].start_turn, 4);
    expect_eq(plan.dropped_nodes, 2); // turns 2 & 3 dropped
});

// ============================================================================
// Test 6: Extreme Tight Budget Graceful Pruning & Zero-Budget Behavior
// ============================================================================
test("summary_tree: extreme tight budget & zero budget graceful pruning", () => {
    let tree = HierarchicalSummaryTree.init();
    * = tree.append_raw_turn(1, 200, 3000, false);
    * = tree.append_raw_turn(2, 200, 9000, false);
    * = tree.append_raw_turn(3, 200, 4000, false);

    // Zero budget selects nothing and reports all 3 raw turns dropped
    const p0 = tree.select_context_frontier(0, 2);
    expect_eq(p0.selected_count, 0);
    expect_eq(p0.total_tokens, 0);
    expect_eq(p0.dropped_nodes, 3);

    // Budget = 200, min_recent_raw_turns = 0 -> selects highest-importance turn 2 (9000 bps)
    const p1 = tree.select_context_frontier(200, 0);
    expect_eq(p1.selected_count, 1);
    expect_eq(p1.total_tokens, 200);
    expect_eq(tree.nodes[p1.selected_node_indices[0] as usize].start_turn, 2);
    expect_eq(p1.dropped_nodes, 2);
});

// ============================================================================
// Test 7: Boundary Guards (Invalid Range, Partial Overlap Rejection & 128-Node Cap)
// ============================================================================
test("summary_tree: invalid range, partial overlap compaction & 128-node capacity guards", () => {
    let tree = HierarchicalSummaryTree.init();
    let t: u32 = 1;
    while (t <= 4) : (t += 1) {
        expect_eq(tree.append_raw_turn(t, 50, 5000, false) != null, true);
    }

    // Invalid range (start > end) & empty range [10..12] rejected
    expect_eq(tree.compact_range(4, 2, 20, 5000) == null, true);
    expect_eq(tree.compact_range(10, 12, 20, 5000) == null, true);

    // Compact [1..3] succeeds
    expect_eq(tree.compact_range(1, 3, 25, 6000) != null, true);
    // Partial overlap [2..4] crosses top-level summary [1..3] -> MUST be rejected!
    expect_eq(tree.compact_range(2, 4, 25, 6000) == null, true);

    // Fill remaining nodes up to SUMMARY_TREE_MAX_NODES (128)
    let next_turn: u32 = 5;
    while (tree.stats().total_nodes < SUMMARY_TREE_MAX_NODES) : (next_turn += 1) {
        expect_eq(tree.append_raw_turn(next_turn, 10, 5000, false) != null, true);
    }
    expect_eq(tree.stats().total_nodes, SUMMARY_TREE_MAX_NODES);

    // Further append or compact_range on full tree returns null cleanly
    expect_eq(tree.append_raw_turn(next_turn + 1, 10, 5000, false) == null, true);
    expect_eq(tree.compact_range(5, 6, 5, 5000) == null, true);
});
