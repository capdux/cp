// Source: cp/cap/test/test_maxflow_production_deep.cp (Production deep tests for cap:maxflow)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NO_EDGE,
    INF_FLOW,
    FlowEdge,
    FlowGraph,
    MinCostResult,
    dinicMaxFlow,
    minCutReachable,
    minCostMaxFlow,
} from "cap:maxflow";

fn computeCutCapacity(g: FlowGraph, reachable: []const u8): i64 {
    let cut_cap: i64 = 0;
    let u: usize = 0;
    while (u < g.num_nodes) {
        if (reachable[u] == 1) {
            let e_idx = g.head[u];
            while (e_idx != NO_EDGE) {
                const eu = e_idx as usize;
                // Only count original forward edges (even indices) crossing S -> T
                if ((eu & 1) == 0) {
                    const e = g.edges[eu];
                    if (reachable[e.to] == 0) {
                        cut_cap += e.cap;
                    }
                }
                e_idx = g.edges[eu].rev_next;
            }
        }
        u += 1;
    }
    return cut_cap;
}

test("1. CLRS 6-node classic network: Dinic max-flow == 23, S-T min-cut capacity == 23 & resetFlows", () => {
    // CLRS Chapter 26 classic 6-vertex network (s=0, v1=1, v2=2, v3=3, v4=4, t=5)
    let head: [6]i32 = std.mem.zeroes([6]i32);
    let edges: [32]FlowEdge = std.mem.zeroes([32]FlowEdge);
    let g = FlowGraph.init(6, head[0..], edges[0..]) ?? unreachable;

    expect_eq(g.addEdge(0, 1, 16), true);
    expect_eq(g.addEdge(0, 2, 13), true);
    expect_eq(g.addEdge(1, 2, 10), true);
    expect_eq(g.addEdge(2, 1, 4), true);
    expect_eq(g.addEdge(1, 3, 12), true);
    expect_eq(g.addEdge(2, 4, 14), true);
    expect_eq(g.addEdge(3, 2, 9), true);
    expect_eq(g.addEdge(4, 3, 7), true);
    expect_eq(g.addEdge(3, 5, 20), true);
    expect_eq(g.addEdge(4, 5, 4), true);
    expect_eq(g.edge_count, 20 as usize);

    let level_buf: [6]i32 = std.mem.zeroes([6]i32);
    let iter_buf: [6]i32 = std.mem.zeroes([6]i32);
    let queue_buf: [6]usize = std.mem.zeroes([6]usize);
    let level_s: []i32 = level_buf[0..];
    let iter_s: []i32 = iter_buf[0..];
    let queue_s: []usize = queue_buf[0..];

    const mf = dinicMaxFlow(&g, 0, 5, &level_s, &iter_s, &queue_s) ?? unreachable;
    expect_eq(mf, 23 as i64);

    // Max-Flow Min-Cut theorem verification
    let reach_buf: [6]u8 = std.mem.zeroes([6]u8);
    let reach_s: []u8 = reach_buf[0..];
    const s_count = minCutReachable(g, 0, &reach_s, &queue_s) ?? unreachable;
    expect_eq(s_count, 4 as usize);
    expect_eq(reach_buf[0], 1 as u8);
    expect_eq(reach_buf[1], 1 as u8);
    expect_eq(reach_buf[2], 1 as u8);
    expect_eq(reach_buf[3], 0 as u8);
    expect_eq(reach_buf[4], 1 as u8);
    expect_eq(reach_buf[5], 0 as u8);
    expect_eq(computeCutCapacity(g, reach_buf[0..]), 23 as i64);

    // Calling dinicMaxFlow again without reset yields 0 additional flow
    const mf_again = dinicMaxFlow(&g, 0, 5, &level_s, &iter_s, &queue_s) ?? unreachable;
    expect_eq(mf_again, 0 as i64);

    // After resetFlows(), re-solving produces identical max-flow == 23
    g.resetFlows();
    const mf_reset = dinicMaxFlow(&g, 0, 5, &level_s, &iter_s, &queue_s) ?? unreachable;
    expect_eq(mf_reset, 23 as i64);
});

