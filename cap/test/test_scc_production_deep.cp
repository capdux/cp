// Source: cp/cap/test/test_scc_production_deep.cp (Production deep tests for cap:scc)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CsrGraph,
    TarjanWorkspace,
    TwoSatClause,
    tarjanScc,
    topoSort,
    isDag,
    dagLongestPath,
    solve2Sat,
} from "cap:scc";

fn evalLiteral(lit: i32, assignment: []const u8): bool {
    if (lit > 0) {
        const idx: usize = (lit - 1) as usize;
        return assignment[idx] != 0;
    }
    const idx: usize = ((0 - lit) - 1) as usize;
    return assignment[idx] == 0;
}

test("1. Classic CLRS 8-node digraph yields 4 exact SCCs in strict reverse topological order", () => {
    // CLRS Fig 22.9: a=0, b=1, c=2, d=3, e=4, f=5, g=6, h=7
    // Edges:
    //   0 -> 1
    //   1 -> 2, 4, 5
    //   2 -> 3, 6
    //   3 -> 2, 7
    //   4 -> 0, 5
    //   5 -> 6
    //   6 -> 5, 7
    //   7 -> 7
    // Exact SCCs: {0, 1, 4}, {2, 3}, {5, 6}, {7}
    const row_ptr: [9]usize = [0, 1, 4, 6, 8, 10, 11, 13, 14];
    const col_idx: [14]usize = [1, 2, 4, 5, 3, 6, 2, 7, 0, 5, 6, 5, 7, 7];
    const g: CsrGraph = CsrGraph {
        num_nodes: 8,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
    };
    expect_eq(g.validate(), true);
    expect_eq(g.edgeCount(), 14 as usize);
    expect_eq(g.neighbors(1).len, 3 as usize);

    let disc: [8]i32 = std.mem.zeroes([8]i32);
    let low: [8]i32 = std.mem.zeroes([8]i32);
    let on_st: [8]u8 = std.mem.zeroes([8]u8);
    let tst: [8]usize = std.mem.zeroes([8]usize);
    let cn: [8]usize = std.mem.zeroes([8]usize);
    let ce: [8]usize = std.mem.zeroes([8]usize);
    let ws: TarjanWorkspace = TarjanWorkspace {
        disc: disc[0..],
        low: low[0..],
        on_stack: on_st[0..],
        tarjan_stack: tst[0..],
        call_node: cn[0..],
        call_edge: ce[0..],
    };

    let comp_buf: [8]usize = std.mem.zeroes([8]usize);
    let comp: []usize = comp_buf[0..];
    const num_sccs: usize = tarjanScc(g, *comp, *ws) ?? unreachable;
    expect_eq(num_sccs, 4 as usize);

    // Same-SCC checks
    expect_eq(comp[0], comp[1]);
    expect_eq(comp[1], comp[4]);
    expect_eq(comp[2], comp[3]);
    expect_eq(comp[5], comp[6]);

    // Distinct-SCC checks
    const c_abe: usize = comp[0];
    const c_cd: usize = comp[2];
    const c_fg: usize = comp[5];
    const c_h: usize = comp[7];
    expect_eq(c_abe != c_cd && c_abe != c_fg && c_abe != c_h, true);
    expect_eq(c_cd != c_fg && c_cd != c_h && c_fg != c_h, true);

    // Reverse topological order check: for every cross-SCC edge u -> v, comp[u] > comp[v]
    let u: usize = 0;
    while (u < 8) {
        const nbrs: []const usize = g.neighbors(u);
        let j: usize = 0;
        while (j < nbrs.len) {
            const v: usize = nbrs[j];
            if (comp[u] != comp[v]) {
                expect_eq(comp[u] > comp[v], true);
            }
            j += 1;
        }
        u += 1;
    }
});

