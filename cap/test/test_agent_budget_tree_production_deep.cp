// Origin: Pure-CP Stdlib Test Suite (lib/agent/budget_tree.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Hierarchical Multi-Agent Budget Tree:
//   1. 3-level delegation (root -> sub-agent -> tool) & iterative spend bubbling (self vs subtree)
//   2. hard_reserved quota delegation, spend offset & closeNodeAndReleaseUnused reclamation
//   3. Triple-dimension breach detection (tokens / cost_micro_usd / wall_ms) & atomic non-pollution
//   4. Ancestor-triggered breach in shared_pool mode (triggered_node_id == root) locking descendants
//   5. Iterative cascading stop-loss (cancelSubtree) isolating sibling subtrees & reclaiming reserves
//   6. Illegal delegation & invalid node guards (parent_insufficient_quota, node_not_found, node_inactive)
//   7. Unlimited (0) dimension semantics (remaining*() == null)
//   8. 64-node capacity saturation (tree_full), stats() telemetry & cap.agent namespace access

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BUDGET_TREE_MAX_NODES,
    BUDGET_TREE_NO_PARENT,
    BudgetNodeState,
    BudgetAllocationMode,
    BudgetExceededDimension,
    BudgetTreeStatus,
    BudgetTreeLimits,
    AgentBudgetTree,
} from "cap:agent";

// ============================================================================
// Test 1: 3-Level Delegation & Iterative Spend Bubbling (self vs subtree)
// ============================================================================

fn testThreeLevelBubbling()! {
    let tree = AgentBudgetTree.init(BudgetTreeLimits{
        max_tokens: 10000,
        max_cost_micro_usd: 50000,
        max_wall_ms: 30000,
    });

    const sub = tree.spawnChildBudget(0, BudgetTreeLimits{
        max_tokens: 6000,
        max_cost_micro_usd: 30000,
        max_wall_ms: 20000,
    }, BudgetAllocationMode.shared_pool);
    expect_eq(sub.status == BudgetTreeStatus.ok, true);

    const tool = tree.spawnChildBudget(sub.child_id, BudgetTreeLimits{
        max_tokens: 2000,
        max_cost_micro_usd: 10000,
        max_wall_ms: 5000,
    }, BudgetAllocationMode.shared_pool);
    expect_eq(tool.status == BudgetTreeStatus.ok, true);

    // Spend on sub directly (500 tok) and on tool leaf (1200 tok)
    expect_eq(tree.recordSpend(sub.child_id, 500, 2000, 1000).status == BudgetTreeStatus.ok, true);
    const sp = tree.recordSpend(tool.child_id, 1200, 4000, 2500);
    expect_eq(sp.status == BudgetTreeStatus.ok, true);
    expect_eq(sp.node_subtree_tokens, 1200);

    const n_root = tree.getNode(0) ?? return error.MissingRoot;
    const n_sub = tree.getNode(sub.child_id) ?? return error.MissingSub;
    const n_tool = tree.getNode(tool.child_id) ?? return error.MissingTool;

    expect_eq(n_tool.self_tokens, 1200);
    expect_eq(n_tool.subtree_tokens, 1200);
    expect_eq(n_sub.self_tokens, 500);
    expect_eq(n_sub.subtree_tokens, 1700);
    expect_eq(n_root.self_tokens, 0);
    expect_eq(n_root.subtree_tokens, 1700);
    expect_eq(n_root.subtree_cost_micro_usd, 6000);
    expect_eq(n_root.subtree_wall_ms, 3500);
    expect_eq(n_root.remainingTokens() ?? 0, 8300);
}

// ============================================================================
// Test 2: hard_reserved Delegation & closeNodeAndReleaseUnused Reclamation
// ============================================================================