test("2. Bipartite maximum matching (4x4 perfect matching == 4 & constrained 4x4 imperfect == 3)", () => {
    // Nodes: source=0, left L0..L3 = 1..4, right R0..R3 = 5..8, sink=9
    let head: [10]i32 = std.mem.zeroes([10]i32);
    let edges: [64]FlowEdge = std.mem.zeroes([64]FlowEdge);
    let g = FlowGraph.init(10, head[0..], edges[0..]) ?? unreachable;

    let i: usize = 0;
    while (i < 4) {
        expect_eq(g.addEdge(0, 1 + i, 1), true);
        expect_eq(g.addEdge(5 + i, 9, 1), true);
        i += 1;
    }

    // Perfect matching edges: L0->{R0,R1}, L1->{R1,R2}, L2->{R2,R3}, L3->{R3,R0}
    expect_eq(g.addEdge(1, 5, 1), true);
    expect_eq(g.addEdge(1, 6, 1), true);
    expect_eq(g.addEdge(2, 6, 1), true);
    expect_eq(g.addEdge(2, 7, 1), true);
    expect_eq(g.addEdge(3, 7, 1), true);
    expect_eq(g.addEdge(3, 8, 1), true);
    expect_eq(g.addEdge(4, 8, 1), true);
    expect_eq(g.addEdge(4, 5, 1), true);

    let level_buf: [10]i32 = std.mem.zeroes([10]i32);
    let iter_buf: [10]i32 = std.mem.zeroes([10]i32);
    let queue_buf: [10]usize = std.mem.zeroes([10]usize);
    let level_s: []i32 = level_buf[0..];
    let iter_s: []i32 = iter_buf[0..];
    let queue_s: []usize = queue_buf[0..];

    const perfect_match = dinicMaxFlow(&g, 0, 9, &level_s, &iter_s, &queue_s) ?? unreachable;
    expect_eq(perfect_match, 4 as i64);

    // Imperfect 4x4 bipartite graph where Hall's condition fails on {L2, L3} -> {R2} only
    let head2: [10]i32 = std.mem.zeroes([10]i32);
    let edges2: [64]FlowEdge = std.mem.zeroes([64]FlowEdge);
    let g2 = FlowGraph.init(10, head2[0..], edges2[0..]) ?? unreachable;

    i = 0;
    while (i < 4) {
        expect_eq(g2.addEdge(0, 1 + i, 1), true);
        expect_eq(g2.addEdge(5 + i, 9, 1), true);
        i += 1;
    }
    // L0->{R0,R1}, L1->{R1}, L2->{R2}, L3->{R2} => R3 isolated, L2 & L3 contend for R2 => max matching == 3
    expect_eq(g2.addEdge(1, 5, 1), true);
    expect_eq(g2.addEdge(1, 6, 1), true);
    expect_eq(g2.addEdge(2, 6, 1), true);
    expect_eq(g2.addEdge(3, 7, 1), true);
    expect_eq(g2.addEdge(4, 7, 1), true);

    const imperfect_match = dinicMaxFlow(&g2, 0, 9, &level_s, &iter_s, &queue_s) ?? unreachable;
    expect_eq(imperfect_match, 3 as i64);
});

