// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/goalTree.cp) | Created: 2026-11-23
// Deep production regression tests for cap:agentmem hierarchical goal tree (`lib/agentmem/goalTree.cp`):
//   1. AND operator: progressive bps rollup (0 -> 3333 -> 6666 -> 10000), all-achieved, any-failed
//   2. OR operator: fallback scheduling on failure, auto-skipping unstarted sibling subtrees on win, all-failed
//   3. SEQ operator: strict predecessor blocking/unblocking and blocked-node mutation rejection
//   4. Bounded retry (`retryNode`): restores failed leaf & re-activates failed ancestor chain until max_retries
//   5. Nested HTN plan (SEQ -> AND + OR -> leaves) with priority-ordered `pickNextActionable` / `collectActionable`
//   6. Depth (16), capacity (64), leaf-parent, empty-name, and duplicate-root boundary guards
//   7. Zero-heap `GoalTreeStats` telemetry snapshot, `reset()`, and `cap.agentmem` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    GOAL_TREE_MAX_NODES,
    GOAL_TREE_MAX_DEPTH,
    GOAL_PROGRESS_FULL_BPS,
    GoalOperator,
    GoalStatus,
    GoalTree,
} from "cap:agentmem";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("goalTree: AND operator progress_bps rollup and any-child failure propagation") {
    let tree = GoalTree.init();
    const root = tree.setRoot("build_and_verify", 0x100, GoalOperator.op_and, 100, 0) ?? return;
    const c1 = tree.addChild(root, "compile", 0x101, GoalOperator.leaf, 30, 0) ?? return;
    const c2 = tree.addChild(root, "lint", 0x102, GoalOperator.leaf, 50, 0) ?? return;
    const c3 = tree.addChild(root, "unit_test", 0x103, GoalOperator.leaf, 40, 0) ?? return;

    expect_eq(tree.rootProgressBps(), @as(u16, 0));
    expect_true(tree.rootStatus() == GoalStatus.pending);

    // Highest priority leaf picked first: c2 (prio 50)
    expect_eq(tree.pickNextActionable() ?? 0, c2);
    expect_true(tree.markActive(c2));
    expect_true(tree.rootStatus() == GoalStatus.active);

    expect_true(tree.markAchieved(c2));
    expect_eq(tree.rootProgressBps(), @as(u16, 3333));

    expect_true(tree.markAchieved(c3));
    expect_eq(tree.rootProgressBps(), @as(u16, 6666));

    expect_true(tree.markAchieved(c1));
    expect_eq(tree.rootProgressBps(), GOAL_PROGRESS_FULL_BPS);
    expect_true(tree.rootStatus() == GoalStatus.achieved);
    expect_true(tree.pickNextActionable() == null);

    // Verify any-child failure fails AND parent
    let t2 = GoalTree.init();
    const r2 = t2.setRoot("strict_and", 0x200, GoalOperator.op_and, 10, 0) ?? return;
    const a1 = t2.addChild(r2, "a1", 0x201, GoalOperator.leaf, 10, 0) ?? return;
    const a2 = t2.addChild(r2, "a2", 0x202, GoalOperator.leaf, 10, 0) ?? return;
    expect_true(t2.markAchieved(a1));
    expect_true(t2.markFailed(a2));
    expect_true(t2.rootStatus() == GoalStatus.failed);
    expect_true(t2.pickNextActionable() == null);
}