fn testHardReservedAndReclamation()! {
    let tree = AgentBudgetTree.init(BudgetTreeLimits{
        max_tokens: 1000,
        max_cost_micro_usd: 1000,
        max_wall_ms: 1000,
    });

    // Reserve 600/1000 on child A -> root remaining drops immediately to 400
    const a = tree.spawnChildBudget(0, BudgetTreeLimits{
        max_tokens: 600,
        max_cost_micro_usd: 600,
        max_wall_ms: 600,
    }, BudgetAllocationMode.hard_reserved);
    expect_eq(a.status == BudgetTreeStatus.ok, true);
    expect_eq((tree.getNode(0) ?? return error.MissingRoot).remainingTokens() ?? 0, 400);

    // Attempting to reserve 500 on child B must fail (only 400 left)
    const b_fail = tree.spawnChildBudget(0, BudgetTreeLimits{
        max_tokens: 500,
        max_cost_micro_usd: 500,
        max_wall_ms: 500,
    }, BudgetAllocationMode.hard_reserved);
    expect_eq(b_fail.status == BudgetTreeStatus.parent_insufficient_quota, true);

    // Child A spends 250 -> root subtree=250, reserved=350, remaining still 400!
    expect_eq(tree.recordSpend(a.child_id, 250, 100, 150).status == BudgetTreeStatus.ok, true);
    const r_mid = tree.getNode(0) ?? return error.MissingRootMid;
    expect_eq(r_mid.subtree_tokens, 250);
    expect_eq(r_mid.reserved_tokens, 350);
    expect_eq(r_mid.remainingTokens() ?? 0, 400);

    // Close child A -> releases unused 350 tokens back to root -> root remaining jumps to 750!
    expect_eq(tree.closeNodeAndReleaseUnused(a.child_id) == BudgetTreeStatus.ok, true);
    const r_after = tree.getNode(0) ?? return error.MissingRootAfter;
    expect_eq(r_after.reserved_tokens, 0);
    expect_eq(r_after.remainingTokens() ?? 0, 750);

    // Closed child rejects further spend & double close
    expect_eq(tree.recordSpend(a.child_id, 1, 1, 1).status == BudgetTreeStatus.node_inactive, true);
    expect_eq(tree.closeNodeAndReleaseUnused(a.child_id) == BudgetTreeStatus.node_inactive, true);

    // Now reserving 500 on child B succeeds!
    const b_ok = tree.spawnChildBudget(0, BudgetTreeLimits{
        max_tokens: 500,
        max_cost_micro_usd: 500,
        max_wall_ms: 500,
    }, BudgetAllocationMode.hard_reserved);
    expect_eq(b_ok.status == BudgetTreeStatus.ok, true);
}

// ============================================================================
// Test 3: Triple-Dimension Breach Detection & Atomic Non-Pollution
// ============================================================================

fn testTripleDimensionBreachAndAtomicity()! {
    let tree = AgentBudgetTree.init(BudgetTreeLimits{
        max_tokens: 5000,
        max_cost_micro_usd: 5000,
        max_wall_ms: 5000,
    });

    const c_tok = tree.spawnChildBudget(0, BudgetTreeLimits{ max_tokens: 100, max_cost_micro_usd: 1000, max_wall_ms: 1000 }, BudgetAllocationMode.shared_pool);
    const c_cost = tree.spawnChildBudget(0, BudgetTreeLimits{ max_tokens: 1000, max_cost_micro_usd: 100, max_wall_ms: 1000 }, BudgetAllocationMode.shared_pool);
    const c_wall = tree.spawnChildBudget(0, BudgetTreeLimits{ max_tokens: 1000, max_cost_micro_usd: 1000, max_wall_ms: 100 }, BudgetAllocationMode.shared_pool);

    expect_eq(tree.recordSpend(c_tok.child_id, 80, 10, 10).status == BudgetTreeStatus.ok, true);
    const r_tok = tree.recordSpend(c_tok.child_id, 25, 10, 10); // 80+25 > 100
    expect_eq(r_tok.status == BudgetTreeStatus.exceeded_tokens, true);
    expect_eq(r_tok.exceeded_dim == BudgetExceededDimension.tokens, true);
    expect_eq(r_tok.triggered_node_id, c_tok.child_id);
    // Atomic: failed spend must NOT pollute counters (stays at 80)
    expect_eq((tree.getNode(c_tok.child_id) ?? return error.MissingCTok).subtree_tokens, 80);

    const r_cost = tree.recordSpend(c_cost.child_id, 10, 150, 10);
    expect_eq(r_cost.status == BudgetTreeStatus.exceeded_cost, true);
    expect_eq(r_cost.exceeded_dim == BudgetExceededDimension.cost_micro_usd, true);
    expect_eq(r_cost.triggered_node_id, c_cost.child_id);

    const r_wall = tree.recordSpend(c_wall.child_id, 10, 10, 120);
    expect_eq(r_wall.status == BudgetTreeStatus.exceeded_wall_ms, true);
    expect_eq(r_wall.exceeded_dim == BudgetExceededDimension.wall_ms, true);
    expect_eq(r_wall.triggered_node_id, c_wall.child_id);
}

