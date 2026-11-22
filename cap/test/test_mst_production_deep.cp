// Source: cp/cap/test/test_mst_production_deep.cp (Production deep tests for cap:mst)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MST_INF_WEIGHT,
    MstWeightedEdge as WeightedEdge,
    PrimEdge,
    DisjointSet,
    kruskalMst,
    maxSpanningTree,
    primDenseMst,
    isValidSpanningTree,
} from "cap:graph";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return (if (d < 0.0) -d else d) <= eps;
}

fn buildDenseMatrix(num_nodes: usize, edges: []const WeightedEdge, mat: *[]f64) {
    let i: usize = 0;
    while (i < num_nodes * num_nodes) {
        mat.*[i] = MST_INF_WEIGHT;
        i += 1;
    }
    i = 0;
    while (i < edges.len) {
        const e = edges[i];
        if (e.u != e.v) {
            const idx1 = e.u * num_nodes + e.v;
            const idx2 = e.v * num_nodes + e.u;
            if (e.weight < mat.*[idx1]) {
                mat.*[idx1] = e.weight;
                mat.*[idx2] = e.weight;
            }
        }
        i += 1;
    }
}

test("1. CLRS 9-node classic graph: Kruskal & Dense Prim exact weight == 37.0 + isValidSpanningTree", () => {
    // CLRS Chapter 23 Figure 23.4: a=0, b=1, c=2, d=3, e=4, f=5, g=6, h=7, i=8
    const edges = [
        WeightedEdge.init(0, 1, 4.0, 0),
        WeightedEdge.init(0, 7, 8.0, 1),
        WeightedEdge.init(1, 2, 8.0, 2),
        WeightedEdge.init(1, 7, 11.0, 3),
        WeightedEdge.init(2, 3, 7.0, 4),
        WeightedEdge.init(2, 5, 4.0, 5),
        WeightedEdge.init(2, 8, 2.0, 6),
        WeightedEdge.init(3, 4, 9.0, 7),
        WeightedEdge.init(3, 5, 14.0, 8),
        WeightedEdge.init(4, 5, 10.0, 9),
        WeightedEdge.init(5, 6, 2.0, 10),
        WeightedEdge.init(6, 7, 1.0, 11),
        WeightedEdge.init(6, 8, 6.0, 12),
        WeightedEdge.init(7, 8, 7.0, 13),
    ];

    let parent: [9]usize = std.mem.zeroes([9]usize);
    let rank: [9]u8 = std.mem.zeroes([9]u8);
    let order: [14]usize = std.mem.zeroes([14]usize);
    let out_idx: [8]usize = std.mem.zeroes([8]usize);
    let p_s: []usize = parent[0..];
    let r_s: []u8 = rank[0..];
    let o_s: []usize = order[0..];
    let out_s: []usize = out_idx[0..];

    const k_res = kruskalMst(9, edges[0..], &p_s, &r_s, &o_s, &out_s) ?? unreachable;
    expect_eq(k_res.isConnected, true);
    expect_eq(k_res.componentCount, 1 as usize);
    expect_eq(k_res.edgeCount, 8 as usize);
    expect_eq(approxEq(k_res.totalWeight, 37.0, 1e-9), true);

    const chk = isValidSpanningTree(9, edges[0..], out_idx[0..k_res.edgeCount], &p_s, &r_s);
    expect_eq(chk.valid, true);
    expect_eq(approxEq(chk.totalWeight, 37.0, 1e-9), true);

    // Dense Prim verification on the exact same CLRS graph
    let mat: [81]f64 = std.mem.zeroes([81]f64);
    let mat_s: []f64 = mat[0..];
    buildDenseMatrix(9, edges[0..], &mat_s);

    let min_w: [9]f64 = std.mem.zeroes([9]f64);
    let p_par: [9]i32 = std.mem.zeroes([9]i32);
    let in_mst: [9]u8 = std.mem.zeroes([9]u8);
    let prim_out: [8]PrimEdge = std.mem.zeroes([8]PrimEdge);
    let mw_s: []f64 = min_w[0..];
    let pp_s: []i32 = p_par[0..];
    let im_s: []u8 = in_mst[0..];
    let po_s: []PrimEdge = prim_out[0..];

    const p_res = primDenseMst(9, mat[0..], &mw_s, &pp_s, &im_s, &po_s) ?? unreachable;
    expect_eq(p_res.isConnected, true);
    expect_eq(p_res.componentCount, 1 as usize);
    expect_eq(p_res.edgeCount, 8 as usize);
    expect_eq(approxEq(p_res.totalWeight, 37.0, 1e-9), true);
});

