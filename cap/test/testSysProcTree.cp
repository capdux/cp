// Origin: Pure-CP Stdlib Test Suite (lib/sys/procTree.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Process Tree, Subreaper Orphan Adoption & Shutdown Planner:
//   1. Process tree spawning, PGID inheritance, state transitions (`running -> stopping -> zombie -> reaped`)
//   2. Pure-iterative bottom-up graceful shutdown ordering (`planShutdownOrder`: deepest leaves before parents)
//   3. Process-group batch selection (`collectProcessGroup`) excluding zombies/reaped processes
//   4. Subreaper orphan adoption (`markExited` re-parenting to nearest alive subreaper or root init) & `reapZombies`
//   5. Cycle detection (`reparent` descendant cycle & self-spawn), duplicate PID & 64-node capacity / slot recycling
//   6. Global telemetry stats (`ProcTreeStats`) & `cap.sys` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    PROC_TREE_MAX_NODES,
    PROC_TREE_NO_PID,
    PROC_TREE_DEFAULT_INIT_PID,
    ProcNodeState,
    ProcTreeOpStatus,
    ReapedProcInfo,
    formatProcNodeState,
    ProcessTree,
} from "cap:sys";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("ProcTree: spawn, PGID inheritance, markStopping & state labels") {
    expect_true(ProcessTree.init(PROC_TREE_NO_PID, 100) == null);

    let tree = ProcessTree.init(PROC_TREE_DEFAULT_INIT_PID, 1000) ?? return;
    // Spawn supervisor pid=10 with explicit pgid=10, then worker pid=20 inheriting pgid=0 -> 10
    expect_eq(tree.spawn(10, 1, 10, true, 1010), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(20, 10, 0, false, 1020), ProcTreeOpStatus.ok);

    if (n20 := tree.getNode(20)) {
        expect_eq(n20.pid, @as(u32, 20));
        expect_eq(n20.ppid, @as(u32, 10));
        expect_eq(n20.pgid, @as(u32, 10)); // inherited from parent 10
        expect_eq(n20.state, ProcNodeState.running);
        expect_true(n20.isAlive());
    } else {
        expect_true(false);
    }

    expect_eq(tree.markStopping(20), ProcTreeOpStatus.ok);
    // Double markStopping fails with invalid_state
    expect_eq(tree.markStopping(20), ProcTreeOpStatus.invalid_state);
    if (n20s := tree.getNode(20)) {
        expect_eq(n20s.state, ProcNodeState.stopping);
        expect_true(n20s.isAlive());
    } else {
        expect_true(false);
    }

    expect_true(bytesEq(formatProcNodeState(ProcNodeState.running), "running"));
    expect_true(bytesEq(formatProcNodeState(ProcNodeState.stopping), "stopping"));
    expect_true(bytesEq(formatProcNodeState(ProcNodeState.zombie), "zombie"));
    expect_true(bytesEq(formatProcNodeState(ProcNodeState.reaped), "reaped"));
}

test("ProcTree: pure-iterative bottom-up planShutdownOrder (deepest leaves before parents)") {
    let tree = ProcessTree.init(1, 100) ?? return;
    // Tree topology:
    //        1
    //      /   \
    //    10     20
    //   /  \      \
    // 100  101    200
    //  |
    // 1000
    expect_eq(tree.spawn(10, 1, 10, false, 110), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(20, 1, 20, false, 120), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(100, 10, 10, false, 130), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(101, 10, 10, false, 140), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(200, 20, 20, false, 150), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(1000, 100, 10, false, 160), ProcTreeOpStatus.ok);

    // Full tree shutdown plan: depth 3 [1000], depth 2 [200, 101, 100], depth 1 [20, 10], depth 0 [1]
    let out: [16]u32 = [0; 16];
    let out_s: []u32 = out[0..];
    const n = tree.planShutdownOrder(1, *out_s);
    expect_eq(n, @as(usize, 7));
    expect_eq(out[0], @as(u32, 1000)); // deepest leaf first
    expect_eq(out[n - 1], @as(u32, 1)); // root last

    // Verify invariant: every child appears strictly BEFORE its parent in `out[0..n]`
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (node := tree.getNode(out[i])) {
            if (node.ppid != 0) {
                let parent_pos: usize = 0;
                let found_parent = false;
                let j: usize = 0;
                while (j < n) : (j += 1) {
                    if (out[j] == node.ppid) {
                        parent_pos = j;
                        found_parent = true;
                        break;
                    }
                }
                expect_true(found_parent);
                expect_true(i < parent_pos);
            }
        }
    }

    // Subtree shutdown plan rooted at 10 -> [1000, 101, 100, 10]
    const n_sub = tree.planShutdownOrder(10, *out_s);
    expect_eq(n_sub, @as(usize, 4));
    expect_eq(out[0], @as(u32, 1000));
    expect_eq(out[3], @as(u32, 10));
}

