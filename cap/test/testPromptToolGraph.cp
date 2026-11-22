// Origin: Pure-CP Stdlib Test Suite (lib/prompt/toolGraph.cp) | Created: 2026-11-23
// Production Deep Verification Suite for AI Agent Parallel Tool-Call DAG Orchestrator:
//   1. Multi-read parallel wave coalescing (`read_only` on same/different resources) & readiness lifecycle
//   2. Automatic RAW / WAR / WAW conflict edge inference & serialization on matching `resource_hash`
//   3. `barrier_all` global synchronization isolation (single & consecutive barriers)
//   4. `max_parallel_per_wave` concurrency capping & critical-path vs serial latency speedup
//   5. Explicit dependency cycle detection (3-node cycle, self-loop, and implicit+explicit mixed cycle)
//   6. Transitive downstream failure cascading (`skipped_dep_failed`) with isolated sibling survival
//   7. Capacity/bounds guards, `reset()` reuse & `cap.prompt` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    TOOL_GRAPH_MAX_CALLS,
    TOOL_GRAPH_MAX_DEPS,
    TOOL_GRAPH_MAX_WAVES,
    TOOL_GRAPH_NAME_MAX_LEN,
    ToolAccessMode,
    ToolCallExecState,
    ToolGraphStatus,
    ToolCallGraph,
} from "cap:prompt";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("ToolGraph: multi-read parallel coalescing & readiness lifecycle") {
    let g = ToolCallGraph.init();

    const c0 = g.addCall("read_file_a", 0xAAAA, ToolAccessMode.read_only, 40) ?? 99;
    const c1 = g.addCall("read_file_a_stat", 0xAAAA, ToolAccessMode.read_only, 120) ?? 99;
    const c2 = g.addCall("read_file_b", 0xBBBB, ToolAccessMode.read_only, 80) ?? 99;
    const c3 = g.addCall("read_file_c", 0x0, ToolAccessMode.read_only, 60) ?? 99;

    expect_eq(c0, @as(u16, 0));
    expect_eq(c3, @as(u16, 3));
    expect_eq(g.nodeCount(), @as(usize, 4));

    if (n0 := g.getNode(c0)) {
        expect_true(bytesEq(n0.tool_name(), "read_file_a"));
    } else {
        expect_true(false);
    }

    const plan = g.buildWavePlan(0);
    expect_true(plan.status == ToolGraphStatus.ok);
    expect_eq(plan.wave_count, @as(usize, 1));
    expect_eq(plan.wave_sizes[0], @as(usize, 4));
    expect_eq(plan.auto_conflict_edges, @as(usize, 0));
    expect_eq(plan.critical_path_ms, @as(u32, 120));
    expect_eq(plan.total_serial_ms, @as(u32, 300));

    let ready_buf: [8]u16 = [0; 8];
    let ready_s: []u16 = ready_buf[0..];
    const ready_n = g.collectReadyCalls(*ready_s);
    expect_eq(ready_n, @as(usize, 4));
    expect_eq(ready_buf[0], @as(u16, 0));
    expect_eq(ready_buf[3], @as(u16, 3));

    expect_true(g.markRunning(c0) == ToolGraphStatus.ok);
    expect_eq(g.markResult(c0, true), @as(usize, 0));
    expect_eq(g.markResult(c1, true), @as(usize, 0));
    expect_eq(g.markResult(c2, true), @as(usize, 0));
    expect_eq(g.stats().completed_waves, @as(usize, 0));

    expect_eq(g.markResult(c3, true), @as(usize, 0));
    const st = g.stats();
    expect_eq(st.total_calls, @as(usize, 4));
    expect_eq(st.succeeded, @as(usize, 4));
    expect_eq(st.failed, @as(usize, 0));
    expect_eq(st.skipped, @as(usize, 0));
    expect_eq(st.completed_waves, @as(usize, 1));
}

