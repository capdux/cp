// Source: cp/cap/test/test_bipartite_production_deep.cp (Production deep tests for cap:bipartite)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    UNMATCHED,
    BipartiteGraph,
    isBipartite,
    hopcroftKarp,
    konigMinVertexCover,
} from "cap:bipartite";

fn verifyMatchingAndCover(
    g: BipartiteGraph,
    match_size: usize,
    ml: []const i32,
    mr: []const i32,
    cl: []const u8,
    cr: []const u8,
) {
    // 1. Verify bidirectional matching consistency and exact cardinality
    let count_l: usize = 0;
    let u: usize = 0;
    while (u < g.num_left) {
        if (ml[u] != UNMATCHED) {
            const v = ml[u] as usize;
            expect_eq(v < g.num_right, true);
            expect_eq(mr[v], u as i32);
            // Verify (u, v) is an actual edge in g
            let found = false;
            let ei = g.row_ptr[u];
            while (ei < g.row_ptr[u + 1]) {
                if (g.col_idx[ei] == v) {
                    found = true;
                    break;
                }
                ei += 1;
            }
            expect_eq(found, true);
            count_l += 1;
        }
        u += 1;
    }
    expect_eq(count_l, match_size);

    // 2. Verify every edge (u, v) is covered by cover_left[u] == 1 || cover_right[v] == 1
    //    (Equivalently, the complement MIS has zero internal edges)
    u = 0;
    while (u < g.num_left) {
        let ei = g.row_ptr[u];
        while (ei < g.row_ptr[u + 1]) {
            const v = g.col_idx[ei];
            expect_eq(cl[u] == 1 || cr[v] == 1, true);
            ei += 1;
        }
        u += 1;
    }
}

test("1. isBipartite: even cycle + disconnected tree pass with valid 2-coloring; odd cycle & self-loop rejected", () => {
    // 8 nodes: 6-cycle {0,1,2,3,4,5} + disjoint edge {6,7}
    const row_ptr: [9]usize = [0, 2, 4, 6, 8, 10, 12, 13, 14];
    const col_idx: [14]usize = [
        1, 5, // 0
        0, 2, // 1
        1, 3, // 2
        2, 4, // 3
        3, 5, // 4
        4, 0, // 5
        7,    // 6
        6,    // 7
    ];

    let colors: [8]i8 = std.mem.zeroes([8]i8);
    let queue: [8]usize = std.mem.zeroes([8]usize);
    let col_s: []i8 = colors[0..];
    let q_s: []usize = queue[0..];

    expect_eq(isBipartite(8, row_ptr[0..], col_idx[0..], &col_s, &q_s), true);

    // Every edge must connect opposite colors
    let u: usize = 0;
    while (u < 8) {
        expect_eq(colors[u] == 0 || colors[u] == 1, true);
        let ei = row_ptr[u];
        while (ei < row_ptr[u + 1]) {
            expect_eq(colors[u] != colors[col_idx[ei]], true);
            ei += 1;
        }
        u += 1;
    }

    // Odd 5-cycle {0,1,2,3,4} -> NOT bipartite
    const odd_rp: [6]usize = [0, 2, 4, 6, 8, 10];
    const odd_ci: [10]usize = [1, 4, 0, 2, 1, 3, 2, 4, 3, 0];
    expect_eq(isBipartite(5, odd_rp[0..], odd_ci[0..], &col_s, &q_s), false);

    // Self-loop on vertex 1 -> NOT bipartite
    const loop_rp: [3]usize = [0, 1, 3];
    const loop_ci: [3]usize = [1, 0, 1];
    expect_eq(isBipartite(2, loop_rp[0..], loop_ci[0..], &col_s, &q_s), false);
});