test("3. Min-Cost Max-Flow (MCMF): multi-path optimal cost & residual reverse-edge rerouting", () => {
    // Part A: Multi-path network with cheap bottleneck and costlier bypass
    // 0 -> 1 (cap 2, cost 1), 0 -> 2 (cap 2, cost 5)
    // 1 -> 2 (cap 1, cost 1), 1 -> 3 (cap 1, cost 4)
    // 2 -> 3 (cap 3, cost 1)
    // Paths:
    //   0->1->2->3: flow 1, unit cost 1+1+1 = 3  (cost 3)
    //   0->1->3:    flow 1, unit cost 1+4   = 5  (cost 5)
    //   0->2->3:    flow 2, unit cost 5+1   = 6  (cost 12)
    // Total max flow = 4, min cost = 3 + 5 + 12 = 20
    let head: [4]i32 = std.mem.zeroes([4]i32);
    let edges: [20]FlowEdge = std.mem.zeroes([20]FlowEdge);
    let g = FlowGraph.init(4, head[0..], edges[0..]) ?? unreachable;

    expect_eq(g.addCostEdge(0, 1, 2, 1), true);
    expect_eq(g.addCostEdge(0, 2, 2, 5), true);
    expect_eq(g.addCostEdge(1, 2, 1, 1), true);
    expect_eq(g.addCostEdge(1, 3, 1, 4), true);
    expect_eq(g.addCostEdge(2, 3, 3, 1), true);

    let dist_buf: [4]i64 = std.mem.zeroes([4]i64);
    let pn_buf: [4]i32 = std.mem.zeroes([4]i32);
    let pe_buf: [4]i32 = std.mem.zeroes([4]i32);
    let iq_buf: [4]u8 = std.mem.zeroes([4]u8);
    let q_buf: [4]usize = std.mem.zeroes([4]usize);
    let dist_s: []i64 = dist_buf[0..];
    let pn_s: []i32 = pn_buf[0..];
    let pe_s: []i32 = pe_buf[0..];
    let iq_s: []u8 = iq_buf[0..];
    let q_s: []usize = q_buf[0..];

    const res = minCostMaxFlow(&g, 0, 3, &dist_s, &pn_s, &pe_s, &iq_s, &q_s) ?? unreachable;
    expect_eq(res.flow, 4 as i64);
    expect_eq(res.cost, 20 as i64);

    // Part B: Classic residual reverse-edge cancellation test (Suurballe trap)
    // 0 -> 1 (cap 1, cost 0), 1 -> 2 (cap 1, cost 0), 2 -> 3 (cap 1, cost 0)  [first shortest path: 0->1->2->3, cost 0]
    // 0 -> 2 (cap 1, cost 5), 1 -> 3 (cap 1, cost 5)
    // Second augmentation MUST cancel (1->2) via negative reverse edge (2->1, cost 0):
    //   0 -> 2 -> 1 -> 3 (cost 5 + 0 + 5 = 10)
    // Final optimal decomposition: (0->1->3 cost 5) + (0->2->3 cost 5) => flow 2, total cost 10
    let head2: [4]i32 = std.mem.zeroes([4]i32);
    let edges2: [20]FlowEdge = std.mem.zeroes([20]FlowEdge);
    let g2 = FlowGraph.init(4, head2[0..], edges2[0..]) ?? unreachable;

    expect_eq(g2.addCostEdge(0, 1, 1, 0), true);
    expect_eq(g2.addCostEdge(1, 2, 1, 0), true);
    expect_eq(g2.addCostEdge(2, 3, 1, 0), true);
    expect_eq(g2.addCostEdge(0, 2, 1, 5), true);
    expect_eq(g2.addCostEdge(1, 3, 1, 5), true);

    const res2 = minCostMaxFlow(&g2, 0, 3, &dist_s, &pn_s, &pe_s, &iq_s, &q_s) ?? unreachable;
    expect_eq(res2.flow, 2 as i64);
    expect_eq(res2.cost, 10 as i64);
});