test("ToolGraph: automatic RAW / WAR / WAW serialization on matching resource_hash") {
    let g = ToolCallGraph.init();
    const res_db: u64 = 0xDEAD_BEEF_0001;

    // Sequence on same resource: R0, R1 -> W2 (WAR) -> W3 (WAW) -> R4, R5 (RAW)
    const r0 = g.addCall("select_users", res_db, ToolAccessMode.read_only, 50) ?? 99;
    const r1 = g.addCall("select_roles", res_db, ToolAccessMode.read_only, 70) ?? 99;
    const w2 = g.addCall("update_schema", res_db, ToolAccessMode.write_exclusive, 100) ?? 99;
    const w3 = g.addCall("seed_admin", res_db, ToolAccessMode.write_exclusive, 80) ?? 99;
    const r4 = g.addCall("verify_users", res_db, ToolAccessMode.read_only, 30) ?? 99;
    const r5 = g.addCall("verify_roles", res_db, ToolAccessMode.read_only, 45) ?? 99;

    const plan = g.buildWavePlan(8);
    expect_true(plan.status == ToolGraphStatus.ok);
    expect_eq(plan.wave_count, @as(usize, 4));

    // Wave 0: [r0, r1]
    expect_eq(plan.wave_sizes[0], @as(usize, 2));
    expect_eq(plan.wave_members[0][0], r0);
    expect_eq(plan.wave_members[0][1], r1);

    // Wave 1: [w2]
    expect_eq(plan.wave_sizes[1], @as(usize, 1));
    expect_eq(plan.wave_members[1][0], w2);

    // Wave 2: [w3]
    expect_eq(plan.wave_sizes[2], @as(usize, 1));
    expect_eq(plan.wave_members[2][0], w3);

    // Wave 3: [r4, r5]
    expect_eq(plan.wave_sizes[3], @as(usize, 2));
    expect_eq(plan.wave_members[3][0], r4);
    expect_eq(plan.wave_members[3][1], r5);

    // Auto edges: R0->W2, R1->W2 (2 WAR) + W2->W3 (1 WAW) + W3->R4, W3->R5 (2 RAW) = 5
    expect_eq(plan.auto_conflict_edges, @as(usize, 5));
    // Critical path: max(50,70) + 100 + 80 + max(30,45) = 70 + 100 + 80 + 45 = 295
    expect_eq(plan.critical_path_ms, @as(u32, 295));
    // Serial total: 50 + 70 + 100 + 80 + 30 + 45 = 375
    expect_eq(plan.total_serial_ms, @as(u32, 375));

    // Verify readiness unlocks step by step
    let ready_buf: [8]u16 = [0; 8];
    let ready_s: []u16 = ready_buf[0..];
    expect_eq(g.collectReadyCalls(*ready_s), @as(usize, 2));
    * = g.markResult(r0, true);
    // w2 still waiting on r1
    expect_eq(g.collectReadyCalls(*ready_s), @as(usize, 1));
    expect_eq(ready_buf[0], r1);

    * = g.markResult(r1, true);
    expect_eq(g.collectReadyCalls(*ready_s), @as(usize, 1));
    expect_eq(ready_buf[0], w2);
}

test("ToolGraph: barrier_all global isolation across independent resources") {
    let g = ToolCallGraph.init();

    const a0 = g.addCall("fetch_docs", 0x101, ToolAccessMode.read_only, 40) ?? 99;
    const a1 = g.addCall("write_tmp", 0x202, ToolAccessMode.write_exclusive, 60) ?? 99;
    const b2 = g.addCall("git_commit_barrier", 0x0, ToolAccessMode.barrier_all, 50) ?? 99;
    const b3 = g.addCall("deploy_barrier", 0x0, ToolAccessMode.barrier_all, 90) ?? 99;
    const c4 = g.addCall("smoke_check_1", 0x303, ToolAccessMode.read_only, 30) ?? 99;
    const c5 = g.addCall("smoke_check_2", 0x404, ToolAccessMode.read_only, 25) ?? 99;

    const plan = g.buildWavePlan(16);
    expect_true(plan.status == ToolGraphStatus.ok);
    expect_eq(plan.wave_count, @as(usize, 4));

    // Wave 0: [a0, a1]
    expect_eq(plan.wave_sizes[0], @as(usize, 2));
    expect_eq(plan.wave_members[0][0], a0);
    expect_eq(plan.wave_members[0][1], a1);
    // Wave 1: [b2] alone
    expect_eq(plan.wave_sizes[1], @as(usize, 1));
    expect_eq(plan.wave_members[1][0], b2);
    // Wave 2: [b3] alone
    expect_eq(plan.wave_sizes[2], @as(usize, 1));
    expect_eq(plan.wave_members[2][0], b3);
    // Wave 3: [c4, c5]
    expect_eq(plan.wave_sizes[3], @as(usize, 2));
    expect_eq(plan.wave_members[3][0], c4);
    expect_eq(plan.wave_members[3][1], c5);

    // Critical path: 60 + 50 + 90 + 30 = 230 vs serial 295
    expect_eq(plan.critical_path_ms, @as(u32, 230));
    expect_eq(plan.total_serial_ms, @as(u32, 295));
    expect_true(plan.auto_conflict_edges >= 5);
}