test("2. Kahn topoSort, isDag & dagLongestPath on DAG vs cyclic graph", () => {
    // 6-node DAG:
    //   0 -> 1, 2
    //   1 -> 3
    //   2 -> 3, 4
    //   3 -> 5
    //   4 -> 5
    //   5 -> (none)
    // Longest path: 0 -> 1 -> 3 -> 5 (length 3)
    const row_ptr: [7]usize = [0, 2, 3, 5, 6, 7, 7];
    const col_idx: [7]usize = [1, 2, 3, 3, 4, 5, 5];
    const dag: CsrGraph = CsrGraph {
        num_nodes: 6,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
    };

    let order_buf: [6]usize = std.mem.zeroes([6]usize);
    let indeg_buf: [6]usize = std.mem.zeroes([6]usize);
    let queue_buf: [6]usize = std.mem.zeroes([6]usize);
    let dist_buf: [6]usize = std.mem.zeroes([6]usize);

    let order: []usize = order_buf[0..];
    let indeg: []usize = indeg_buf[0..];
    let queue: []usize = queue_buf[0..];
    let dist: []usize = dist_buf[0..];

    expect_eq(isDag(dag, *indeg, *queue, *order), true);
    expect_eq(topoSort(dag, *order, *indeg, *queue), true);

    // Build inverse position map and verify pos[u] < pos[v] for every edge u -> v
    let pos: [6]usize = std.mem.zeroes([6]usize);
    let p: usize = 0;
    while (p < 6) {
        pos[order[p]] = p;
        p += 1;
    }
    let u: usize = 0;
    while (u < 6) {
        const nbrs: []const usize = dag.neighbors(u);
        let j: usize = 0;
        while (j < nbrs.len) {
            expect_eq(pos[u] < pos[nbrs[j]], true);
            j += 1;
        }
        u += 1;
    }

    const longest: usize = dagLongestPath(dag, *dist, *order, *indeg, *queue) ?? unreachable;
    expect_eq(longest, 3 as usize);
    expect_eq(dist[0], 0 as usize);
    expect_eq(dist[5], 3 as usize);

    // Add back-edge 5 -> 1 to create cycle 1 -> 3 -> 5 -> 1
    const cyc_row: [7]usize = [0, 2, 3, 5, 6, 7, 8];
    const cyc_col: [8]usize = [1, 2, 3, 3, 4, 5, 5, 1];
    const cyclic: CsrGraph = CsrGraph {
        num_nodes: 6,
        row_ptr: cyc_row[0..],
        col_idx: cyc_col[0..],
    };

    expect_eq(topoSort(cyclic, *order, *indeg, *queue), false);
    expect_eq(isDag(cyclic, *indeg, *queue, *order), false);
    expect_eq(dagLongestPath(cyclic, *dist, *order, *indeg, *queue) == null, true);
});

test("3. solve2Sat finds valid satisfying assignment for satisfiable 4-variable CNF", () => {
    // Clauses over x1, x2, x3, x4:
    // (x1 OR x2), (~x1 OR x3), (~x2 OR ~x3), (x3 OR x4), (~x4 OR x1), (x2 OR x4)
    const clauses: [6]TwoSatClause = [
        TwoSatClause { a: 1, b: 2 },
        TwoSatClause { a: -1, b: 3 },
        TwoSatClause { a: -2, b: -3 },
        TwoSatClause { a: 3, b: 4 },
        TwoSatClause { a: -4, b: 1 },
        TwoSatClause { a: 2, b: 4 },
    ];

    let assign_buf: [4]u8 = std.mem.zeroes([4]u8);
    let rp_buf: [9]usize = std.mem.zeroes([9]usize);
    let ci_buf: [12]usize = std.mem.zeroes([12]usize);
    let comp_buf: [8]usize = std.mem.zeroes([8]usize);

    let disc: [8]i32 = std.mem.zeroes([8]i32);
    let low: [8]i32 = std.mem.zeroes([8]i32);
    let on_st: [8]u8 = std.mem.zeroes([8]u8);
    let tst: [8]usize = std.mem.zeroes([8]usize);
    let cn: [8]usize = std.mem.zeroes([8]usize);
    let ce: [8]usize = std.mem.zeroes([8]usize);
    let ws: TarjanWorkspace = TarjanWorkspace {
        disc: disc[0..],
        low: low[0..],
        on_stack: on_st[0..],
        tarjan_stack: tst[0..],
        call_node: cn[0..],
        call_edge: ce[0..],
    };

    let assign: []u8 = assign_buf[0..];
    let rp: []usize = rp_buf[0..];
    let ci: []usize = ci_buf[0..];
    let comp: []usize = comp_buf[0..];

    const sat: bool = solve2Sat(4, clauses[0..], *assign, *rp, *ci, *comp, *ws);
    expect_eq(sat, true);

    // Every clause must evaluate to true under the returned assignment
    let idx: usize = 0;
    while (idx < clauses.len) {
        const cl: TwoSatClause = clauses[idx];
        const ok: bool = evalLiteral(cl.a, assign) || evalLiteral(cl.b, assign);
        expect_eq(ok, true);
        idx += 1;
    }
});