test("4. Anti-parallel edges, zero-capacity edges, self-loops & disconnected graph (max-flow == 0)", () => {
    // Disconnected network with anti-parallel edges inside component {0,1} and zero-cap bridge to {2,3}
    let head: [4]i32 = std.mem.zeroes([4]i32);
    let edges: [20]FlowEdge = std.mem.zeroes([20]FlowEdge);
    let g = FlowGraph.init(4, head[0..], edges[0..]) ?? unreachable;

    // Anti-parallel edges between 0 and 1 + self-loop on 0
    expect_eq(g.addEdge(0, 1, 10), true);
    expect_eq(g.addEdge(1, 0, 7), true);
    expect_eq(g.addEdge(0, 0, 5), true);
    // Zero-capacity bridge to component {2,3}
    expect_eq(g.addEdge(1, 2, 0), true);
    expect_eq(g.addEdge(2, 3, 10), true);

    let level_buf: [4]i32 = std.mem.zeroes([4]i32);
    let iter_buf: [4]i32 = std.mem.zeroes([4]i32);
    let queue_buf: [4]usize = std.mem.zeroes([4]usize);
    let level_s: []i32 = level_buf[0..];
    let iter_s: []i32 = iter_buf[0..];
    let queue_s: []usize = queue_buf[0..];

    const mf_disc = dinicMaxFlow(&g, 0, 3, &level_s, &iter_s, &queue_s) ?? unreachable;
    expect_eq(mf_disc, 0 as i64);

    let reach_buf: [4]u8 = std.mem.zeroes([4]u8);
    let reach_s: []u8 = reach_buf[0..];
    const s_cnt = minCutReachable(g, 0, &reach_s, &queue_s) ?? unreachable;
    expect_eq(s_cnt, 2 as usize);
    expect_eq(reach_buf[0], 1 as u8);
    expect_eq(reach_buf[1], 1 as u8);
    expect_eq(reach_buf[2], 0 as u8);
    expect_eq(reach_buf[3], 0 as u8);
    expect_eq(computeCutCapacity(g, reach_buf[0..]), 0 as i64);

    // Now add a positive bridge 1 -> 2 (cap 6) and re-solve
    expect_eq(g.addEdge(1, 2, 6), true);
    const mf_conn = dinicMaxFlow(&g, 0, 3, &level_s, &iter_s, &queue_s) ?? unreachable;
    expect_eq(mf_conn, 6 as i64);
});

test("5. Defensive validation: source==sink, out-of-range nodes, negative cap, edge-pool overflow & undersized scratch", () => {
    let head_small: [2]i32 = std.mem.zeroes([2]i32);
    let edges_small: [4]FlowEdge = std.mem.zeroes([4]FlowEdge);

    // Invalid init: num_nodes == 0 or head.len < num_nodes
    expect_eq(FlowGraph.init(0, head_small[0..], edges_small[0..]) == null, true);
    expect_eq(FlowGraph.init(3, head_small[0..], edges_small[0..]) == null, true);

    let g = FlowGraph.init(2, head_small[0..], edges_small[0..]) ?? unreachable;

    // Out-of-range vertices & negative capacity rejected
    expect_eq(g.addEdge(2, 0, 5), false);
    expect_eq(g.addEdge(0, 2, 5), false);
    expect_eq(g.addEdge(0, 1, -1), false);
    expect_eq(g.addCostEdge(0, 1, -5, 10), false);

    // Edge pool capacity is 4 half-edges => 2 directed edges fit, 3rd overflows
    expect_eq(g.addEdge(0, 1, 10), true);
    expect_eq(g.addEdge(0, 1, 5), true);
    expect_eq(g.addEdge(0, 1, 1), false);
    expect_eq(g.edge_count, 4 as usize);

    let level_buf: [2]i32 = std.mem.zeroes([2]i32);
    let iter_buf: [2]i32 = std.mem.zeroes([2]i32);
    let queue_buf: [2]usize = std.mem.zeroes([2]usize);
    let level_s: []i32 = level_buf[0..];
    let iter_s: []i32 = iter_buf[0..];
    let queue_s: []usize = queue_buf[0..];

    // source == sink or out-of-range source/sink => null
    expect_eq(dinicMaxFlow(&g, 0, 0, &level_s, &iter_s, &queue_s) == null, true);
    expect_eq(dinicMaxFlow(&g, 0, 2, &level_s, &iter_s, &queue_s) == null, true);
    expect_eq(dinicMaxFlow(&g, 2, 1, &level_s, &iter_s, &queue_s) == null, true);

    // Undersized scratch slice => null
    let short_lvl: []i32 = level_buf[0..1];
    expect_eq(dinicMaxFlow(&g, 0, 1, &short_lvl, &iter_s, &queue_s) == null, true);

    // minCutReachable out-of-range source or undersized output => null
    let reach_buf: [2]u8 = std.mem.zeroes([2]u8);
    let reach_s: []u8 = reach_buf[0..];
    let short_reach: []u8 = reach_buf[0..1];
    expect_eq(minCutReachable(g, 2, &reach_s, &queue_s) == null, true);
    expect_eq(minCutReachable(g, 0, &short_reach, &queue_s) == null, true);

    // minCostMaxFlow invalid source==sink & undersized scratch => null
    let dist_buf: [2]i64 = std.mem.zeroes([2]i64);
    let pn_buf: [2]i32 = std.mem.zeroes([2]i32);
    let pe_buf: [2]i32 = std.mem.zeroes([2]i32);
    let iq_buf: [2]u8 = std.mem.zeroes([2]u8);
    let dist_s: []i64 = dist_buf[0..];
    let pn_s: []i32 = pn_buf[0..];
    let pe_s: []i32 = pe_buf[0..];
    let iq_s: []u8 = iq_buf[0..];
    let short_dist: []i64 = dist_buf[0..1];

    expect_eq(minCostMaxFlow(&g, 1, 1, &dist_s, &pn_s, &pe_s, &iq_s, &queue_s) == null, true);
    expect_eq(minCostMaxFlow(&g, 0, 1, &short_dist, &pn_s, &pe_s, &iq_s, &queue_s) == null, true);

    // Valid call on the parallel edges (10 + 5 == 15) succeeds
    const ok_flow = dinicMaxFlow(&g, 0, 1, &level_s, &iter_s, &queue_s) ?? unreachable;
    expect_eq(ok_flow, 15 as i64);
});