test("ToolGraph: max_parallel_per_wave concurrency cap & critical-path speedup") {
    let g = ToolCallGraph.init();

    let i: usize = 0;
    while (i < 6) : (i += 1) {
        const res: u64 = @as(u64, i + 1);
        * = g.addCall("parallel_search", res, ToolAccessMode.read_only, 100);
    }

    // Unbounded: all 6 run in 1 wave -> 6x speedup (100ms vs 600ms)
    const p_unbounded = g.buildWavePlan(0);
    expect_true(p_unbounded.status == ToolGraphStatus.ok);
    expect_eq(p_unbounded.wave_count, @as(usize, 1));
    expect_eq(p_unbounded.wave_sizes[0], @as(usize, 6));
    expect_eq(p_unbounded.critical_path_ms, @as(u32, 100));
    expect_eq(p_unbounded.total_serial_ms, @as(u32, 600));

    // Capped at 2 per wave: 3 waves of 2 -> 2x speedup (300ms vs 600ms)
    const p_cap2 = g.buildWavePlan(2);
    expect_true(p_cap2.status == ToolGraphStatus.ok);
    expect_eq(p_cap2.wave_count, @as(usize, 3));
    expect_eq(p_cap2.wave_sizes[0], @as(usize, 2));
    expect_eq(p_cap2.wave_sizes[1], @as(usize, 2));
    expect_eq(p_cap2.wave_sizes[2], @as(usize, 2));
    expect_eq(p_cap2.critical_path_ms, @as(u32, 300));
    expect_eq(p_cap2.total_serial_ms, @as(u32, 600));
}

test("ToolGraph: cycle detection for explicit, self-loop & mixed implicit-explicit cycles") {
    // Case A: 3-node explicit cycle 0 -> 1 -> 2 -> 0
    let g1 = ToolCallGraph.init();
    const n0 = g1.addCall("step_a", 0x1, ToolAccessMode.read_only, 10) ?? 99;
    const n1 = g1.addCall("step_b", 0x2, ToolAccessMode.read_only, 10) ?? 99;
    const n2 = g1.addCall("step_c", 0x3, ToolAccessMode.read_only, 10) ?? 99;
    expect_true(g1.addDependency(n0, n1) == ToolGraphStatus.ok);
    expect_true(g1.addDependency(n1, n2) == ToolGraphStatus.ok);
    expect_true(g1.addDependency(n2, n0) == ToolGraphStatus.ok);

    const p1 = g1.buildWavePlan(4);
    expect_true(p1.status == ToolGraphStatus.cycle_detected);
    expect_eq(p1.wave_count, @as(usize, 0));

    // Case B: self-loop 0 -> 0
    let g2 = ToolCallGraph.init();
    const s0 = g2.addCall("self_dep", 0x10, ToolAccessMode.read_only, 20) ?? 99;
    expect_true(g2.addDependency(s0, s0) == ToolGraphStatus.ok);
    const p2 = g2.buildWavePlan(4);
    expect_true(p2.status == ToolGraphStatus.cycle_detected);

    // Case C: implicit RAW (w0 -> r1) + reversed explicit (r1 -> w0) creates cycle
    let g3 = ToolCallGraph.init();
    const w0 = g3.addCall("write_cfg", 0x999, ToolAccessMode.write_exclusive, 30) ?? 99;
    const r1 = g3.addCall("read_cfg", 0x999, ToolAccessMode.read_only, 15) ?? 99;
    expect_true(g3.addDependency(r1, w0) == ToolGraphStatus.ok);
    const p3 = g3.buildWavePlan(4);
    expect_true(p3.status == ToolGraphStatus.cycle_detected);
}