test("2. Disconnected graph Minimum Spanning Forest: exact componentCount == 3 (Kruskal & Prim)", () => {
    // Component A: {0, 1, 2} -> MST weight = 1 + 2 = 3
    // Component B: {3, 4}    -> MST weight = 5
    // Component C: {5}       -> isolated vertex
    const edges = [
        WeightedEdge.init(0, 1, 1.0, 1),
        WeightedEdge.init(1, 2, 2.0, 2),
        WeightedEdge.init(0, 2, 10.0, 3),
        WeightedEdge.init(3, 4, 5.0, 4),
    ];

    let parent: [6]usize = std.mem.zeroes([6]usize);
    let rank: [6]u8 = std.mem.zeroes([6]u8);
    let order: [4]usize = std.mem.zeroes([4]usize);
    let out_idx: [5]usize = std.mem.zeroes([5]usize);
    let p_s: []usize = parent[0..];
    let r_s: []u8 = rank[0..];
    let o_s: []usize = order[0..];
    let out_s: []usize = out_idx[0..];

    const k_res = kruskalMst(6, edges[0..], &p_s, &r_s, &o_s, &out_s) ?? unreachable;
    expect_eq(k_res.isConnected, false);
    expect_eq(k_res.componentCount, 3 as usize);
    expect_eq(k_res.edgeCount, 3 as usize);
    expect_eq(approxEq(k_res.totalWeight, 8.0, 1e-9), true);

    // Forest is NOT a connected spanning tree
    const chk = isValidSpanningTree(6, edges[0..], out_idx[0..k_res.edgeCount], &p_s, &r_s);
    expect_eq(chk.valid, false);

    // Dense Prim produces identical forest summary
    let mat: [36]f64 = std.mem.zeroes([36]f64);
    let mat_s: []f64 = mat[0..];
    buildDenseMatrix(6, edges[0..], &mat_s);

    let min_w: [6]f64 = std.mem.zeroes([6]f64);
    let p_par: [6]i32 = std.mem.zeroes([6]i32);
    let in_mst: [6]u8 = std.mem.zeroes([6]u8);
    let prim_out: [5]PrimEdge = std.mem.zeroes([5]PrimEdge);
    let mw_s: []f64 = min_w[0..];
    let pp_s: []i32 = p_par[0..];
    let im_s: []u8 = in_mst[0..];
    let po_s: []PrimEdge = prim_out[0..];

    const p_res = primDenseMst(6, mat[0..], &mw_s, &pp_s, &im_s, &po_s) ?? unreachable;
    expect_eq(p_res.isConnected, false);
    expect_eq(p_res.componentCount, 3 as usize);
    expect_eq(p_res.edgeCount, 3 as usize);
    expect_eq(approxEq(p_res.totalWeight, 8.0, 1e-9), true);
});

test("3. Negative weights, zero weights, parallel edges & self-loops", () => {
    // 4 vertices with self-loops, parallel edges between (0,1) and negative/zero weights
    const edges = [
        WeightedEdge.init(0, 0, -100.0, 0), // self-loop must be ignored
        WeightedEdge.init(0, 1, 5.0, 1),
        WeightedEdge.init(0, 1, -3.5, 2),   // lighter parallel edge chosen
        WeightedEdge.init(1, 2, 0.0, 3),    // zero-weight edge
        WeightedEdge.init(2, 3, -2.5, 4),   // negative-weight edge
        WeightedEdge.init(0, 3, 4.0, 5),
        WeightedEdge.init(3, 3, 0.0, 6),    // self-loop must be ignored
    ];

    let parent: [4]usize = std.mem.zeroes([4]usize);
    let rank: [4]u8 = std.mem.zeroes([4]u8);
    let order: [7]usize = std.mem.zeroes([7]usize);
    let out_idx: [3]usize = std.mem.zeroes([3]usize);
    let p_s: []usize = parent[0..];
    let r_s: []u8 = rank[0..];
    let o_s: []usize = order[0..];
    let out_s: []usize = out_idx[0..];

    const k_res = kruskalMst(4, edges[0..], &p_s, &r_s, &o_s, &out_s) ?? unreachable;
    // Optimal MST: (-3.5) + 0.0 + (-2.5) = -6.0
    expect_eq(k_res.isConnected, true);
    expect_eq(k_res.edgeCount, 3 as usize);
    expect_eq(approxEq(k_res.totalWeight, -6.0, 1e-9), true);

    const chk = isValidSpanningTree(4, edges[0..], out_idx[0..k_res.edgeCount], &p_s, &r_s);
    expect_eq(chk.valid, true);
    expect_eq(approxEq(chk.totalWeight, -6.0, 1e-9), true);

    // Dense Prim matches exact -6.0 total weight
    let mat: [16]f64 = std.mem.zeroes([16]f64);
    let mat_s: []f64 = mat[0..];
    buildDenseMatrix(4, edges[0..], &mat_s);

    let min_w: [4]f64 = std.mem.zeroes([4]f64);
    let p_par: [4]i32 = std.mem.zeroes([4]i32);
    let in_mst: [4]u8 = std.mem.zeroes([4]u8);
    let prim_out: [3]PrimEdge = std.mem.zeroes([3]PrimEdge);
    let mw_s: []f64 = min_w[0..];
    let pp_s: []i32 = p_par[0..];
    let im_s: []u8 = in_mst[0..];
    let po_s: []PrimEdge = prim_out[0..];

    const p_res = primDenseMst(4, mat[0..], &mw_s, &pp_s, &im_s, &po_s) ?? unreachable;
    expect_eq(p_res.isConnected, true);
    expect_eq(approxEq(p_res.totalWeight, -6.0, 1e-9), true);
});

