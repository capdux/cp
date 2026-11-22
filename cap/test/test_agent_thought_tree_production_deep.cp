// Origin: Pure-CP Stdlib Test Suite (lib/agent/thought_tree.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Heap Tree-of-Thoughts Search Manager:
//   1. Tree branching, parent Open->Expanded transition & Best-First frontier selection with tie-breaking
//   2. Multi-level O(N) non-recursive subtree pruning & sibling isolation
//   3. Parent-chain reward backpropagation (visit_count & running-mean agg_score)
//   4. Root-to-target forward path extraction & undersized buffer protection
//   5. Max-depth limit & fixed node pool exhaustion enforcement
//   6. Empty buffer, duplicate root, NaN/Inf score/reward & invalid parent/node defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    THOUGHT_NO_PARENT,
    ThoughtNodeState,
    ThoughtNode,
    ThoughtTreeStats,
    ThoughtTree,
} from "cap:agent";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

// ============================================================================
// Test 1: Branching & Best-First Frontier Selection with Tie-Breaking
// ============================================================================

fn testBestFirstSelectionAndTieBreaking()! {
    let pool: [8]ThoughtNode = [ThoughtNode{}; 8];
    let tree = cap.agent.ThoughtTree.init(pool[0..], 4) ?? return error.InitFailed;

    const root = tree.setRoot("Root problem", 0.5) ?? return error.RootFailed;
    expect_eq(root, 0);
    expect_eq(tree.selectBestOpenNode() ?? 99, 0);

    // Branch 3 children: c1=0.7, c2=0.9 (Open), c3=0.9 (Open, higher id), c4=1.0 (Terminal)
    const c1 = tree.addChild(root, "Plan A", 0.7, false) ?? return error.AddFailed;
    const c2 = tree.addChild(root, "Plan B", 0.9, false) ?? return error.AddFailed;
    const c3 = tree.addChild(root, "Plan C", 0.9, false) ?? return error.AddFailed;
    const c4 = tree.addChild(root, "Direct Terminal", 1.0, true) ?? return error.AddFailed;

    // Root must now be Expanded; Terminal c4 is skipped by selectBestOpenNode; tie (c2 vs c3) picks smaller ID c2
    expect_eq((tree.getNode(root) ?? return error.Missing).state == ThoughtNodeState.Expanded, true);
    expect_eq((tree.getNode(c4) ?? return error.Missing).state == ThoughtNodeState.Terminal, true);
    expect_eq(tree.selectBestOpenNode() ?? 99, c2);

    // Expanding c2 transitions c2 to Expanded -> next best Open is c3 (0.9 > c1's 0.7)
    * = tree.addChild(c2, "Plan B.1", 0.6, false) ?? return error.AddFailed;
    expect_eq(tree.selectBestOpenNode() ?? 99, c3);
    * = c1;
}

// ============================================================================
// Test 2: Multi-Level Subtree Pruning & Sibling Isolation
// ============================================================================

fn testMultiLevelSubtreePruning()! {
    let pool: [12]ThoughtNode = [ThoughtNode{}; 12];
    let tree = ThoughtTree.init(pool[0..], 5) ?? return error.InitFailed;

    const root = tree.setRoot("Root", 0.5) ?? return error.RootFailed;
    const a = tree.addChild(root, "Branch A", 0.8, false) ?? return error.AddFailed;
    const b = tree.addChild(root, "Branch B", 0.6, false) ?? return error.AddFailed;
    const a1 = tree.addChild(a, "A.1", 0.4, false) ?? return error.AddFailed;
    const a2 = tree.addChild(a, "A.2", 0.3, false) ?? return error.AddFailed;
    const a1_deep = tree.addChild(a1, "A.1.1", 0.2, false) ?? return error.AddFailed;
    const b1 = tree.addChild(b, "B.1", 0.7, false) ?? return error.AddFailed;

    // Prune Branch A -> must prune {a, a1, a2, a1_deep} = 4 nodes, leaving {root, b, b1} untouched!
    const pruned_n = tree.pruneSubtree(a);
    expect_eq(pruned_n, 4);
    expect_eq((tree.getNode(a) ?? return error.Missing).state == ThoughtNodeState.Pruned, true);
    expect_eq((tree.getNode(a1) ?? return error.Missing).state == ThoughtNodeState.Pruned, true);
    expect_eq((tree.getNode(a2) ?? return error.Missing).state == ThoughtNodeState.Pruned, true);
    expect_eq((tree.getNode(a1_deep) ?? return error.Missing).state == ThoughtNodeState.Pruned, true);
    expect_eq((tree.getNode(b1) ?? return error.Missing).state == ThoughtNodeState.Open, true);

    // Idempotent re-prune returns 0; branching under pruned node rejected
    expect_eq(tree.pruneSubtree(a), 0);
    expect_eq(tree.addChild(a1, "Illegal child", 0.9, false) == null, true);

    // Best open node must now be b1
    expect_eq(tree.selectBestOpenNode() ?? 99, b1);

    const st = tree.stats();
    expect_eq(st.total_nodes, 7);
    expect_eq(st.pruned_nodes, 4);
    expect_eq(st.open_nodes, 1);
    expect_eq(st.expanded_nodes, 2);
    expect_eq(st.max_depth_reached, 3);
}