test("2. Hopcroft-Karp: 4x4 perfect matching (== 4) & constrained imperfect matching (== 3)", () => {
    // Perfect matching: L0->{0,1}, L1->{1,2}, L2->{2,3}, L3->{3,0}
    const rp1: [5]usize = [0, 2, 4, 6, 8];
    const ci1: [8]usize = [0, 1, 1, 2, 2, 3, 3, 0];
    const g1 = BipartiteGraph.init(4, 4, rp1[0..], ci1[0..]) ?? unreachable;

    let ml: [4]i32 = std.mem.zeroes([4]i32);
    let mr: [4]i32 = std.mem.zeroes([4]i32);
    let dist: [4]u32 = std.mem.zeroes([4]u32);
    let q: [4]usize = std.mem.zeroes([4]usize);
    let ml_s: []i32 = ml[0..];
    let mr_s: []i32 = mr[0..];
    let d_s: []u32 = dist[0..];
    let q_s: []usize = q[0..];

    const m1 = hopcroftKarp(g1, &ml_s, &mr_s, &d_s, &q_s) ?? unreachable;
    expect_eq(m1, 4 as usize);

    let cl: [4]u8 = std.mem.zeroes([4]u8);
    let cr: [4]u8 = std.mem.zeroes([4]u8);
    let vl: [4]u8 = std.mem.zeroes([4]u8);
    let vr: [4]u8 = std.mem.zeroes([4]u8);
    let cl_s: []u8 = cl[0..];
    let cr_s: []u8 = cr[0..];
    let vl_s: []u8 = vl[0..];
    let vr_s: []u8 = vr[0..];

    const mvc1 = konigMinVertexCover(g1, ml[0..], mr[0..], &cl_s, &cr_s, &vl_s, &vr_s, &q_s) ?? unreachable;
    expect_eq(mvc1, 4 as usize);
    verifyMatchingAndCover(g1, m1, ml[0..], mr[0..], cl[0..], cr[0..]);

    // Imperfect 4x4: L0->{0,1}, L1->{1}, L2->{2}, L3->{2} => max matching == 3
    const rp2: [5]usize = [0, 2, 3, 4, 5];
    const ci2: [5]usize = [0, 1, 1, 2, 2];
    const g2 = BipartiteGraph.init(4, 4, rp2[0..], ci2[0..]) ?? unreachable;

    const m2 = hopcroftKarp(g2, &ml_s, &mr_s, &d_s, &q_s) ?? unreachable;
    expect_eq(m2, 3 as usize);
    const mvc2 = konigMinVertexCover(g2, ml[0..], mr[0..], &cl_s, &cr_s, &vl_s, &vr_s, &q_s) ?? unreachable;
    expect_eq(mvc2, 3 as usize);
    verifyMatchingAndCover(g2, m2, ml[0..], mr[0..], cl[0..], cr[0..]);
});

test("3. Asymmetric 5x4 Konig minimum vertex cover & maximum independent set verification", () => {
    // L0->{0,1}, L1->{1,2}, L2->{1}, L3->{2,3}, L4->{1}
    // Right vertex 1 has degree 4; max matching = 4 (L0->0, L2->1, L1->2, L3->3; L4 unmatched)
    const rp: [6]usize = [0, 2, 4, 5, 7, 8];
    const ci: [8]usize = [0, 1, 1, 2, 1, 2, 3, 1];
    const g = BipartiteGraph.init(5, 4, rp[0..], ci[0..]) ?? unreachable;

    let ml: [5]i32 = std.mem.zeroes([5]i32);
    let mr: [4]i32 = std.mem.zeroes([4]i32);
    let dist: [5]u32 = std.mem.zeroes([5]u32);
    let q: [5]usize = std.mem.zeroes([5]usize);
    let ml_s: []i32 = ml[0..];
    let mr_s: []i32 = mr[0..];
    let d_s: []u32 = dist[0..];
    let q_s: []usize = q[0..];

    const m = hopcroftKarp(g, &ml_s, &mr_s, &d_s, &q_s) ?? unreachable;
    expect_eq(m, 4 as usize);

    let cl: [5]u8 = std.mem.zeroes([5]u8);
    let cr: [4]u8 = std.mem.zeroes([4]u8);
    let vl: [5]u8 = std.mem.zeroes([5]u8);
    let vr: [4]u8 = std.mem.zeroes([4]u8);
    let cl_s: []u8 = cl[0..];
    let cr_s: []u8 = cr[0..];
    let vl_s: []u8 = vl[0..];
    let vr_s: []u8 = vr[0..];

    const mvc = konigMinVertexCover(g, ml[0..], mr[0..], &cl_s, &cr_s, &vl_s, &vr_s, &q_s) ?? unreachable;
    expect_eq(mvc, m);
    verifyMatchingAndCover(g, m, ml[0..], mr[0..], cl[0..], cr[0..]);

    // Maximum Independent Set size = |L| + |R| - |MVC| = 9 - 4 = 5
    let mis_size: usize = 0;
    let i: usize = 0;
    while (i < 5) {
        if (cl[i] == 0) {
            mis_size += 1;
        }
        i += 1;
    }
    i = 0;
    while (i < 4) {
        if (cr[i] == 0) {
            mis_size += 1;
        }
        i += 1;
    }
    expect_eq(mis_size, 5 as usize);
});