test("ProcTree: collectProcessGroup filters alive members in ascending PID order") {
    let tree = ProcessTree.init(1, 100) ?? return;
    expect_eq(tree.spawn(50, 1, 500, false, 110), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(30, 1, 500, false, 120), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(40, 30, 500, false, 130), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(60, 1, 600, false, 140), ProcTreeOpStatus.ok);

    // Mark pid=40 exited -> zombie should NOT appear in alive process group
    expect_eq(tree.markExited(40, 0, 200).status, ProcTreeOpStatus.ok);

    let pg: [8]u32 = [0; 8];
    let pg_s: []u32 = pg[0..];
    const cnt = tree.collectProcessGroup(500, *pg_s);
    expect_eq(cnt, @as(usize, 2));
    expect_eq(pg[0], @as(u32, 30));
    expect_eq(pg[1], @as(u32, 50));
}

test("ProcTree: subreaper orphan adoption & zombie reaping (multi-level fallback)") {
    let tree = ProcessTree.init(1, 100) ?? return;
    // 1 (init/subreaper) -> 10 (subreaper) -> 20 (normal) -> 30, 31 (workers)
    expect_eq(tree.spawn(10, 1, 10, true, 110), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(20, 10, 10, false, 120), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(30, 20, 10, false, 130), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(31, 20, 10, false, 140), ProcTreeOpStatus.ok);

    // 1. Middle parent 20 crashes -> orphans 30 & 31 are adopted by nearest subreaper (10)
    const ex20 = tree.markExited(20, 137, 200);
    expect_eq(ex20.status, ProcTreeOpStatus.ok);
    expect_eq(ex20.reparented_children, @as(usize, 2));
    expect_eq(ex20.new_parent_pid, @as(u32, 10));

    if (n30 := tree.getNode(30)) {
        expect_eq(n30.ppid, @as(u32, 10));
    } else {
        expect_true(false);
    }
    if (n31 := tree.getNode(31)) {
        expect_eq(n31.ppid, @as(u32, 10));
    } else {
        expect_true(false);
    }

    // 2. Worker 31 exits -> subreaper 10 reaps both 20 (exit 137) and 31 (exit 0) in PID order
    expect_eq(tree.markExited(31, 0, 250).status, ProcTreeOpStatus.ok);
    let reaped: [4]ReapedProcInfo = [ReapedProcInfo{}; 4];
    let reaped_s: []ReapedProcInfo = reaped[0..];
    const r_cnt = tree.reapZombies(10, *reaped_s);
    expect_eq(r_cnt, @as(usize, 2));
    expect_eq(reaped[0].pid, @as(u32, 20));
    expect_eq(reaped[0].exit_code, @as(i32, 137));
    expect_eq(reaped[1].pid, @as(u32, 31));
    expect_eq(reaped[1].exit_code, @as(i32, 0));

    // 3. Now subreaper 10 itself exits -> remaining child 30 falls back to root init (pid=1)
    const ex10 = tree.markExited(10, 1, 300);
    expect_eq(ex10.status, ProcTreeOpStatus.ok);
    expect_eq(ex10.reparented_children, @as(usize, 1));
    expect_eq(ex10.new_parent_pid, @as(u32, 1));
    if (n30b := tree.getNode(30)) {
        expect_eq(n30b.ppid, @as(u32, 1));
    } else {
        expect_true(false);
    }
}

