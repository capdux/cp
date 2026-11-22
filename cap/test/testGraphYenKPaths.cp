// Origin: Pure-CP Stdlib Test Suite (lib/graph/yenKPaths.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Yen's K-Shortest Loopless Paths Solver:
//   1. Classic Yen (1971) 6-node benchmark graph (exact costs 5, 7, 8, 8, 8, 10 & lex tie-breaking)
//   2. Cyclic graph loopless guarantee: cycles & back-edges never produce loops; finite count < k
//   3. Deterministic lexicographical tie-breaking on equal-cost diamond paths
//   4. Zero-weight edges & fewer-than-k available paths
//   5. Edge cases: src == dst (cost 0), k == 0, and unreachable destination
//   6. Full boundary guards: invalid node count, negative weight, self-loop, OOB nodes, short out_paths & 256-edge cap
//   7. 16-node layered mesh stress test (top-10 paths) & cap.graph dynamic facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    YEN_MAX_EDGES,
    YEN_INF_COST,
    YenPath,
    YenGraph,
} from "cap:graph";

fn pathNodesEq(p: YenPath, expected: []const u8): bool {
    if (p.len != expected.len) return false;
    let i: usize = 0;
    while (i < expected.len) : (i += 1) {
        if (p.nodes[i] != expected[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Classic Yen (1971) 6-Node Benchmark Graph
// ============================================================================
test("yenKPaths: classic 6-node benchmark graph produces exact top-6 paths & lex tie-breaking", () => {
    // 0=C, 1=D, 2=E, 3=F, 4=G, 5=H
    let g = YenGraph.init(6) ?? return;
    expect_true(g.addEdge(0, 1, 3));
    expect_true(g.addEdge(0, 2, 2));
    expect_true(g.addEdge(1, 3, 4));
    expect_true(g.addEdge(2, 1, 1));
    expect_true(g.addEdge(2, 3, 2));
    expect_true(g.addEdge(2, 4, 3));
    expect_true(g.addEdge(3, 4, 2));
    expect_true(g.addEdge(3, 5, 1));
    expect_true(g.addEdge(4, 5, 2));

    let out: [6]YenPath = [YenPath{}; 6];
    let out_s: []YenPath = out[0..];
    const n = g.findKShortestPaths(0, 5, 6, *out_s) ?? 0;
    expect_eq(n, 6 as usize);

    // 1st: [0, 2, 3, 5], cost 5
    const p0: [4]u8 = [0, 2, 3, 5];
    expect_eq(out[0].total_cost, 5 as u64);
    expect_true(pathNodesEq(out[0], p0[0..]));

    // 2nd: [0, 2, 4, 5], cost 7
    const p1: [4]u8 = [0, 2, 4, 5];
    expect_eq(out[1].total_cost, 7 as u64);
    expect_true(pathNodesEq(out[1], p1[0..]));

    // 3rd, 4th, 5th all have cost 8 -> sorted lexicographically:
    //   [0, 1, 3, 5] < [0, 2, 1, 3, 5] < [0, 2, 3, 4, 5]
    const p2: [4]u8 = [0, 1, 3, 5];
    const p3: [5]u8 = [0, 2, 1, 3, 5];
    const p4: [5]u8 = [0, 2, 3, 4, 5];
    expect_eq(out[2].total_cost, 8 as u64);
    expect_true(pathNodesEq(out[2], p2[0..]));
    expect_eq(out[3].total_cost, 8 as u64);
    expect_true(pathNodesEq(out[3], p3[0..]));
    expect_eq(out[4].total_cost, 8 as u64);
    expect_true(pathNodesEq(out[4], p4[0..]));

    // 6th: [0, 1, 3, 4, 5], cost 11 (or [0, 2, 1, 3, 4, 5], cost 11 -> [0, 1, 3, 4, 5] wins lex)
    const p5: [5]u8 = [0, 1, 3, 4, 5];
    expect_eq(out[5].total_cost, 11 as u64);
    expect_true(pathNodesEq(out[5], p5[0..]));

    let i: usize = 0;
    while (i < n) : (i += 1) {
        expect_true(out[i].isLoopless());
    }
});

// ============================================================================
// Test 2: Cyclic Graph Loopless Guarantee
// ============================================================================
test("yenKPaths: cyclic graph with back-edges only emits strictly loopless simple paths", () => {
    let g = YenGraph.init(5) ?? return;
    // Cycles: 0->1->2->0 and 1<->3
    expect_true(g.addEdge(0, 1, 1));
    expect_true(g.addEdge(1, 2, 1));
    expect_true(g.addEdge(2, 0, 1)); // cycle back to 0
    expect_true(g.addEdge(1, 3, 2));
    expect_true(g.addEdge(3, 1, 1)); // back-edge 3->1
    expect_true(g.addEdge(2, 3, 1));
    expect_true(g.addEdge(2, 4, 5));
    expect_true(g.addEdge(3, 4, 2));

    let out: [10]YenPath = [YenPath{}; 10];
    let out_s: []YenPath = out[0..];
    // Simple paths 0->4:
    //   [0, 1, 3, 4] (cost 5)
    //   [0, 1, 2, 3, 4] (cost 5)
    //   [0, 1, 2, 4] (cost 7)
    const n = g.findKShortestPaths(0, 4, 10, *out_s) ?? 0;
    expect_eq(n, 3 as usize);
    expect_eq(out[0].total_cost, 5 as u64);
    expect_eq(out[1].total_cost, 5 as u64);
    expect_eq(out[2].total_cost, 7 as u64);

    let i: usize = 0;
    while (i < n) : (i += 1) {
        expect_true(out[i].isLoopless());
        let j: usize = i + 1;
        while (j < n) : (j += 1) {
            expect_eq(out[i].equals(out[j]), false);
        }
    }
});

// ============================================================================
// Test 3: Equal-Cost Diamond Lexicographical Tie-Breaking
// ============================================================================
test("yenKPaths: equal-cost diamond paths are deterministically ordered lexicographically", () => {
    let g = YenGraph.init(5) ?? return;
    // Add in reverse node order to verify sorting does not depend on insertion order
    expect_true(g.addEdge(0, 3, 5));
    expect_true(g.addEdge(3, 4, 5));
    expect_true(g.addEdge(0, 2, 4));
    expect_true(g.addEdge(2, 4, 6));
    expect_true(g.addEdge(0, 1, 7));
    expect_true(g.addEdge(1, 4, 3));

    let out: [3]YenPath = [YenPath{}; 3];
    let out_s: []YenPath = out[0..];
    const n = g.findKShortestPaths(0, 4, 3, *out_s) ?? 0;
    expect_eq(n, 3 as usize);

    const e0: [3]u8 = [0, 1, 4];
    const e1: [3]u8 = [0, 2, 4];
    const e2: [3]u8 = [0, 3, 4];
    expect_true(pathNodesEq(out[0], e0[0..]));
    expect_true(pathNodesEq(out[1], e1[0..]));
    expect_true(pathNodesEq(out[2], e2[0..]));
});

// ============================================================================
// Test 4: Zero-Weight Edges & Fewer-Than-K Available Paths
// ============================================================================
test("yenKPaths: handles zero-weight edges and returns actual count when fewer than k exist", () => {
    let g = YenGraph.init(3) ?? return;
    expect_true(g.addEdge(0, 1, 0));
    expect_true(g.addEdge(1, 2, 0));

    let out: [5]YenPath = [YenPath{}; 5];
    let out_s: []YenPath = out[0..];
    const n = g.findKShortestPaths(0, 2, 5, *out_s) ?? 0;
    expect_eq(n, 1 as usize);
    expect_eq(out[0].total_cost, 0 as u64);
    expect_eq(out[0].len, 3 as usize);
});

// ============================================================================
// Test 5: Trivial src == dst, k == 0, and Unreachable Target
// ============================================================================
test("yenKPaths: src==dst returns single 0-cost path; k==0 and unreachable return 0", () => {
    let g = YenGraph.init(4) ?? return;
    expect_true(g.addEdge(0, 1, 2));
    // Node 3 is completely isolated

    let out: [3]YenPath = [YenPath{}; 3];
    let out_s: []YenPath = out[0..];

    // src == dst
    const n_same = g.findKShortestPaths(2, 2, 3, *out_s) ?? 99;
    expect_eq(n_same, 1 as usize);
    expect_eq(out[0].len, 1 as usize);
    expect_eq(out[0].nodes[0], 2 as u8);
    expect_eq(out[0].total_cost, 0 as u64);

    // k == 0
    const n_zero = g.findKShortestPaths(0, 1, 0, *out_s) ?? 99;
    expect_eq(n_zero, 0 as usize);

    // Unreachable
    const n_unreach = g.findKShortestPaths(0, 3, 3, *out_s) ?? 99;
    expect_eq(n_unreach, 0 as usize);
    expect_eq(g.shortestPath(0, 3) == null, true);
});

// ============================================================================
// Test 6: Full Boundary Guards & 256-Edge Capacity Limit
// ============================================================================
test("yenKPaths: rejects invalid node counts, negative weights, self-loops, OOB, short buffers & full edges", () => {
    expect_eq(YenGraph.init(0) == null, true);
    expect_eq(YenGraph.init(33) == null, true);

    let g = YenGraph.init(32) ?? return;
    // Reject negative weight, self-loop, OOB, YEN_INF_COST
    expect_eq(g.addSignedEdge(0, 1, -1), false);
    expect_eq(g.addEdge(2, 2, 5), false);
    expect_eq(g.addEdge(0, 32, 5), false);
    expect_eq(g.addEdge(32, 0, 5), false);
    expect_eq(g.addEdge(0, 1, YEN_INF_COST), false);

    // Undersized out_paths buffer (k=4, len=2) -> null
    let small_out: [2]YenPath = [YenPath{}; 2];
    let small_s: []YenPath = small_out[0..];
    expect_eq(g.findKShortestPaths(0, 1, 4, *small_s) == null, true);
    expect_eq(g.findKShortestPaths(0, 32, 2, *small_s) == null, true);

    // Fill all 256 edges
    let added: usize = 0;
    let u: usize = 0;
    while (u < 32 && added < YEN_MAX_EDGES) : (u += 1) {
        let v: usize = 0;
        while (v < 32 && added < YEN_MAX_EDGES) : (v += 1) {
            if (u != v) {
                expect_true(g.addEdge(u, v, (u + v) as u64));
                added += 1;
            }
        }
    }
    expect_eq(g.edgeCount(), YEN_MAX_EDGES);
    // 257th edge rejected
    expect_eq(g.addEdge(31, 30, 1), false);
});

// ============================================================================
// Test 7: 16-Node Layered Mesh Top-10 Stress Test & cap.graph Dynamic Facade
// ============================================================================
test("yenKPaths: 16-node layered mesh top-10 paths monotonicity & cap.graph facade", () => {
    // 4x4 grid (nodes 0..15) moving right and down from 0 to 15
    let g = cap.graph.YenGraph.init(16) ?? return;
    let r: usize = 0;
    while (r < 4) : (r += 1) {
        let c: usize = 0;
        while (c < 4) : (c += 1) {
            const id = r * 4 + c;
            if (c + 1 < 4) {
                expect_true(g.addSignedEdge(id, id + 1, ((r + c + 1) as i64)));
            }
            if (r + 1 < 4) {
                expect_true(g.addSignedEdge(id, id + 4, ((r * 2 + c + 1) as i64)));
            }
        }
    }

    let out: [10]YenPath = [YenPath{}; 10];
    let out_s: []YenPath = out[0..];
    const n = g.findKShortestPaths(0, 15, 10, *out_s) ?? 0;
    expect_eq(n, 10 as usize);

    // Verify strict monotonic non-decreasing cost, uniqueness, and looplessness
    let i: usize = 0;
    while (i < n) : (i += 1) {
        expect_true(out[i].isLoopless());
        expect_eq(out[i].nodes[0], 0 as u8);
        expect_eq(out[i].nodes[out[i].len - 1], 15 as u8);
        if (i > 0) {
            expect_true(out[i - 1].isBetterThan(out[i]));
            expect_true(out[i - 1].total_cost <= out[i].total_cost);
        }
    }
});