test("ToolGraph: transitive failure cascade to skipped_dep_failed with isolated sibling survival") {
    let g = ToolCallGraph.init();

    // Diamond DAG: a -> b, a -> c, b -> d, c -> d; plus isolated sibling e
    const a = g.addCall("build_artifact", 0x1, ToolAccessMode.read_only, 100) ?? 99;
    const b = g.addCall("unit_test", 0x2, ToolAccessMode.read_only, 80) ?? 99;
    const c = g.addCall("lint_check", 0x3, ToolAccessMode.read_only, 60) ?? 99;
    const d = g.addCall("publish_pkg", 0x4, ToolAccessMode.read_only, 50) ?? 99;
    const e = g.addCall("notify_metrics", 0x5, ToolAccessMode.read_only, 20) ?? 99;

    expect_true(g.addDependency(a, b) == ToolGraphStatus.ok);
    expect_true(g.addDependency(a, c) == ToolGraphStatus.ok);
    expect_true(g.addDependency(b, d) == ToolGraphStatus.ok);
    expect_true(g.addDependency(c, d) == ToolGraphStatus.ok);

    const plan = g.buildWavePlan(4);
    expect_true(plan.status == ToolGraphStatus.ok);
    expect_eq(plan.wave_count, @as(usize, 3));

    // Wave 0 has [a, e] ready
    let ready_buf: [8]u16 = [0; 8];
    let ready_s: []u16 = ready_buf[0..];
    expect_eq(g.collectReadyCalls(*ready_s), @as(usize, 2));

    // Fail `a` -> cascades to b, c, d (3 nodes skipped), leaves `e` untouched & ready
    const skipped = g.markResult(a, false);
    expect_eq(skipped, @as(usize, 3));

    if (nb := g.getNode(b)) {
        expect_true(nb.state == ToolCallExecState.skipped_dep_failed);
    } else {
        expect_true(false);
    }
    if (nc := g.getNode(c)) {
        expect_true(nc.state == ToolCallExecState.skipped_dep_failed);
    } else {
        expect_true(false);
    }
    if (nd := g.getNode(d)) {
        expect_true(nd.state == ToolCallExecState.skipped_dep_failed);
    } else {
        expect_true(false);
    }

    // Only `e` remains ready
    expect_eq(g.collectReadyCalls(*ready_s), @as(usize, 1));
    expect_eq(ready_buf[0], e);

    expect_eq(g.markResult(e, true), @as(usize, 0));
    const st = g.stats();
    expect_eq(st.total_calls, @as(usize, 5));
    expect_eq(st.succeeded, @as(usize, 1));
    expect_eq(st.failed, @as(usize, 1));
    expect_eq(st.skipped, @as(usize, 3));
    expect_eq(st.completed_waves, @as(usize, 3));
}

test("ToolGraph: capacity guards, invalid IDs, reset reuse & cap.prompt facade parity") {
    expect_eq(TOOL_GRAPH_MAX_CALLS, @as(usize, 32));
    expect_eq(TOOL_GRAPH_MAX_DEPS, @as(usize, 64));
    expect_eq(TOOL_GRAPH_MAX_WAVES, @as(usize, 32));
    expect_eq(TOOL_GRAPH_NAME_MAX_LEN, @as(usize, 48));

    let g = cap.prompt.ToolCallGraph.init();

    // Empty graph plan succeeds cleanly
    const empty_plan = g.buildWavePlan(4);
    expect_true(empty_plan.status == ToolGraphStatus.ok);
    expect_eq(empty_plan.wave_count, @as(usize, 0));

    // Invalid IDs rejected
    expect_true(g.addDependency(0, 1) == ToolGraphStatus.invalid_id);
    expect_true(g.getNode(0) == null);
    expect_eq(g.markResult(99, false), @as(usize, 0));

    // Fill to capacity (32 calls); 33rd returns null
    let idx: usize = 0;
    while (idx < TOOL_GRAPH_MAX_CALLS) : (idx += 1) {
        const id_opt = g.addCall("batch_tool", @as(u64, idx + 1), ToolAccessMode.read_only, 10);
        expect_true(id_opt != null);
    }
    expect_true(g.addCall("overflow_tool", 999, ToolAccessMode.read_only, 10) == null);

    // Duplicate dependency is idempotent
    expect_true(g.addDependency(0, 1) == ToolGraphStatus.ok);
    expect_true(g.addDependency(0, 1) == ToolGraphStatus.ok);

    // Reset clears everything for zero-alloc reuse
    g.reset();
    expect_eq(g.nodeCount(), @as(usize, 0));
    const reused = g.addCall("fresh_call", 0x42, ToolAccessMode.write_exclusive, 25) ?? 99;
    expect_eq(reused, @as(u16, 0));
    const p = g.buildWavePlan(2);
    expect_true(p.status == ToolGraphStatus.ok);
    expect_eq(p.wave_count, @as(usize, 1));
}
