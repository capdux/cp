// Source: cp/cap/test/test_pagerank_production_deep.cp (Production deep tests for cap:pagerank)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CsrGraph,
    PageRankResult,
    HitsResult,
    pagerank,
    personalizedPagerank,
    hits,
    topNode,
    sumScores,
} from "cap:pagerank";

fn absDiff(a: f64, b: f64): f64 {
    const d = a - b;
    return if (d < 0.0) 0.0 - d else d;
}

fn l2Norm(xs: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        s = s + xs[i] * xs[i];
        i += 1;
    }
    return std.math.sqrt(s);
}

test("1. Star sink graph with dangling hub: exact analytical PageRank & probability conservation", () => {
    // 5 nodes: 0->4, 1->4, 2->4, 3->4; node 4 is a dangling sink (out-degree 0)
    const row_ptr: [6]usize = [0, 1, 2, 3, 4, 4];
    const col_idx: [4]usize = [4, 4, 4, 4];
    const g = CsrGraph {
        num_nodes: 5,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
    };

    expect_eq(g.validate(), true);
    expect_eq(g.numEdges(), 4 as usize);
    expect_eq(g.outDegree(0), 1 as usize);
    expect_eq(g.outDegree(4), 0 as usize);
    expect_eq(g.neighbors(2).len, 1 as usize);
    expect_eq(g.neighbors(2)[0], 4 as usize);

    let ranks: [5]f64 = std.mem.zeroes([5]f64);
    let scratch: [5]f64 = std.mem.zeroes([5]f64);
    const d: f64 = 0.85;

    const res = pagerank(g, d, 1.0e-14, 500, ranks[0..], scratch[0..]) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.l1_diff <= 1.0e-14, true);

    // Probability sum strictly == 1.0
    const total = sumScores(ranks[0..]);
    expect_eq(absDiff(total, 1.0) < 1.0e-12, true);

    // Closed-form stationary distribution:
    // r[0..3] = 1 / (5 + 4d), r[4] = (1 + 4d) / (5 + 4d)
    const expected_leaf = 1.0 / (5.0 + 4.0 * d);
    const expected_sink = (1.0 + 4.0 * d) / (5.0 + 4.0 * d);
    expect_eq(absDiff(ranks[0], expected_leaf) < 1.0e-12, true);
    expect_eq(absDiff(ranks[1], expected_leaf) < 1.0e-12, true);
    expect_eq(absDiff(ranks[2], expected_leaf) < 1.0e-12, true);
    expect_eq(absDiff(ranks[3], expected_leaf) < 1.0e-12, true);
    expect_eq(absDiff(ranks[4], expected_sink) < 1.0e-12, true);

    const best = topNode(ranks[0..]) ?? unreachable;
    expect_eq(best, 4 as usize);
});

test("2. Classic 4-node directed cycle + dangling node: stationary fixed-point equations", () => {
    // Edges: 0->1, 0->2, 1->2, 2->0, 2->3; node 3 is dangling
    const row_ptr: [5]usize = [0, 2, 3, 5, 5];
    const col_idx: [5]usize = [1, 2, 2, 0, 3];
    const g = CsrGraph {
        num_nodes: 4,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
    };
    expect_eq(g.validate(), true);

    let ranks: [4]f64 = std.mem.zeroes([4]f64);
    let scratch: [4]f64 = std.mem.zeroes([4]f64);
    const d: f64 = 0.85;

    const res = pagerank(g, d, 1.0e-13, 500, ranks[0..], scratch[0..]) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(absDiff(sumScores(ranks[0..]), 1.0) < 1.0e-12, true);

    // Verify exact PageRank fixed-point equations with dangling mass r[3]
    const base = ((1.0 - d) + d * ranks[3]) / 4.0;
    const eq0 = d * (ranks[2] / 2.0) + base;
    const eq1 = d * (ranks[0] / 2.0) + base;
    const eq2 = d * (ranks[0] / 2.0 + ranks[1]) + base;
    const eq3 = d * (ranks[2] / 2.0) + base;

    expect_eq(absDiff(ranks[0], eq0) < 1.0e-11, true);
    expect_eq(absDiff(ranks[1], eq1) < 1.0e-11, true);
    expect_eq(absDiff(ranks[2], eq2) < 1.0e-11, true);
    expect_eq(absDiff(ranks[3], eq3) < 1.0e-11, true);
    // Symmetry: node 0 and node 3 receive identical incoming rank (d * r[2]/2 + base)
    expect_eq(absDiff(ranks[0], ranks[3]) < 1.0e-12, true);

    const best = topNode(ranks[0..]) ?? unreachable;
    expect_eq(best, 2 as usize);
});