// ============================================================================
// Test 3: Parent-Chain Reward Backpropagation
// ============================================================================

fn testRewardBackpropagation()! {
    let pool: [8]ThoughtNode = [ThoughtNode{}; 8];
    let tree = ThoughtTree.init(pool[0..], 4) ?? return error.InitFailed;

    const root = tree.setRoot("Root", 0.4) ?? return error.RootFailed;        // score=0.4, visits=1
    const c1 = tree.addChild(root, "C1", 0.2, false) ?? return error.AddFailed; // score=0.2, visits=1
    const c2 = tree.addChild(root, "C2", 0.5, false) ?? return error.AddFailed; // score=0.5, visits=1
    const leaf = tree.addChild(c1, "L1", 0.6, false) ?? return error.AddFailed; // score=0.6, visits=1

    // Backpropagate reward = 1.0 from leaf -> updates leaf, c1, root (not c2!)
    expect_eq(tree.backpropagateScore(leaf, 1.0), true);

    const n_leaf = tree.getNode(leaf) ?? return error.Missing;
    const n_c1 = tree.getNode(c1) ?? return error.Missing;
    const n_root = tree.getNode(root) ?? return error.Missing;
    const n_c2 = tree.getNode(c2) ?? return error.Missing;

    expect_eq(n_leaf.visit_count, 2);
    expect_eq(absF32(n_leaf.agg_score - 0.8) < 0.0001, true); // (0.6 + 1.0) / 2 = 0.8
    expect_eq(n_c1.visit_count, 2);
    expect_eq(absF32(n_c1.agg_score - 0.6) < 0.0001, true);   // (0.2 + 1.0) / 2 = 0.6
    expect_eq(n_root.visit_count, 2);
    expect_eq(absF32(n_root.agg_score - 0.7) < 0.0001, true); // (0.4 + 1.0) / 2 = 0.7
    expect_eq(n_c2.visit_count, 1);
    expect_eq(absF32(n_c2.agg_score - 0.5) < 0.0001, true);

    // Best open node is leaf (0.8 > c2's 0.5)
    expect_eq(tree.selectBestOpenNode() ?? 99, leaf);
}

// ============================================================================
// Test 4: Root-to-Target Forward Path Extraction
// ============================================================================

fn testExtractPathToRoot()! {
    let pool: [8]ThoughtNode = [ThoughtNode{}; 8];
    let tree = ThoughtTree.init(pool[0..], 4) ?? return error.InitFailed;

    const r = tree.setRoot("R", 0.5) ?? return error.RootFailed;
    const n1 = tree.addChild(r, "N1", 0.6, false) ?? return error.AddFailed;
    const n2 = tree.addChild(n1, "N2", 0.7, false) ?? return error.AddFailed;
    const n3 = tree.addChild(n2, "N3", 0.9, true) ?? return error.AddFailed;

    let path: [4]u32 = [0; 4];
    const plen = tree.extractPathToRoot(n3, path[0..]) ?? return error.PathFailed;
    expect_eq(plen, 4);
    expect_eq(path[0], r);
    expect_eq(path[1], n1);
    expect_eq(path[2], n2);
    expect_eq(path[3], n3);

    // Root-only path has length 1
    const rlen = tree.extractPathToRoot(r, path[0..]) ?? return error.PathFailed;
    expect_eq(rlen, 1);
    expect_eq(path[0], r);

    // Undersized buffer returns null safely
    let small: [2]u32 = [0; 2];
    expect_eq(tree.extractPathToRoot(n3, small[0..]) == null, true);
}