test("ProcTree: cycle detection, duplicate PID, capacity limit & reaped slot recycling") {
    let tree = ProcessTree.init(1, 100) ?? return;
    expect_eq(tree.spawn(2, 1, 0, false, 110), ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(3, 2, 0, false, 120), ProcTreeOpStatus.ok);

    // Duplicate PID, self-cycle, nonexistent parent, descendant reparent cycle
    expect_eq(tree.spawn(2, 1, 0, false, 125), ProcTreeOpStatus.duplicate_pid);
    expect_eq(tree.spawn(9, 9, 0, false, 126), ProcTreeOpStatus.cycle_detected);
    expect_eq(tree.spawn(9, 999, 0, false, 127), ProcTreeOpStatus.parent_not_found);
    expect_eq(tree.reparent(2, 3), ProcTreeOpStatus.cycle_detected); // 3 is descendant of 2
    expect_eq(tree.reparent(2, 2), ProcTreeOpStatus.cycle_detected);
    // Root cannot exit or lose subreaper status
    expect_eq(tree.markExited(1, 0, 130).status, ProcTreeOpStatus.invalid_pid);
    expect_eq(tree.setSubreaper(1, false), ProcTreeOpStatus.invalid_state);

    // Fill remaining slots up to 64 total (currently 3 nodes: 1, 2, 3)
    let p: u32 = 100;
    while (p < 161) : (p += 1) {
        expect_eq(tree.spawn(p, 1, 0, false, @as(u64, p)), ProcTreeOpStatus.ok);
    }
    expect_eq(tree.stats().total_nodes, PROC_TREE_MAX_NODES);
    // 65th active node fails with capacity_full
    expect_eq(tree.spawn(999, 1, 0, false, 500), ProcTreeOpStatus.capacity_full);

    // Exit and reap pid=100 -> slot becomes recyclable for pid=999
    expect_eq(tree.markExited(100, 0, 510).status, ProcTreeOpStatus.ok);
    let rbuf: [2]ReapedProcInfo = [ReapedProcInfo{}; 2];
    let rbuf_s: []ReapedProcInfo = rbuf[0..];
    expect_eq(tree.reapZombies(1, *rbuf_s), @as(usize, 1));
    expect_eq(tree.spawn(999, 1, 0, false, 520), ProcTreeOpStatus.ok);
}

test("ProcTree: global telemetry stats & cap.sys facade parity") {
    let tree = cap.sys.ProcessTree.init(1, 1000) ?? return;
    expect_eq(tree.spawn(10, 1, 10, true, 1010), cap.sys.ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(20, 10, 10, false, 1020), cap.sys.ProcTreeOpStatus.ok);
    expect_eq(tree.spawn(30, 20, 10, false, 1030), cap.sys.ProcTreeOpStatus.ok);
    expect_eq(tree.markStopping(30), cap.sys.ProcTreeOpStatus.ok);
    * = tree.markExited(20, 0, 1050); // reparents 30 -> 10

    const st = tree.stats();
    expect_eq(st.root_pid, @as(u32, 1));
    expect_eq(st.capacity, PROC_TREE_MAX_NODES);
    expect_eq(st.total_nodes, @as(usize, 4));
    expect_eq(st.running_count, @as(usize, 2));  // 1, 10
    expect_eq(st.stopping_count, @as(usize, 1)); // 30
    expect_eq(st.zombie_count, @as(usize, 1));   // 20
    expect_eq(st.reaped_count, @as(usize, 0));
    expect_eq(st.subreaper_count, @as(usize, 2)); // 1, 10
    expect_eq(st.reparented_total, @as(u64, 1));
    expect_eq(st.reaped_total, @as(u64, 0));
}