test("3. Symmetric directed cycle: uniform 1/n distribution & equivalence with uniform PPR", () => {
    // 5-node cycle: 0->1->2->3->4->0
    const row_ptr: [6]usize = [0, 1, 2, 3, 4, 5];
    const col_idx: [5]usize = [1, 2, 3, 4, 0];
    const g = CsrGraph {
        num_nodes: 5,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
    };

    let ranks_pr: [5]f64 = std.mem.zeroes([5]f64);
    let ranks_ppr: [5]f64 = std.mem.zeroes([5]f64);
    let scratch: [5]f64 = std.mem.zeroes([5]f64);

    const r1 = pagerank(g, 0.85, 1.0e-14, 100, ranks_pr[0..], scratch[0..]) ?? unreachable;
    expect_eq(r1.converged, true);
    expect_eq(r1.iterations, 1 as u32);
    expect_eq(absDiff(sumScores(ranks_pr[0..]), 1.0) < 1.0e-12, true);

    // Unnormalized uniform teleport vector [3, 3, 3, 3, 3] must produce identical result
    const uniform_tp: [5]f64 = [3.0, 3.0, 3.0, 3.0, 3.0];
    const r2 = personalizedPagerank(g, uniform_tp[0..], 0.85, 1.0e-14, 100, ranks_ppr[0..], scratch[0..]) ?? unreachable;
    expect_eq(r2.converged, true);
    expect_eq(absDiff(sumScores(ranks_ppr[0..]), 1.0) < 1.0e-12, true);

    let i: usize = 0;
    while (i < 5) {
        expect_eq(absDiff(ranks_pr[i], 0.2) < 1.0e-13, true);
        expect_eq(absDiff(ranks_ppr[i], 0.2) < 1.0e-13, true);
        i += 1;
    }
});