// ============================================================================
// Test 5: Depth Limit & Fixed Node Pool Exhaustion
// ============================================================================

fn testDepthAndCapacityLimits()! {
    // Max depth = 2, pool capacity = 3
    let pool: [3]ThoughtNode = [ThoughtNode{}; 3];
    let tree = ThoughtTree.init(pool[0..], 2) ?? return error.InitFailed;

    const r = tree.setRoot("d0", 0.5) ?? return error.RootFailed;
    const d1 = tree.addChild(r, "d1", 0.5, false) ?? return error.AddFailed;
    const d2 = tree.addChild(d1, "d2", 0.5, false) ?? return error.AddFailed;

    // Pool is full (3/3)
    expect_eq(tree.addChild(r, "overflow", 0.5, false) == null, true);

    // Separate tree with large pool but max_depth = 1
    let pool2: [8]ThoughtNode = [ThoughtNode{}; 8];
    let shallow = ThoughtTree.init(pool2[0..], 1) ?? return error.InitFailed;
    const sr = shallow.setRoot("d0", 0.5) ?? return error.RootFailed;
    const sd1 = shallow.addChild(sr, "d1", 0.5, false) ?? return error.AddFailed;
    expect_eq(shallow.addChild(sd1, "d2-too-deep", 0.5, false) == null, true);
    * = d2;
}

// ============================================================================
// Test 6: Defensive Boundary & Invalid Input Guards
// ============================================================================

fn testDefensiveGuards()! {
    let pool: [4]ThoughtNode = [ThoughtNode{}; 4];
    let empty_pool: []ThoughtNode = pool[0..0];

    expect_eq(ThoughtTree.init(empty_pool, 4) == null, true);

    let tree = ThoughtTree.init(pool[0..], 4) ?? return error.InitFailed;
    expect_eq(tree.selectBestOpenNode() == null, true);

    // NaN / Inf score rejected on setRoot
    expect_eq(tree.setRoot("nan", std.math.nan(f32)) == null, true);
    expect_eq(tree.setRoot("inf", std.math.inf(f32)) == null, true);

    const r = tree.setRoot("root", 0.5) ?? return error.RootFailed;
    // Duplicate setRoot rejected
    expect_eq(tree.setRoot("root2", 0.5) == null, true);

    // Terminal child cannot have children; NaN score rejected; invalid parent rejected
    const term = tree.addChild(r, "done", 1.0, true) ?? return error.AddFailed;
    expect_eq(tree.addChild(term, "after-term", 0.5, false) == null, true);
    expect_eq(tree.addChild(r, "nan-child", std.math.nan(f32), false) == null, true);
    expect_eq(tree.addChild(99, "bad-parent", 0.5, false) == null, true);

    // Invalid IDs & NaN reward on backpropagate / prune / extract / get
    expect_eq(tree.backpropagateScore(r, std.math.nan(f32)), false);
    expect_eq(tree.backpropagateScore(99, 1.0), false);
    expect_eq(tree.pruneSubtree(99), 0);
    let path: [4]u32 = [0; 4];
    expect_eq(tree.extractPathToRoot(99, path[0..]) == null, true);
    expect_eq(tree.getNode(99) == null, true);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("agent/thought_tree: branching & Best-First frontier selection with tie-breaking", testBestFirstSelectionAndTieBreaking);
test("agent/thought_tree: multi-level O(N) subtree pruning & sibling isolation", testMultiLevelSubtreePruning);
test("agent/thought_tree: parent-chain reward backpropagation", testRewardBackpropagation);
test("agent/thought_tree: root-to-target forward path extraction", testExtractPathToRoot);
test("agent/thought_tree: max-depth & fixed pool exhaustion guards", testDepthAndCapacityLimits);
test("agent/thought_tree: empty pool, duplicate root, NaN/Inf & invalid ID guards", testDefensiveGuards);