test("4. 50x50 deterministic sparse bipartite graph: Hopcroft-Karp + Konig full verification", () => {
    const n: usize = 50;
    let row_ptr: [51]usize = std.mem.zeroes([51]usize);
    let col_idx: [300]usize = std.mem.zeroes([300]usize);

    let rng: u32 = 20261121;
    let edge_pos: usize = 0;
    let u: usize = 0;
    while (u < n) {
        row_ptr[u] = edge_pos;
        // Generate 4 deterministic neighbors per left vertex (skipping duplicates)
        let added: [4]usize = [n, n, n, n];
        let deg: usize = 0;
        let attempt: usize = 0;
        while (attempt < 4) {
            rng = rng *% 1664525 +% 1013904223;
            const v = ((rng >> 16) as usize) % n;
            let dup = false;
            let k: usize = 0;
            while (k < deg) {
                if (added[k] == v) {
                    dup = true;
                    break;
                }
                k += 1;
            }
            if (!dup) {
                added[deg] = v;
                deg += 1;
                col_idx[edge_pos] = v;
                edge_pos += 1;
            }
            attempt += 1;
        }
        u += 1;
    }
    row_ptr[n] = edge_pos;

    const g = BipartiteGraph.init(n, n, row_ptr[0..], col_idx[0..edge_pos]) ?? unreachable;

    let ml: [50]i32 = std.mem.zeroes([50]i32);
    let mr: [50]i32 = std.mem.zeroes([50]i32);
    let dist: [50]u32 = std.mem.zeroes([50]u32);
    let q: [50]usize = std.mem.zeroes([50]usize);
    let ml_s: []i32 = ml[0..];
    let mr_s: []i32 = mr[0..];
    let d_s: []u32 = dist[0..];
    let q_s: []usize = q[0..];

    const m = hopcroftKarp(g, &ml_s, &mr_s, &d_s, &q_s) ?? unreachable;
    expect_eq(m > 30 && m <= 50, true);

    let cl: [50]u8 = std.mem.zeroes([50]u8);
    let cr: [50]u8 = std.mem.zeroes([50]u8);
    let vl: [50]u8 = std.mem.zeroes([50]u8);
    let vr: [50]u8 = std.mem.zeroes([50]u8);
    let cl_s: []u8 = cl[0..];
    let cr_s: []u8 = cr[0..];
    let vl_s: []u8 = vl[0..];
    let vr_s: []u8 = vr[0..];

    const mvc = konigMinVertexCover(g, ml[0..], mr[0..], &cl_s, &cr_s, &vl_s, &vr_s, &q_s) ?? unreachable;
    expect_eq(mvc, m);
    verifyMatchingAndCover(g, m, ml[0..], mr[0..], cl[0..], cr[0..]);
});

test("5. Edge cases: empty graph, one-sided empty graph & edgeless graph", () => {
    const rp0: [1]usize = [0];
    const empty_ci: [0]usize = [];
    const g0 = BipartiteGraph.init(0, 0, rp0[0..], empty_ci[0..]) ?? unreachable;

    let ml: [3]i32 = std.mem.zeroes([3]i32);
    let mr: [3]i32 = std.mem.zeroes([3]i32);
    let dist: [3]u32 = std.mem.zeroes([3]u32);
    let q: [3]usize = std.mem.zeroes([3]usize);
    let ml_s: []i32 = ml[0..];
    let mr_s: []i32 = mr[0..];
    let d_s: []u32 = dist[0..];
    let q_s: []usize = q[0..];

    expect_eq(hopcroftKarp(g0, &ml_s, &mr_s, &d_s, &q_s) ?? unreachable, 0 as usize);

    // 3x3 edgeless graph => matching == 0, MVC == 0
    const rp_none: [4]usize = [0, 0, 0, 0];
    const g_none = BipartiteGraph.init(3, 3, rp_none[0..], empty_ci[0..]) ?? unreachable;
    expect_eq(hopcroftKarp(g_none, &ml_s, &mr_s, &d_s, &q_s) ?? unreachable, 0 as usize);

    // Empty undirected graph is trivially bipartite
    let colors: [1]i8 = std.mem.zeroes([1]i8);
    let col_s: []i8 = colors[0..];
    expect_eq(isBipartite(0, rp0[0..], empty_ci[0..], &col_s, &q_s), true);
});

test("6. Defensive validation: malformed CSR & undersized scratch/output buffers rejected", () => {
    const empty_ci: [0]usize = [];
    // row_ptr[0] != 0 rejected
    const bad_start: [3]usize = [1, 1, 1];
    expect_eq(BipartiteGraph.init(2, 2, bad_start[0..], empty_ci[0..]) == null, true);

    // Non-monotonic row_ptr rejected
    const bad_mono: [3]usize = [0, 2, 1];
    const ci2: [2]usize = [0, 1];
    expect_eq(BipartiteGraph.init(2, 2, bad_mono[0..], ci2[0..]) == null, true);

    // Out-of-range right vertex rejected
    const ok_rp: [3]usize = [0, 1, 2];
    const bad_col: [2]usize = [0, 5];
    expect_eq(BipartiteGraph.init(2, 2, ok_rp[0..], bad_col[0..]) == null, true);

    // Valid 2x2 graph with undersized output buffers rejected
    const ok_col: [2]usize = [0, 1];
    const g = BipartiteGraph.init(2, 2, ok_rp[0..], ok_col[0..]) ?? unreachable;
    let ml: [2]i32 = std.mem.zeroes([2]i32);
    let mr: [2]i32 = std.mem.zeroes([2]i32);
    let dist: [2]u32 = std.mem.zeroes([2]u32);
    let q: [2]usize = std.mem.zeroes([2]usize);
    let short_ml: []i32 = ml[0..1];
    let mr_s: []i32 = mr[0..];
    let d_s: []u32 = dist[0..];
    let q_s: []usize = q[0..];

    expect_eq(hopcroftKarp(g, &short_ml, &mr_s, &d_s, &q_s) == null, true);

    // isBipartite rejects undersized color buffer
    let colors: [1]i8 = std.mem.zeroes([1]i8);
    let col_s: []i8 = colors[0..];
    expect_eq(isBipartite(2, ok_rp[0..], ok_col[0..], &col_s, &q_s), false);
});
