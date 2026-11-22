// Origin: Pure-CP Stdlib Test Suite (lib/graph/max_flow.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Dinic Max-Flow / Min-Cut & Bipartite Matching:
//   1. Classic CLRS 6-node network: exact max_flow = 23 + Max-Flow Min-Cut Theorem equality verification
//   2. Diamond network requiring reverse residual flow cancellation to reach optimal flow
//   3. Disconnected source/sink (0 flow), self-loops & zero-capacity edges
//   4. Multiple parallel edges summing capacities accurately
//   5. solveBipartiteMatching: multi-Agent Task-Worker maximum matching with augmenting path & duplicate edge dedup
//   6. Large network near FLOW_MAX_NODES (64) & edge_flow conservation at intermediate vertices
//   7. Boundary guards: node_count 0 / >64, out-of-bounds u/v, negative capacity, edge pool cap, source == sink

import { test, expect_eq } from "cap:test";
import {
    FLOW_MAX_NODES,
    FLOW_MAX_EDGES,
    MaxFlowResult,
    BipartiteMatchPair,
    DinicFlowNetwork,
    solveBipartiteMatching,
} from "cap:graph";

// ============================================================================
// Test 1: Classic CLRS 6-Node Network + Max-Flow Min-Cut Theorem Equality
// ============================================================================
test("max_flow: CLRS 6-node network exact max_flow=23 & min-cut theorem equality", () => {
    expect_eq(FLOW_MAX_NODES, 64);
    expect_eq(FLOW_MAX_EDGES, 512);

    let net = DinicFlowNetwork.init(6) ?? return;
    // CLRS Ch. 26 standard network (0=s, 5=t), known max flow = 23
    const u_arr: [10]usize = [0, 0, 1, 2, 1, 3, 2, 4, 3, 4];
    const v_arr: [10]usize = [1, 2, 2, 1, 3, 2, 4, 3, 5, 5];
    const c_arr: [10]i64 = [16, 13, 10, 4, 12, 9, 14, 7, 20, 4];

    let i: usize = 0;
    while (i < 10) : (i += 1) {
        expect_eq(net.add_edge(u_arr[i], v_arr[i], c_arr[i]), true);
    }

    const res: MaxFlowResult = net.solve(0, 5);
    expect_eq(res.valid, true);
    expect_eq(res.max_flow, 23);
    expect_eq(res.bfs_phases >= 1, true);

    // Source 0 must be in S, sink 5 must be in T
    expect_eq(res.isInSourceCut(0), true);
    expect_eq(res.isInSourceCut(5), false);
    // Out-of-bounds node query safely returns false
    expect_eq(res.isInSourceCut(64), false);

    // Verify Max-Flow == Min-Cut capacity: sum of capacities of edges crossing S -> T must equal 23
    let cut_cap: i64 = 0;
    let k: usize = 0;
    while (k < 10) : (k += 1) {
        if (res.isInSourceCut(u_arr[k]) && !res.isInSourceCut(v_arr[k])) {
            cut_cap += c_arr[k];
            // Every forward edge crossing S -> T must be saturated
            expect_eq(net.edge_flow(k), c_arr[k]);
        }
    }
    expect_eq(cut_cap, res.max_flow);
});

// ============================================================================
// Test 2: Reverse Residual Edge Cancellation (Back-Flow Augmentation)
// ============================================================================
test("max_flow: diamond network requiring reverse residual flow cancellation", () => {
    // 0 -> 1 (1), 0 -> 2 (1), 1 -> 2 (1), 1 -> 3 (1), 2 -> 3 (1)
    // Even if greedy path 0->1->2->3 saturates 1->2 first, reverse residual edge 2->1 allows
    // 0->2->1->3 to cancel flow on 1->2 and reach optimal max_flow = 2.
    let net = DinicFlowNetwork.init(4) ?? return;
    expect_eq(net.add_edge(0, 1, 1), true);
    expect_eq(net.add_edge(0, 2, 1), true);
    expect_eq(net.add_edge(1, 2, 1), true);
    expect_eq(net.add_edge(1, 3, 1), true);
    expect_eq(net.add_edge(2, 3, 1), true);

    const res = net.solve(0, 3);
    expect_eq(res.valid, true);
    expect_eq(res.max_flow, 2);
    expect_eq(net.edge_flow(0), 1); // 0->1
    expect_eq(net.edge_flow(1), 1); // 0->2
    expect_eq(net.edge_flow(2), 0); // 1->2 net flow cancelled to 0
    expect_eq(net.edge_flow(3), 1); // 1->3
    expect_eq(net.edge_flow(4), 1); // 2->3
});

// ============================================================================
// Test 3: Disconnected Source/Sink, Self-Loops & Zero-Capacity Edges
// ============================================================================
test("max_flow: disconnected graph, self-loops & zero-capacity edges", () => {
    let net = DinicFlowNetwork.init(4) ?? return;
    expect_eq(net.add_edge(0, 0, 100), true); // self-loop
    expect_eq(net.add_edge(0, 1, 10), true);
    expect_eq(net.add_edge(1, 2, 0), true);   // zero-capacity bottleneck
    expect_eq(net.add_edge(2, 3, 10), true);

    const res = net.solve(0, 3);
    expect_eq(res.valid, true);
    expect_eq(res.max_flow, 0);
    expect_eq(res.bfs_phases, 0);
    expect_eq(res.isInSourceCut(0), true);
    expect_eq(res.isInSourceCut(1), true);
    expect_eq(res.isInSourceCut(2), false);
    expect_eq(res.isInSourceCut(3), false);
});