test("4. solve2Sat rejects unsatisfiable contradiction and invalid literals", () => {
    // Unsatisfiable 2-variable contradiction:
    // (x1 OR x2) AND (~x1 OR x2) AND (x1 OR ~x2) AND (~x1 OR ~x2)
    const unsat_clauses: [4]TwoSatClause = [
        TwoSatClause { a: 1, b: 2 },
        TwoSatClause { a: -1, b: 2 },
        TwoSatClause { a: 1, b: -2 },
        TwoSatClause { a: -1, b: -2 },
    ];

    let assign_buf: [2]u8 = std.mem.zeroes([2]u8);
    let rp_buf: [5]usize = std.mem.zeroes([5]usize);
    let ci_buf: [8]usize = std.mem.zeroes([8]usize);
    let comp_buf: [4]usize = std.mem.zeroes([4]usize);

    let disc: [4]i32 = std.mem.zeroes([4]i32);
    let low: [4]i32 = std.mem.zeroes([4]i32);
    let on_st: [4]u8 = std.mem.zeroes([4]u8);
    let tst: [4]usize = std.mem.zeroes([4]usize);
    let cn: [4]usize = std.mem.zeroes([4]usize);
    let ce: [4]usize = std.mem.zeroes([4]usize);
    let ws: TarjanWorkspace = TarjanWorkspace {
        disc: disc[0..],
        low: low[0..],
        on_stack: on_st[0..],
        tarjan_stack: tst[0..],
        call_node: cn[0..],
        call_edge: ce[0..],
    };

    let assign: []u8 = assign_buf[0..];
    let rp: []usize = rp_buf[0..];
    let ci: []usize = ci_buf[0..];
    let comp: []usize = comp_buf[0..];

    expect_eq(solve2Sat(2, unsat_clauses[0..], *assign, *rp, *ci, *comp, *ws), false);

    // Invalid literal 0 or out-of-range variable index
    const zero_lit: [1]TwoSatClause = [TwoSatClause { a: 0, b: 1 }];
    const oob_lit: [1]TwoSatClause = [TwoSatClause { a: 1, b: 3 }];
    expect_eq(solve2Sat(2, zero_lit[0..], *assign, *rp, *ci, *comp, *ws), false);
    expect_eq(solve2Sat(2, oob_lit[0..], *assign, *rp, *ci, *comp, *ws), false);
});

test("5. Edge cases: empty graph, single node, self-loop & invalid CSR rejection", () => {
    let disc: [4]i32 = std.mem.zeroes([4]i32);
    let low: [4]i32 = std.mem.zeroes([4]i32);
    let on_st: [4]u8 = std.mem.zeroes([4]u8);
    let tst: [4]usize = std.mem.zeroes([4]usize);
    let cn: [4]usize = std.mem.zeroes([4]usize);
    let ce: [4]usize = std.mem.zeroes([4]usize);
    let ws: TarjanWorkspace = TarjanWorkspace {
        disc: disc[0..],
        low: low[0..],
        on_stack: on_st[0..],
        tarjan_stack: tst[0..],
        call_node: cn[0..],
        call_edge: ce[0..],
    };

    let comp_buf: [4]usize = std.mem.zeroes([4]usize);
    let order_buf: [4]usize = std.mem.zeroes([4]usize);
    let indeg_buf: [4]usize = std.mem.zeroes([4]usize);
    let queue_buf: [4]usize = std.mem.zeroes([4]usize);
    let comp: []usize = comp_buf[0..];
    let order: []usize = order_buf[0..];
    let indeg: []usize = indeg_buf[0..];
    let queue: []usize = queue_buf[0..];

    // Empty graph (0 nodes)
    const empty_rp: [1]usize = [0];
    const empty_ci: [0]usize = [];
    const g_empty: CsrGraph = CsrGraph { num_nodes: 0, row_ptr: empty_rp[0..], col_idx: empty_ci[0..] };
    expect_eq(g_empty.validate(), true);
    expect_eq(tarjanScc(g_empty, *comp, *ws) ?? unreachable, 0 as usize);
    expect_eq(topoSort(g_empty, *order, *indeg, *queue), true);

    // Single node with no edges
    const single_rp: [2]usize = [0, 0];
    const g_single: CsrGraph = CsrGraph { num_nodes: 1, row_ptr: single_rp[0..], col_idx: empty_ci[0..] };
    expect_eq(tarjanScc(g_single, *comp, *ws) ?? unreachable, 1 as usize);
    expect_eq(comp[0], 0 as usize);
    expect_eq(topoSort(g_single, *order, *indeg, *queue), true);

    // Single node with self-loop (0 -> 0): 1 SCC, NOT a DAG
    const loop_rp: [2]usize = [0, 1];
    const loop_ci: [1]usize = [0];
    const g_loop: CsrGraph = CsrGraph { num_nodes: 1, row_ptr: loop_rp[0..], col_idx: loop_ci[0..] };
    expect_eq(tarjanScc(g_loop, *comp, *ws) ?? unreachable, 1 as usize);
    expect_eq(topoSort(g_loop, *order, *indeg, *queue), false);

    // Invalid CSRs: target out of range, non-monotonic row_ptr, row_ptr[0] != 0, short row_ptr
    const bad_target_ci: [1]usize = [2];
    const g_bad1: CsrGraph = CsrGraph { num_nodes: 1, row_ptr: loop_rp[0..], col_idx: bad_target_ci[0..] };
    expect_eq(g_bad1.validate(), false);
    expect_eq(tarjanScc(g_bad1, *comp, *ws) == null, true);
    expect_eq(topoSort(g_bad1, *order, *indeg, *queue), false);

    const non_mono_rp: [3]usize = [0, 2, 1];
    const dummy_ci: [2]usize = [0, 1];
    const g_bad2: CsrGraph = CsrGraph { num_nodes: 2, row_ptr: non_mono_rp[0..], col_idx: dummy_ci[0..] };
    expect_eq(g_bad2.validate(), false);
    expect_eq(tarjanScc(g_bad2, *comp, *ws) == null, true);

    const nonzero_rp: [2]usize = [1, 1];
    const g_bad3: CsrGraph = CsrGraph { num_nodes: 1, row_ptr: nonzero_rp[0..], col_idx: dummy_ci[0..] };
    expect_eq(g_bad3.validate(), false);

    const short_rp: [1]usize = [0];
    const g_bad4: CsrGraph = CsrGraph { num_nodes: 2, row_ptr: short_rp[0..], col_idx: dummy_ci[0..] };
    expect_eq(g_bad4.validate(), false);
});