// ============================================================================
// Test 4: Ancestor Breach in shared_pool Mode Locks All Descendants
// ============================================================================

fn testAncestorBreachLocksDescendants()! {
    let tree = AgentBudgetTree.init(BudgetTreeLimits{ max_tokens: 500, max_cost_micro_usd: 0, max_wall_ms: 0 });
    const c1 = tree.spawnChildBudget(0, BudgetTreeLimits{ max_tokens: 400 }, BudgetAllocationMode.shared_pool);
    const c2 = tree.spawnChildBudget(0, BudgetTreeLimits{ max_tokens: 400 }, BudgetAllocationMode.shared_pool);

    expect_eq(tree.recordSpend(c1.child_id, 350, 0, 0).status == BudgetTreeStatus.ok, true);
    // c2 is within its own 400 limit, but 350 + 200 > root's 500!
    const breach = tree.recordSpend(c2.child_id, 200, 0, 0);
    expect_eq(breach.status == BudgetTreeStatus.exceeded_tokens, true);
    expect_eq(breach.triggered_node_id, 0);
    expect_eq((tree.getNode(0) ?? return error.MissingRoot).state == BudgetNodeState.exhausted, true);

    // Now even c1 (which still has 50 own tokens left) is blocked because root is exhausted!
    expect_eq(tree.recordSpend(c1.child_id, 10, 0, 0).status == BudgetTreeStatus.node_inactive, true);
}

// ============================================================================
// Test 5: Iterative Cascading Stop-Loss (cancelSubtree) & Sibling Isolation
// ============================================================================

fn testCascadingCancelSubtree()! {
    let tree = AgentBudgetTree.init(BudgetTreeLimits{ max_tokens: 1000, max_cost_micro_usd: 1000, max_wall_ms: 1000 });
    const branch_a = tree.spawnChildBudget(0, BudgetTreeLimits{ max_tokens: 400, max_cost_micro_usd: 400, max_wall_ms: 400 }, BudgetAllocationMode.hard_reserved);
    const branch_b = tree.spawnChildBudget(0, BudgetTreeLimits{ max_tokens: 400, max_cost_micro_usd: 400, max_wall_ms: 400 }, BudgetAllocationMode.hard_reserved);
    const leaf_a1 = tree.spawnChildBudget(branch_a.child_id, BudgetTreeLimits{ max_tokens: 200, max_cost_micro_usd: 200, max_wall_ms: 200 }, BudgetAllocationMode.shared_pool);
    const leaf_a2 = tree.spawnChildBudget(leaf_a1.child_id, BudgetTreeLimits{ max_tokens: 100, max_cost_micro_usd: 100, max_wall_ms: 100 }, BudgetAllocationMode.shared_pool);

    expect_eq(tree.recordSpend(leaf_a2.child_id, 50, 50, 50).status == BudgetTreeStatus.ok, true);

    // Cancel branch_a -> must cancel branch_a + leaf_a1 + leaf_a2 (3 nodes) and return 350 reserved tokens to root
    const cancelled = tree.cancelSubtree(branch_a.child_id);
    expect_eq(cancelled, 3);
    expect_eq((tree.getNode(branch_a.child_id) ?? return error.MA).state == BudgetNodeState.cancelled, true);
    expect_eq((tree.getNode(leaf_a2.child_id) ?? return error.MA2).state == BudgetNodeState.cancelled, true);
    expect_eq(tree.recordSpend(leaf_a2.child_id, 1, 1, 1).status == BudgetTreeStatus.node_inactive, true);

    // Root remaining tokens should be 1000 - 50 (spent) - 400 (branch_b reserved) = 550
    expect_eq((tree.getNode(0) ?? return error.MRoot).remainingTokens() ?? 0, 550);

    // Sibling branch_b remains completely active and unaffected
    expect_eq(tree.recordSpend(branch_b.child_id, 100, 100, 100).status == BudgetTreeStatus.ok, true);
}

// ============================================================================
// Test 6: Illegal Delegation & Invalid Node Guards
// ============================================================================