// ============================================================================
// Test 4: Multiple Parallel Edges Summing Flow Accurately
// ============================================================================
test("max_flow: multiple parallel edges sum capacities cleanly", () => {
    let net = DinicFlowNetwork.init(3) ?? return;
    expect_eq(net.add_edge(0, 1, 5), true);
    expect_eq(net.add_edge(0, 1, 7), true);
    expect_eq(net.add_edge(0, 1, 8), true); // total 0->1 = 20
    expect_eq(net.add_edge(1, 2, 9), true);
    expect_eq(net.add_edge(1, 2, 6), true); // total 1->2 = 15

    const res = net.solve(0, 2);
    expect_eq(res.valid, true);
    expect_eq(res.max_flow, 15);
    expect_eq(net.edge_flow(0) + net.edge_flow(1) + net.edge_flow(2), 15);
    expect_eq(net.edge_flow(3) + net.edge_flow(4), 15);
});

// ============================================================================
// Test 5: solveBipartiteMatching (Augmenting Path + Duplicate Edge Dedup)
// ============================================================================
test("max_flow: solveBipartiteMatching optimal assignment & duplicate edge dedup", () => {
    // 4 Agent tasks (L=0..3), 4 GPU workers (R=0..3)
    // Edges include duplicates (0,0) twice and an out-of-bounds edge (9,9) to test guards
    const el: [8]u16 = [0, 0, 0, 1, 2, 2, 3, 9];
    const er: [8]u16 = [0, 0, 1, 0, 1, 2, 3, 9];
    let pairs_buf: [4]BipartiteMatchPair = [BipartiteMatchPair{}; 4];
    let pairs_s: []BipartiteMatchPair = pairs_buf[0..];

    const n = solveBipartiteMatching(4, 4, el[0..], er[0..], pairs_s);
    expect_eq(n, 4);

    // Verify every left and every right vertex is matched at most once (valid matching)
    let left_mask: u64 = 0;
    let right_mask: u64 = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const l_bit: u64 = (1 as u64) << @intCast(pairs_buf[i].left_idx);
        const r_bit: u64 = (1 as u64) << @intCast(pairs_buf[i].right_idx);
        expect_eq((left_mask & l_bit) == 0, true);
        expect_eq((right_mask & r_bit) == 0, true);
        left_mask = left_mask | l_bit;
        right_mask = right_mask | r_bit;
    }
    expect_eq(left_mask, 0xF);
    expect_eq(right_mask, 0xF);
});

// ============================================================================
// Test 6: 64-Node Chain Network (Max Capacity) & Flow Conservation
// ============================================================================
test("max_flow: 64-node chain reaches FLOW_MAX_NODES limit with exact bottleneck", () => {
    let net = DinicFlowNetwork.init(FLOW_MAX_NODES) ?? return;
    let u: usize = 0;
    while (u + 1 < FLOW_MAX_NODES) : (u += 1) {
        // Bottleneck at edge 30 -> 31 with capacity 42, others 100
        const cap: i64 = if (u == 30) 42 else 100;
        expect_eq(net.add_edge(u, u + 1, cap), true);
    }

    const res = net.solve(0, FLOW_MAX_NODES - 1);
    expect_eq(res.valid, true);
    expect_eq(res.max_flow, 42);
    expect_eq(res.isInSourceCut(0), true);
    expect_eq(res.isInSourceCut(30), true);
    expect_eq(res.isInSourceCut(31), false);
    expect_eq(res.isInSourceCut(63), false);

    // Every edge along the chain carries exact flow 42
    let e: usize = 0;
    while (e + 1 < FLOW_MAX_NODES) : (e += 1) {
        expect_eq(net.edge_flow(e), 42);
    }
});

// ============================================================================
// Test 7: Boundary Guards (Invalid Node Count, Negative Cap, Edge Pool Cap, source == sink)
// ============================================================================
test("max_flow: boundary guards for init, add_edge, edge pool cap & solve", () => {
    // Invalid node counts
    expect_eq(DinicFlowNetwork.init(0) == null, true);
    expect_eq(DinicFlowNetwork.init(FLOW_MAX_NODES + 1) == null, true);

    let net = DinicFlowNetwork.init(3) ?? return;
    // Out-of-bounds vertex & negative capacity rejected
    expect_eq(net.add_edge(3, 0, 10), false);
    expect_eq(net.add_edge(0, 3, 10), false);
    expect_eq(net.add_edge(0, 1, -5), false);

    // source == sink or out-of-bounds solve returns valid = false
    expect_eq(net.solve(1, 1).valid, false);
    expect_eq(net.solve(0, 3).valid, false);
    expect_eq(net.solve(3, 0).valid, false);

    // Fill edge pool to FLOW_MAX_EDGES / 2 = 256 user edges -> 257th fails cleanly
    let added: usize = 0;
    while (added < 256) : (added += 1) {
        expect_eq(net.add_edge(0, 1, 1), true);
    }
    expect_eq(net.add_edge(0, 1, 1), false);
    expect_eq(net.solve(0, 1).max_flow, 256);

    // Out-of-bounds edge_flow returns 0 safely
    expect_eq(net.edge_flow(999), 0);

    // Bipartite matching boundary guards (0 vertices or >62 total vertices)
    const empty_e: [1]u16 = [0];
    let out_p: [2]BipartiteMatchPair = [BipartiteMatchPair{}; 2];
    let out_s: []BipartiteMatchPair = out_p[0..];
    expect_eq(solveBipartiteMatching(0, 2, empty_e[0..0], empty_e[0..0], out_s), 0);
    expect_eq(solveBipartiteMatching(32, 32, empty_e[0..0], empty_e[0..0], out_s), 0); // 2+32+32=66 > 64
});