test("goalTree: OR operator fallback, auto-skip of unstarted branches, and all-failed") {
    let tree = GoalTree.init();
    const root = tree.setRoot("fetch_data", 0x300, GoalOperator.op_or, 100, 0) ?? return;
    const primary = tree.addChild(root, "query_cache", 0x301, GoalOperator.leaf, 90, 0) ?? return;
    const backup = tree.addChild(root, "query_db", 0x302, GoalOperator.leaf, 70, 0) ?? return;
    const cold_branch = tree.addChild(root, "cold_archive", 0x303, GoalOperator.op_and, 50, 0) ?? return;
    const cold_leaf = tree.addChild(cold_branch, "restore_s3", 0x304, GoalOperator.leaf, 40, 0) ?? return;

    // Primary fails -> OR remains pending/runnable and scheduler falls back to backup
    expect_eq(tree.pickNextActionable() ?? 0, primary);
    expect_true(tree.markFailed(primary));
    expect_true(tree.rootStatus() == GoalStatus.pending);
    expect_eq(tree.pickNextActionable() ?? 0, backup);

    // Backup achieves -> OR root achieves and unstarted cold_branch + cold_leaf are auto-skipped!
    expect_true(tree.markAchieved(backup));
    expect_true(tree.rootStatus() == GoalStatus.achieved);
    expect_eq(tree.rootProgressBps(), GOAL_PROGRESS_FULL_BPS);

    if (cb := tree.getNode(cold_branch)) {
        expect_true(cb.status == GoalStatus.skipped);
    } else {
        expect_true(false);
    }
    if (cl := tree.getNode(cold_leaf)) {
        expect_true(cl.status == GoalStatus.skipped);
    } else {
        expect_true(false);
    }
    expect_true(tree.pickNextActionable() == null);

    // All children fail -> OR fails
    let t_fail = GoalTree.init();
    const rf = t_fail.setRoot("or_all_fail", 0x310, GoalOperator.op_or, 10, 0) ?? return;
    const f1 = t_fail.addChild(rf, "f1", 0x311, GoalOperator.leaf, 20, 0) ?? return;
    const f2 = t_fail.addChild(rf, "f2", 0x312, GoalOperator.leaf, 10, 0) ?? return;
    expect_true(t_fail.markFailed(f1));
    expect_true(t_fail.rootStatus() == GoalStatus.pending);
    expect_true(t_fail.markFailed(f2));
    expect_true(t_fail.rootStatus() == GoalStatus.failed);
}

test("goalTree: SEQ operator strict gating, blocked rejection, and ordered unlocking") {
    let tree = GoalTree.init();
    const root = tree.setRoot("deploy_pipeline", 0x400, GoalOperator.op_seq, 100, 0) ?? return;
    const s1 = tree.addChild(root, "prepare", 0x401, GoalOperator.leaf, 10, 0) ?? return;
    const s2 = tree.addChild(root, "migrate", 0x402, GoalOperator.leaf, 99, 0) ?? return; // higher prio, but blocked!
    const s3 = tree.addChild(root, "verify", 0x403, GoalOperator.leaf, 50, 0) ?? return;

    // Even though s2 has higher priority (99), SEQ gates it behind s1
    expect_eq(tree.pickNextActionable() ?? 0, s1);
    if (n2 := tree.getNode(s2)) {
        expect_true(n2.status == GoalStatus.blocked);
    } else {
        expect_true(false);
    }
    // Attempting to activate or achieve blocked s2 fails cleanly
    expect_eq(tree.markActive(s2), false);
    expect_eq(tree.markAchieved(s2), false);

    // Achieving s1 unlocks s2 (while s3 remains blocked)
    expect_true(tree.markAchieved(s1));
    expect_eq(tree.rootProgressBps(), @as(u16, 3333));
    expect_eq(tree.pickNextActionable() ?? 0, s2);
    if (n3 := tree.getNode(s3)) {
        expect_true(n3.status == GoalStatus.blocked);
    } else {
        expect_true(false);
    }

    // Complete s2 -> unlocks s3 -> complete s3 -> root achieved
    expect_true(tree.markAchieved(s2));
    expect_eq(tree.rootProgressBps(), @as(u16, 6666));
    expect_eq(tree.pickNextActionable() ?? 0, s3);
    expect_true(tree.markAchieved(s3));
    expect_eq(tree.rootProgressBps(), GOAL_PROGRESS_FULL_BPS);
    expect_true(tree.rootStatus() == GoalStatus.achieved);
}