fn testIllegalDelegationAndInvalidGuards()! {
    let tree = AgentBudgetTree.init(BudgetTreeLimits{ max_tokens: 100, max_cost_micro_usd: 100, max_wall_ms: 100 });

    // Unknown parent / node IDs
    expect_eq(tree.spawnChildBudget(99, BudgetTreeLimits{}, BudgetAllocationMode.shared_pool).status == BudgetTreeStatus.node_not_found, true);
    expect_eq(tree.recordSpend(99, 1, 1, 1).status == BudgetTreeStatus.node_not_found, true);
    expect_eq(tree.closeNodeAndReleaseUnused(99) == BudgetTreeStatus.node_not_found, true);
    expect_eq(tree.cancelSubtree(99), 0);
    expect_eq(tree.getNode(BUDGET_TREE_NO_PARENT) == null, true);

    // hard_reserved with 0 (unlimited) on a bounded parent dimension must fail
    expect_eq(tree.spawnChildBudget(0, BudgetTreeLimits{ max_tokens: 0, max_cost_micro_usd: 50, max_wall_ms: 50 }, BudgetAllocationMode.hard_reserved).status == BudgetTreeStatus.parent_insufficient_quota, true);
}

// ============================================================================
// Test 7: Unlimited (0) Dimension Semantics
// ============================================================================

fn testUnlimitedDimensions()! {
    let tree = AgentBudgetTree.init(BudgetTreeLimits{
        max_tokens: 0,           // unlimited
        max_cost_micro_usd: 500, // bounded
        max_wall_ms: 0,          // unlimited
    });
    const root = tree.getNode(0) ?? return error.MissingRoot;
    expect_eq(root.remainingTokens() == null, true);
    expect_eq(root.remainingWallMs() == null, true);
    expect_eq(root.remainingCostMicroUsd() ?? 0, 500);

    expect_eq(tree.recordSpend(0, 999999999, 400, 888888888).status == BudgetTreeStatus.ok, true);
    expect_eq(tree.recordSpend(0, 1, 150, 1).status == BudgetTreeStatus.exceeded_cost, true);
}

// ============================================================================
// Test 8: 64-Node Capacity Saturation, stats() & cap.agent Namespace
// ============================================================================

fn testCapacitySaturationAndStats()! {
    let tree = cap.agent.AgentBudgetTree.init(BudgetTreeLimits{});

    // Root occupies slot 0; spawn 63 children to reach BUDGET_TREE_MAX_NODES (64)
    let i: usize = 1;
    while (i < BUDGET_TREE_MAX_NODES) : (i += 1) {
        const res = tree.spawnChildBudget(0, BudgetTreeLimits{}, BudgetAllocationMode.shared_pool);
        expect_eq(res.status == BudgetTreeStatus.ok, true);
    }

    // 65th node must fail with tree_full
    expect_eq(tree.spawnChildBudget(0, BudgetTreeLimits{}, BudgetAllocationMode.shared_pool).status == BudgetTreeStatus.tree_full, true);

    expect_eq(tree.closeNodeAndReleaseUnused(1) == BudgetTreeStatus.ok, true);
    expect_eq(tree.cancelSubtree(2), 1);

    const st = tree.stats();
    expect_eq(st.total_nodes, BUDGET_TREE_MAX_NODES);
    expect_eq(st.closed_nodes, 1);
    expect_eq(st.cancelled_nodes, 1);
    expect_eq(st.active_nodes, BUDGET_TREE_MAX_NODES - 2);
}

// ============================================================================
// Test Entrypoints
// ============================================================================

test("budget_tree: 3-level delegation & iterative spend bubbling (self vs subtree)", testThreeLevelBubbling);
test("budget_tree: hard_reserved delegation & closeNodeAndReleaseUnused reclamation", testHardReservedAndReclamation);
test("budget_tree: triple-dimension breach detection & atomic non-pollution", testTripleDimensionBreachAndAtomicity);
test("budget_tree: ancestor breach in shared_pool mode locks descendants", testAncestorBreachLocksDescendants);
test("budget_tree: iterative cascading stop-loss (cancelSubtree) & sibling isolation", testCascadingCancelSubtree);
test("budget_tree: illegal delegation & invalid node guards", testIllegalDelegationAndInvalidGuards);
test("budget_tree: unlimited (0) dimension semantics", testUnlimitedDimensions);
test("budget_tree: 64-node capacity saturation, stats() & cap.agent namespace", testCapacitySaturationAndStats);