test("4. Maximum Spanning Tree verification & cycle rejection in isValidSpanningTree", () => {
    const edges = [
        WeightedEdge.init(0, 1, 3.0, 0),
        WeightedEdge.init(1, 2, 10.0, 1),
        WeightedEdge.init(2, 3, 8.0, 2),
        WeightedEdge.init(3, 0, 7.0, 3),
        WeightedEdge.init(0, 2, 9.0, 4),
        WeightedEdge.init(1, 3, 2.0, 5),
    ];

    let parent: [4]usize = std.mem.zeroes([4]usize);
    let rank: [4]u8 = std.mem.zeroes([4]u8);
    let order: [6]usize = std.mem.zeroes([6]usize);
    let out_idx: [3]usize = std.mem.zeroes([3]usize);
    let p_s: []usize = parent[0..];
    let r_s: []u8 = rank[0..];
    let o_s: []usize = order[0..];
    let out_s: []usize = out_idx[0..];

    const max_res = maxSpanningTree(4, edges[0..], &p_s, &r_s, &o_s, &out_s) ?? unreachable;
    // Optimal MaxST: (1,2)=10 + (0,2)=9 + (2,3)=8 => 27.0
    expect_eq(max_res.isConnected, true);
    expect_eq(max_res.edgeCount, 3 as usize);
    expect_eq(approxEq(max_res.totalWeight, 27.0, 1e-9), true);

    const chk = isValidSpanningTree(4, edges[0..], out_idx[0..max_res.edgeCount], &p_s, &r_s);
    expect_eq(chk.valid, true);
    expect_eq(approxEq(chk.totalWeight, 27.0, 1e-9), true);

    // Manually construct a cyclic 3-edge subset on {0, 1, 2} (missing vertex 3) -> must be invalid
    const cyclic_indices = [0 as usize, 1 as usize, 4 as usize];
    const bad_chk = isValidSpanningTree(4, edges[0..], cyclic_indices[0..], &p_s, &r_s);
    expect_eq(bad_chk.valid, false);
});