test("goalTree: retryNode recovers failed leaf and re-activates failed ancestor chain up to max_retries") {
    let tree = GoalTree.init();
    const root = tree.setRoot("reliable_job", 0x500, GoalOperator.op_and, 100, 0) ?? return;
    const flaky = tree.addChild(root, "call_api", 0x501, GoalOperator.leaf, 80, 2) ?? return;
    const done = tree.addChild(root, "save_log", 0x502, GoalOperator.leaf, 50, 0) ?? return;

    expect_true(tree.markAchieved(done));
    expect_true(tree.markFailed(flaky));
    expect_true(tree.rootStatus() == GoalStatus.failed);

    // Retry #1 recovers both flaky and root
    expect_true(tree.retryNode(flaky));
    expect_true(tree.rootStatus() == GoalStatus.active);
    expect_eq(tree.pickNextActionable() ?? 0, flaky);

    // Fail again -> Retry #2 succeeds
    expect_true(tree.markFailed(flaky));
    expect_true(tree.retryNode(flaky));
    expect_true(tree.markFailed(flaky));

    // Retry #3 exceeds max_retries (2) -> rejected
    expect_eq(tree.retryNode(flaky), false);
    if (fn_node := tree.getNode(flaky)) {
        expect_eq(fn_node.retry_count, @as(u16, 2));
        expect_eq(fn_node.canRetry(), false);
    } else {
        expect_true(false);
    }
}

test("goalTree: nested HTN plan (SEQ -> AND + OR) with collectActionable priority order") {
    let tree = GoalTree.init();
    const root = tree.setRoot("agent_mission", 0x600, GoalOperator.op_seq, 100, 0) ?? return;
    const phase1 = tree.addChild(root, "gather_context", 0x601, GoalOperator.op_and, 90, 0) ?? return;
    const p1_low = tree.addChild(phase1, "read_docs", 0x602, GoalOperator.leaf, 20, 0) ?? return;
    const p1_high = tree.addChild(phase1, "scan_repo", 0x603, GoalOperator.leaf, 80, 0) ?? return;
    const phase2 = tree.addChild(root, "execute_fix", 0x604, GoalOperator.op_or, 70, 0) ?? return;
    const p2_fast = tree.addChild(phase2, "hotfix_patch", 0x605, GoalOperator.leaf, 95, 0) ?? return;
    const p2_full = tree.addChild(phase2, "full_refactor", 0x606, GoalOperator.leaf, 40, 0) ?? return;

    // While phase1 is running, phase2 is blocked -> only p1_high (80) and p1_low (20) are actionable
    let act: [8]u32 = [0; 8];
    let act_s: []u32 = act[0..];
    const n1 = tree.collectActionable(*act_s);
    expect_eq(n1, @as(usize, 2));
    expect_eq(act[0], p1_high);
    expect_eq(act[1], p1_low);

    // Finish phase1 -> root progress 5000 bps -> phase2 unlocks both p2_fast (95) and p2_full (40)
    expect_true(tree.markAchieved(p1_high));
    expect_eq(tree.rootProgressBps(), @as(u16, 2500));
    expect_true(tree.markAchieved(p1_low));
    expect_eq(tree.rootProgressBps(), @as(u16, 5000));

    const n2 = tree.collectActionable(*act_s);
    expect_eq(n2, @as(usize, 2));
    expect_eq(act[0], p2_fast);
    expect_eq(act[1], p2_full);

    // Complete p2_fast -> phase2 & root achieve (10000 bps), p2_full skipped
    expect_true(tree.markAchieved(p2_fast));
    expect_eq(tree.rootProgressBps(), GOAL_PROGRESS_FULL_BPS);
    expect_true(tree.rootStatus() == GoalStatus.achieved);
    if (full_n := tree.getNode(p2_full)) {
        expect_true(full_n.status == GoalStatus.skipped);
    } else {
        expect_true(false);
    }
}