test("6. Iterative Tarjan & DAG longest path on 100-node deep chain without recursion", () => {
    // 100-node directed cycle 0 -> 1 -> ... -> 99 -> 0: single SCC
    let rp_cyc: [101]usize = std.mem.zeroes([101]usize);
    let ci_cyc: [100]usize = std.mem.zeroes([100]usize);
    let i: usize = 0;
    while (i < 100) {
        rp_cyc[i] = i;
        ci_cyc[i] = (i + 1) % 100;
        i += 1;
    }
    rp_cyc[100] = 100;

    let disc: [100]i32 = std.mem.zeroes([100]i32);
    let low: [100]i32 = std.mem.zeroes([100]i32);
    let on_st: [100]u8 = std.mem.zeroes([100]u8);
    let tst: [100]usize = std.mem.zeroes([100]usize);
    let cn: [100]usize = std.mem.zeroes([100]usize);
    let ce: [100]usize = std.mem.zeroes([100]usize);
    let ws: TarjanWorkspace = TarjanWorkspace {
        disc: disc[0..],
        low: low[0..],
        on_stack: on_st[0..],
        tarjan_stack: tst[0..],
        call_node: cn[0..],
        call_edge: ce[0..],
    };

    let comp_buf: [100]usize = std.mem.zeroes([100]usize);
    let comp: []usize = comp_buf[0..];
    const g_cyc: CsrGraph = CsrGraph { num_nodes: 100, row_ptr: rp_cyc[0..], col_idx: ci_cyc[0..] };
    expect_eq(tarjanScc(g_cyc, *comp, *ws) ?? unreachable, 1 as usize);

    // Break edge 99 -> 0 to form 100-node path DAG: 100 SCCs, longest path == 99
    rp_cyc[100] = 99;
    const g_chain: CsrGraph = CsrGraph { num_nodes: 100, row_ptr: rp_cyc[0..], col_idx: ci_cyc[0..99] };
    expect_eq(tarjanScc(g_chain, *comp, *ws) ?? unreachable, 100 as usize);
    // Reverse topological order along chain: comp[0] == 99, comp[99] == 0
    expect_eq(comp[0], 99 as usize);
    expect_eq(comp[99], 0 as usize);

    let dist_buf: [100]usize = std.mem.zeroes([100]usize);
    let order_buf: [100]usize = std.mem.zeroes([100]usize);
    let indeg_buf: [100]usize = std.mem.zeroes([100]usize);
    let queue_buf: [100]usize = std.mem.zeroes([100]usize);
    let dist: []usize = dist_buf[0..];
    let order: []usize = order_buf[0..];
    let indeg: []usize = indeg_buf[0..];
    let queue: []usize = queue_buf[0..];

    const lp: usize = dagLongestPath(g_chain, *dist, *order, *indeg, *queue) ?? unreachable;
    expect_eq(lp, 99 as usize);
    expect_eq(order[0], 0 as usize);
    expect_eq(order[99], 99 as usize);
});