test("5. Medium-scale deterministic 24-node graph: Kruskal vs Dense Prim exact cross-check", () => {
    const n: usize = 24;
    let edges: [120]WeightedEdge = std.mem.zeroes([120]WeightedEdge);
    let rng: u32 = 1234567;
    let e_cnt: usize = 0;

    // First build a spanning backbone chain to guarantee connectivity
    let i: usize = 0;
    while (i + 1 < n) {
        rng = rng *% 1664525 +% 1013904223;
        const w = ((rng >> 16) % 200) as f64 + 1.0;
        edges[e_cnt] = WeightedEdge.init(i, i + 1, w, e_cnt as u32);
        e_cnt += 1;
        i += 1;
    }

    // Add 97 pseudo-random chord edges
    while (e_cnt < 120) {
        rng = rng *% 1664525 +% 1013904223;
        const u = ((rng >> 16) as usize) % n;
        rng = rng *% 1664525 +% 1013904223;
        const v = ((rng >> 16) as usize) % n;
        rng = rng *% 1664525 +% 1013904223;
        const w = (((rng >> 16) % 250) as f64) - 50.0;
        edges[e_cnt] = WeightedEdge.init(u, v, w, e_cnt as u32);
        e_cnt += 1;
    }

    let parent: [24]usize = std.mem.zeroes([24]usize);
    let rank: [24]u8 = std.mem.zeroes([24]u8);
    let order: [120]usize = std.mem.zeroes([120]usize);
    let out_idx: [23]usize = std.mem.zeroes([23]usize);
    let p_s: []usize = parent[0..];
    let r_s: []u8 = rank[0..];
    let o_s: []usize = order[0..];
    let out_s: []usize = out_idx[0..];

    const k_res = kruskalMst(n, edges[0..e_cnt], &p_s, &r_s, &o_s, &out_s) ?? unreachable;
    expect_eq(k_res.isConnected, true);
    expect_eq(k_res.edgeCount, 23 as usize);

    const chk = isValidSpanningTree(n, edges[0..e_cnt], out_idx[0..k_res.edgeCount], &p_s, &r_s);
    expect_eq(chk.valid, true);
    expect_eq(approxEq(chk.totalWeight, k_res.totalWeight, 1e-9), true);

    let mat: [576]f64 = std.mem.zeroes([576]f64);
    let mat_s: []f64 = mat[0..];
    buildDenseMatrix(n, edges[0..e_cnt], &mat_s);

    let min_w: [24]f64 = std.mem.zeroes([24]f64);
    let p_par: [24]i32 = std.mem.zeroes([24]i32);
    let in_mst: [24]u8 = std.mem.zeroes([24]u8);
    let prim_out: [23]PrimEdge = std.mem.zeroes([23]PrimEdge);
    let mw_s: []f64 = min_w[0..];
    let pp_s: []i32 = p_par[0..];
    let im_s: []u8 = in_mst[0..];
    let po_s: []PrimEdge = prim_out[0..];

    const p_res = primDenseMst(n, mat[0..], &mw_s, &pp_s, &im_s, &po_s) ?? unreachable;
    expect_eq(p_res.isConnected, true);
    expect_eq(p_res.edgeCount, 23 as usize);
    expect_eq(approxEq(p_res.totalWeight, k_res.totalWeight, 1e-9), true);
});

test("6. Defensive edge cases: empty graph, single node, out-of-range vertices, undersized buffers & DSU", () => {
    let parent: [4]usize = std.mem.zeroes([4]usize);
    let rank: [4]u8 = std.mem.zeroes([4]u8);
    let order: [4]usize = std.mem.zeroes([4]usize);
    let out_idx: [4]usize = std.mem.zeroes([4]usize);
    let p_s: []usize = parent[0..];
    let r_s: []u8 = rank[0..];
    let o_s: []usize = order[0..];
    let out_s: []usize = out_idx[0..];
    const empty_edges: [0]WeightedEdge = [];

    // Empty graph (0 nodes)
    const empty_res = kruskalMst(0, empty_edges[0..], &p_s, &r_s, &o_s, &out_s) ?? unreachable;
    expect_eq(empty_res.edgeCount, 0 as usize);
    expect_eq(empty_res.componentCount, 0 as usize);
    expect_eq(empty_res.isConnected, false);

    // Single node (1 node, 0 edges): connected trivial tree
    const single_res = kruskalMst(1, empty_edges[0..], &p_s, &r_s, &o_s, &out_s) ?? unreachable;
    expect_eq(single_res.edgeCount, 0 as usize);
    expect_eq(single_res.componentCount, 1 as usize);
    expect_eq(single_res.isConnected, true);

    const single_chk = isValidSpanningTree(1, empty_edges[0..], out_idx[0..0], &p_s, &r_s);
    expect_eq(single_chk.valid, true);

    // Out-of-range vertex in edge list => null
    const bad_edges = [WeightedEdge.init(0, 5, 1.0, 0)];
    expect_eq(kruskalMst(3, bad_edges[0..], &p_s, &r_s, &o_s, &out_s) == null, true);

    // Undersized scratch/output buffers => null
    let short_p: []usize = parent[0..2];
    let short_out: []usize = out_idx[0..1];
    const ok_edges = [WeightedEdge.init(0, 1, 1.0, 0), WeightedEdge.init(1, 2, 2.0, 1)];
    expect_eq(kruskalMst(3, ok_edges[0..], &short_p, &r_s, &o_s, &out_s) == null, true);
    expect_eq(kruskalMst(3, ok_edges[0..], &p_s, &r_s, &o_s, &short_out) == null, true);

    // DisjointSet direct operations & bounds check
    let dsu = DisjointSet.init(4, parent[0..], rank[0..]) ?? unreachable;
    expect_eq(dsu.components(), 4 as usize);
    expect_eq(dsu.unite(0, 1), true);
    expect_eq(dsu.unite(1, 2), true);
    expect_eq(dsu.unite(0, 2), false);
    expect_eq(dsu.connected(0, 2), true);
    expect_eq(dsu.connected(0, 3), false);
    expect_eq(dsu.components(), 2 as usize);
    expect_eq(dsu.find(4) == null, true);
    expect_eq(dsu.unite(0, 4), false);
});