test("4. Personalized PageRank on two-community graph biases seed & local neighborhood", () => {
    // Community A: {0, 1, 2} tightly linked; Community B: {3, 4, 5} tightly linked;
    // Weak bridges: 2->3 and 5->0
    // 0 -> [1, 2]
    // 1 -> [0, 2]
    // 2 -> [0, 1, 3]
    // 3 -> [4, 5]
    // 4 -> [3, 5]
    // 5 -> [3, 4, 0]
    const row_ptr: [7]usize = [0, 2, 4, 7, 9, 11, 14];
    const col_idx: [14]usize = [1, 2, 0, 2, 0, 1, 3, 4, 5, 3, 5, 3, 4, 0];
    const g = CsrGraph {
        num_nodes: 6,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
    };
    expect_eq(g.validate(), true);

    let ranks: [6]f64 = std.mem.zeroes([6]f64);
    let scratch: [6]f64 = std.mem.zeroes([6]f64);

    // Seed focused on node 0 (unnormalized weight 10.0)
    const tp_a: [6]f64 = [10.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    const ra = personalizedPagerank(g, tp_a[0..], 0.75, 1.0e-13, 500, ranks[0..], scratch[0..]) ?? unreachable;
    expect_eq(ra.converged, true);
    expect_eq(absDiff(sumScores(ranks[0..]), 1.0) < 1.0e-12, true);
    expect_eq(topNode(ranks[0..]) ?? unreachable, 0 as usize);

    const mass_a = ranks[0] + ranks[1] + ranks[2];
    const mass_b = ranks[3] + ranks[4] + ranks[5];
    expect_eq(mass_a > 0.80, true);
    expect_eq(mass_a > mass_b * 4.0, true);
    // Direct neighbor 1 of seed 0 should outrank remote node 4
    expect_eq(ranks[1] > ranks[4], true);

    // Switch seed to node 3 in Community B
    const tp_b: [6]f64 = [0.0, 0.0, 0.0, 5.0, 0.0, 0.0];
    const rb = personalizedPagerank(g, tp_b[0..], 0.75, 1.0e-13, 500, ranks[0..], scratch[0..]) ?? unreachable;
    expect_eq(rb.converged, true);
    expect_eq(absDiff(sumScores(ranks[0..]), 1.0) < 1.0e-12, true);
    expect_eq(topNode(ranks[0..]) ?? unreachable, 3 as usize);
    expect_eq((ranks[3] + ranks[4] + ranks[5]) > (ranks[0] + ranks[1] + ranks[2]) * 4.0, true);
});

test("5. HITS on bipartite Hub->Authority graph cleanly separates hubs, authorities & handles edgeless graph", () => {
    // Bipartite graph: Hubs {0, 1, 2} -> Authorities {3, 4, 5}
    // 0 -> [3, 4, 5] (strongest hub)
    // 1 -> [3, 4]    (medium hub)
    // 2 -> [3]       (weakest hub)
    // 3, 4, 5 -> []  (pure authorities, out-degree 0)
    const row_ptr: [7]usize = [0, 3, 5, 6, 6, 6, 6];
    const col_idx: [6]usize = [3, 4, 5, 3, 4, 3];
    const g = CsrGraph {
        num_nodes: 6,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
    };
    expect_eq(g.validate(), true);

    let hubs: [6]f64 = std.mem.zeroes([6]f64);
    let auths: [6]f64 = std.mem.zeroes([6]f64);
    let nh: [6]f64 = std.mem.zeroes([6]f64);
    let na: [6]f64 = std.mem.zeroes([6]f64);

    const res = hits(g, 1.0e-13, 500, hubs[0..], auths[0..], nh[0..], na[0..]) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(absDiff(l2Norm(hubs[0..]), 1.0) < 1.0e-12, true);
    expect_eq(absDiff(l2Norm(auths[0..]), 1.0) < 1.0e-12, true);

    // Pure authorities have 0 hub score; pure hubs have 0 authority score
    expect_eq(hubs[3], 0.0);
    expect_eq(hubs[4], 0.0);
    expect_eq(hubs[5], 0.0);
    expect_eq(auths[0], 0.0);
    expect_eq(auths[1], 0.0);
    expect_eq(auths[2], 0.0);

    // Strict monotonic ordering of hubs and authorities
    expect_eq(hubs[0] > hubs[1], true);
    expect_eq(hubs[1] > hubs[2], true);
    expect_eq(hubs[2] > 0.0, true);
    expect_eq(auths[3] > auths[4], true);
    expect_eq(auths[4] > auths[5], true);
    expect_eq(auths[5] > 0.0, true);

    expect_eq(topNode(hubs[0..]) ?? unreachable, 0 as usize);
    expect_eq(topNode(auths[0..]) ?? unreachable, 3 as usize);

    // Edgeless 3-node graph must converge cleanly to zeros without NaN
    const empty_rp: [4]usize = [0, 0, 0, 0];
    const empty_ci: [0]usize = [];
    const g_empty = CsrGraph {
        num_nodes: 3,
        row_ptr: empty_rp[0..],
        col_idx: empty_ci[0..],
    };
    let h3: [3]f64 = std.mem.zeroes([3]f64);
    let a3: [3]f64 = std.mem.zeroes([3]f64);
    let nh3: [3]f64 = std.mem.zeroes([3]f64);
    let na3: [3]f64 = std.mem.zeroes([3]f64);
    const re = hits(g_empty, 1.0e-12, 50, h3[0..], a3[0..], nh3[0..], na3[0..]) ?? unreachable;
    expect_eq(re.converged, true);
    expect_eq(sumScores(h3[0..]), 0.0);
    expect_eq(sumScores(a3[0..]), 0.0);
});

test("6. Malformed CSR, invalid damping/tol/teleport & undersized buffer rejection", () => {
    let ranks: [4]f64 = std.mem.zeroes([4]f64);
    let scratch: [4]f64 = std.mem.zeroes([4]f64);
    let small: [2]f64 = std.mem.zeroes([2]f64);

    // 1. Out-of-bounds col_idx (>= num_nodes)
    const rp_ok: [4]usize = [0, 1, 2, 2];
    const ci_oob: [2]usize = [1, 3]; // node 3 >= num_nodes(3)
    const g_oob = CsrGraph { num_nodes: 3, row_ptr: rp_ok[0..], col_idx: ci_oob[0..] };
    expect_eq(g_oob.validate(), false);
    expect_eq(pagerank(g_oob, 0.85, 1.0e-9, 100, ranks[0..], scratch[0..]) == null, true);

    // 2. Non-monotonic row_ptr
    const rp_nonmono: [4]usize = [0, 2, 1, 2];
    const ci_ok: [2]usize = [1, 0];
    const g_nonmono = CsrGraph { num_nodes: 3, row_ptr: rp_nonmono[0..], col_idx: ci_ok[0..] };
    expect_eq(g_nonmono.validate(), false);
    expect_eq(pagerank(g_nonmono, 0.85, 1.0e-9, 100, ranks[0..], scratch[0..]) == null, true);

    // 3. row_ptr[0] != 0
    const rp_bad0: [4]usize = [1, 1, 2, 2];
    const g_bad0 = CsrGraph { num_nodes: 3, row_ptr: rp_bad0[0..], col_idx: ci_ok[0..] };
    expect_eq(g_bad0.validate(), false);

    // 4. row_ptr length mismatch & row_ptr[n] > col_idx.len
    const rp_short: [3]usize = [0, 1, 2];
    const g_short = CsrGraph { num_nodes: 3, row_ptr: rp_short[0..], col_idx: ci_ok[0..] };
    expect_eq(g_short.validate(), false);

    const rp_over: [4]usize = [0, 1, 2, 5];
    const g_over = CsrGraph { num_nodes: 3, row_ptr: rp_over[0..], col_idx: ci_ok[0..] };
    expect_eq(g_over.validate(), false);

    // 5. Empty graph (num_nodes == 0) rejected by algorithms
    const rp_zero: [1]usize = [0];
    const ci_zero: [0]usize = [];
    const g_zero = CsrGraph { num_nodes: 0, row_ptr: rp_zero[0..], col_idx: ci_zero[0..] };
    expect_eq(g_zero.validate(), true);
    expect_eq(pagerank(g_zero, 0.85, 1.0e-9, 100, ranks[0..], scratch[0..]) == null, true);

    // 6. Valid 3-node graph with invalid damping, tol, teleport, and undersized buffers
    const g_valid = CsrGraph { num_nodes: 3, row_ptr: rp_ok[0..], col_idx: ci_ok[0..] };
    expect_eq(g_valid.validate(), true);
    expect_eq(g_valid.outDegree(99), 0 as usize);
    expect_eq(g_valid.neighbors(99).len, 0 as usize);

    expect_eq(pagerank(g_valid, 0.0, 1.0e-9, 100, ranks[0..], scratch[0..]) == null, true);
    expect_eq(pagerank(g_valid, 1.0, 1.0e-9, 100, ranks[0..], scratch[0..]) == null, true);
    expect_eq(pagerank(g_valid, -0.5, 1.0e-9, 100, ranks[0..], scratch[0..]) == null, true);
    expect_eq(pagerank(g_valid, 0.85, 0.0, 100, ranks[0..], scratch[0..]) == null, true);
    expect_eq(pagerank(g_valid, 0.85, -1.0e-6, 100, ranks[0..], scratch[0..]) == null, true);
    expect_eq(pagerank(g_valid, 0.85, 1.0e-9, 100, small[0..], scratch[0..]) == null, true);
    expect_eq(pagerank(g_valid, 0.85, 1.0e-9, 100, ranks[0..], small[0..]) == null, true);

    const tp_zero: [3]f64 = [0.0, 0.0, 0.0];
    const tp_neg: [3]f64 = [1.0, -0.2, 0.5];
    const tp_short: [2]f64 = [1.0, 1.0];
    expect_eq(personalizedPagerank(g_valid, tp_zero[0..], 0.85, 1.0e-9, 100, ranks[0..], scratch[0..]) == null, true);
    expect_eq(personalizedPagerank(g_valid, tp_neg[0..], 0.85, 1.0e-9, 100, ranks[0..], scratch[0..]) == null, true);
    expect_eq(personalizedPagerank(g_valid, tp_short[0..], 0.85, 1.0e-9, 100, ranks[0..], scratch[0..]) == null, true);

    expect_eq(hits(g_valid, 0.0, 100, ranks[0..], scratch[0..], ranks[0..], scratch[0..]) == null, true);
    expect_eq(hits(g_valid, 1.0e-9, 100, small[0..], scratch[0..], ranks[0..], scratch[0..]) == null, true);
    expect_eq(hits(g_oob, 1.0e-9, 100, ranks[0..], scratch[0..], ranks[0..], scratch[0..]) == null, true);

    // topNode on empty slice returns null
    const empty_scores: [0]f64 = [];
    expect_eq(topNode(empty_scores[0..]) == null, true);
});