test("6. Large 64-bit capacities & FlowEdge.residual() accuracy without i32 overflow", () => {
    let head: [4]i32 = std.mem.zeroes([4]i32);
    let edges: [16]FlowEdge = std.mem.zeroes([16]FlowEdge);
    let g = FlowGraph.init(4, head[0..], edges[0..]) ?? unreachable;

    const ten_billion: i64 = 10000000000;
    const seven_billion: i64 = 7000000000;
    expect_eq(g.addCostEdge(0, 1, ten_billion, 2), true);
    expect_eq(g.addCostEdge(1, 3, seven_billion, 3), true);
    expect_eq(g.addCostEdge(0, 2, seven_billion, 1), true);
    expect_eq(g.addCostEdge(2, 3, ten_billion, 4), true);

    expect_eq(g.edges[0].residual(), ten_billion);
    expect_eq(INF_FLOW > ten_billion, true);

    let dist_buf: [4]i64 = std.mem.zeroes([4]i64);
    let pn_buf: [4]i32 = std.mem.zeroes([4]i32);
    let pe_buf: [4]i32 = std.mem.zeroes([4]i32);
    let iq_buf: [4]u8 = std.mem.zeroes([4]u8);
    let q_buf: [4]usize = std.mem.zeroes([4]usize);
    let dist_s: []i64 = dist_buf[0..];
    let pn_s: []i32 = pn_buf[0..];
    let pe_s: []i32 = pe_buf[0..];
    let iq_s: []u8 = iq_buf[0..];
    let q_s: []usize = q_buf[0..];

    const res = minCostMaxFlow(&g, 0, 3, &dist_s, &pn_s, &pe_s, &iq_s, &q_s) ?? unreachable;
    // Path 0->1->3: 7B * (2+3) = 35B; Path 0->2->3: 7B * (1+4) = 35B => flow 14B, cost 70B
    expect_eq(res.flow, 14000000000 as i64);
    expect_eq(res.cost, 70000000000 as i64);
    // Residual on edge 0->1 should be 10B - 7B = 3B
    expect_eq(g.edges[0].residual(), 3000000000 as i64);
});