test("goalTree: depth (16), capacity (64), leaf-parent, and invalid input boundary guards") {
    let tree = GoalTree.init();
    // Empty name rejected
    expect_true(tree.setRoot("", 1, GoalOperator.op_and, 1, 0) == null);

    const root = tree.setRoot("root", 1, GoalOperator.op_and, 1, 0) ?? return;
    // Duplicate root rejected
    expect_true(tree.setRoot("root2", 2, GoalOperator.op_and, 1, 0) == null);

    // Leaf cannot accept children
    const leaf = tree.addChild(root, "leaf", 3, GoalOperator.leaf, 1, 0) ?? return;
    expect_true(tree.addChild(leaf, "illegal_child", 4, GoalOperator.leaf, 1, 0) == null);

    // Depth guard: chain up to depth 15 (GOAL_TREE_MAX_DEPTH - 1), 16th rejected
    let cur: u32 = root;
    let d: u8 = 1;
    while (d < GOAL_TREE_MAX_DEPTH) {
        cur = tree.addChild(cur, "chain", @as(u64, d), GoalOperator.op_and, 1, 0) ?? return;
        d += 1;
    }
    expect_true(tree.addChild(cur, "too_deep", 999, GoalOperator.leaf, 1, 0) == null);

    // Fill remaining nodes up to GOAL_TREE_MAX_NODES (64)
    while (tree.stats().total_nodes < GOAL_TREE_MAX_NODES) {
        expect_true(tree.addChild(root, "filler", 7, GoalOperator.leaf, 1, 0) != null);
    }
    expect_eq(tree.stats().total_nodes, GOAL_TREE_MAX_NODES);
    // 65th node rejected
    expect_true(tree.addChild(root, "overflow", 8, GoalOperator.leaf, 1, 0) == null);
}

test("goalTree: zero-heap stats snapshot, reset, and cap.agentmem facade parity") {
    let tree = cap.agentmem.GoalTree.init();
    const root = tree.setRoot("mission", 0x999, cap.agentmem.GoalOperator.op_or, 100, 0) ?? return;
    const c1 = tree.addChild(root, "opt_a", 0x99A, cap.agentmem.GoalOperator.leaf, 50, 1) ?? return;
    const c2 = tree.addChild(root, "opt_b", 0x99B, cap.agentmem.GoalOperator.leaf, 30, 0) ?? return;

    if (rn := tree.getNode(root)) {
        expect_true(bytesEq(rn.nameSlice(), "mission"));
        expect_eq(rn.goal_hash, @as(u64, 0x999));
    } else {
        expect_true(false);
    }

    expect_true(tree.markFailed(c1));
    expect_true(tree.retryNode(c1));
    expect_true(tree.markAchieved(c1));

    const st = tree.stats();
    expect_eq(st.capacity, GOAL_TREE_MAX_NODES);
    expect_eq(st.total_nodes, @as(usize, 3));
    expect_eq(st.leaf_nodes, @as(usize, 2));
    expect_eq(st.achieved_nodes, @as(usize, 2)); // c1 + root
    expect_eq(st.skipped_nodes, @as(usize, 1));  // c2 auto-skipped
    expect_eq(st.max_depth, @as(u8, 1));
    expect_eq(st.root_progress_bps, GOAL_PROGRESS_FULL_BPS);
    expect_eq(st.total_retries, @as(u64, 1));
    expect_eq(st.total_transitions, @as(u64, 3));
    expect_eq(tree.rootId() ?? 0, root);

    // Verify c2 was skipped
    if (n2 := tree.getNode(c2)) {
        expect_true(n2.status == cap.agentmem.GoalStatus.skipped);
    } else {
        expect_true(false);
    }

    tree.reset();
    expect_true(tree.rootId() == null);
    expect_eq(tree.stats().total_nodes, @as(usize, 0));
    expect_eq(tree.stats().total_transitions, @as(u64, 0));
}
