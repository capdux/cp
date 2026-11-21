// Source: cp/cap/test/test_louvain_production_deep.cp (Production deep tests for cap:louvain)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    WeightedGraph,
    LouvainConfig,
    LouvainWorkspace,
    computeModularity,
    louvainDetect,
    labelPropagation,
} from "cap:louvain";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

/// Helper: builds a symmetric weighted CSR graph of `num_cliques` cliques of size `clique_size`
/// connected in a ring by weak bridge edges of weight `bridge_w`.
fn buildRingOfCliques(
    num_cliques: usize,
    clique_size: usize,
    intra_w: f64,
    bridge_w: f64,
    row_ptr: *[]usize,
    col_idx: *[]usize,
    weights: *[]f64,
): usize {
    const n = num_cliques * clique_size;
    let cursor: usize = 0;
    let u: usize = 0;
    while (u < n) {
        row_ptr.*[u] = cursor;
        const c = u / clique_size;
        const local = u % clique_size;
        const c_start = c * clique_size;

        // Intra-clique edges
        let v: usize = c_start;
        while (v < c_start + clique_size) {
            if (v != u) {
                col_idx.*[cursor] = v;
                weights.*[cursor] = intra_w;
                cursor += 1;
            }
            v += 1;
        }

        // Weak bridge edge between last node of clique c and first node of clique (c + 1) % num_cliques
        if (num_cliques > 1 && bridge_w > 0.0) {
            if (local == clique_size - 1) {
                const next_c = (c + 1) % num_cliques;
                col_idx.*[cursor] = next_c * clique_size;
                weights.*[cursor] = bridge_w;
                cursor += 1;
            }
            if (local == 0) {
                const prev_c = (c + num_cliques - 1) % num_cliques;
                col_idx.*[cursor] = prev_c * clique_size + (clique_size - 1);
                weights.*[cursor] = bridge_w;
                cursor += 1;
            }
        }
        u += 1;
    }
    row_ptr.*[n] = cursor;
    return cursor;
}

test("1. Two 5-cliques joined by a weak bridge: Louvain finds 2 communities with Q > 0.4 and exact modularity match", () => {
    // 10 nodes: 2 * 5 * 4 = 40 intra edges + 4 directed bridge edges (ring of 2 has 2 bridges: 4-5 and 9-0)
    // Build a single weak bridge (4 <-> 5) explicitly for classic dumbbell graph.
    let rp_arr: [11]usize = std.mem.zeroes([11]usize);
    let ci_arr: [42]usize = std.mem.zeroes([42]usize);
    let w_arr: [42]f64 = std.mem.zeroes([42]f64);

    let cursor: usize = 0;
    let u: usize = 0;
    while (u < 10) {
        rp_arr[u] = cursor;
        const c_start: usize = if (u < 5) 0 else 5;
        let v: usize = c_start;
        while (v < c_start + 5) {
            if (v != u) {
                ci_arr[cursor] = v;
                w_arr[cursor] = 1.0;
                cursor += 1;
            }
            v += 1;
        }
        if (u == 4) {
            ci_arr[cursor] = 5;
            w_arr[cursor] = 0.2;
            cursor += 1;
        } else if (u == 5) {
            ci_arr[cursor] = 4;
            w_arr[cursor] = 0.2;
            cursor += 1;
        }
        u += 1;
    }
    rp_arr[10] = cursor;

    const g = WeightedGraph {
        num_nodes: 10,
        row_ptr: rp_arr[0..11],
        col_idx: ci_arr[0..cursor],
        weights: w_arr[0..cursor],
    };
    expect_eq(g.validate(), true);
    expect_eq(absF64(g.totalWeight() - 40.4) < 1.0e-12, true);

    let comm_arr: [10]usize = std.mem.zeroes([10]usize);
    let deg_arr: [10]f64 = std.mem.zeroes([10]f64);
    let tot_arr: [10]f64 = std.mem.zeroes([10]f64);
    let cw_arr: [10]f64 = std.mem.zeroes([10]f64);
    let touched_arr: [10]usize = std.mem.zeroes([10]usize);
    let relabel_arr: [10]i32 = std.mem.zeroes([10]i32);
    let ws = LouvainWorkspace {
        node_deg: deg_arr[0..],
        comm_tot: tot_arr[0..],
        comm_weights: cw_arr[0..],
        touched_comms: touched_arr[0..],
        relabel_map: relabel_arr[0..],
    };
    let comm_out: []usize = comm_arr[0..];

    const res = louvainDetect(g, LouvainConfig.default(), &comm_out, &ws) ?? unreachable;
    expect_eq(res.num_communities, 2);
    expect_eq(res.modularity > 0.4, true);

    // Clique 0 (0..4) all share one community; Clique 1 (5..9) share the other.
    const c0 = comm_arr[0];
    const c1 = comm_arr[5];
    expect_eq(c0 != c1, true);
    let i: usize = 0;
    while (i < 5) {
        expect_eq(comm_arr[i], c0);
        expect_eq(comm_arr[i + 5], c1);
        i += 1;
    }

    // Independent computeModularity matches louvainDetect result identically.
    let mod_in: [10]f64 = std.mem.zeroes([10]f64);
    let mod_tot: [10]f64 = std.mem.zeroes([10]f64);
    let mod_in_s: []f64 = mod_in[0..];
    let mod_tot_s: []f64 = mod_tot[0..];
    const q_check = computeModularity(g, comm_arr[0..], 1.0, &mod_in_s, &mod_tot_s) ?? unreachable;
    expect_eq(absF64(q_check - res.modularity) < 1.0e-12, true);

    // Label propagation also recovers the 2 communities cleanly.
    let lpa_lbl: [10]usize = std.mem.zeroes([10]usize);
    let lpa_w: [10]f64 = std.mem.zeroes([10]f64);
    let lpa_t: [10]usize = std.mem.zeroes([10]usize);
    let lpa_lbl_s: []usize = lpa_lbl[0..];
    let lpa_w_s: []f64 = lpa_w[0..];
    let lpa_t_s: []usize = lpa_t[0..];
    const lpa_comms = labelPropagation(g, 20, &lpa_lbl_s, &lpa_w_s, &lpa_t_s) ?? unreachable;
    expect_eq(lpa_comms, 2);
    expect_eq(lpa_lbl[0] != lpa_lbl[5], true);
});

test("2. Three-community weighted modular graph: exact recovery of all 3 communities", () => {
    // 12 nodes: three 4-cliques (intra_w = 2.0) linked in a ring by weak bridges (bridge_w = 0.1)
    let rp_arr: [13]usize = std.mem.zeroes([13]usize);
    let ci_arr: [64]usize = std.mem.zeroes([64]usize);
    let w_arr: [64]f64 = std.mem.zeroes([64]f64);
    let rp_s: []usize = rp_arr[0..];
    let ci_s: []usize = ci_arr[0..];
    let w_s: []f64 = w_arr[0..];

    const nnz = buildRingOfCliques(3, 4, 2.0, 0.1, &rp_s, &ci_s, &w_s);
    const g = WeightedGraph {
        num_nodes: 12,
        row_ptr: rp_arr[0..13],
        col_idx: ci_arr[0..nnz],
        weights: w_arr[0..nnz],
    };
    expect_eq(g.validate(), true);

    let comm_arr: [12]usize = std.mem.zeroes([12]usize);
    let deg_arr: [12]f64 = std.mem.zeroes([12]f64);
    let tot_arr: [12]f64 = std.mem.zeroes([12]f64);
    let cw_arr: [12]f64 = std.mem.zeroes([12]f64);
    let touched_arr: [12]usize = std.mem.zeroes([12]usize);
    let relabel_arr: [12]i32 = std.mem.zeroes([12]i32);
    let ws = LouvainWorkspace {
        node_deg: deg_arr[0..],
        comm_tot: tot_arr[0..],
        comm_weights: cw_arr[0..],
        touched_comms: touched_arr[0..],
        relabel_map: relabel_arr[0..],
    };
    let comm_out: []usize = comm_arr[0..];

    const res = louvainDetect(g, LouvainConfig.default(), &comm_out, &ws) ?? unreachable;
    expect_eq(res.num_communities, 3);
    expect_eq(res.modularity > 0.6, true);

    const c0 = comm_arr[0];
    const c1 = comm_arr[4];
    const c2 = comm_arr[8];
    expect_eq(c0 != c1 && c1 != c2 && c0 != c2, true);
    let k: usize = 0;
    while (k < 4) {
        expect_eq(comm_arr[k], c0);
        expect_eq(comm_arr[4 + k], c1);
        expect_eq(comm_arr[8 + k], c2);
        k += 1;
    }
});

test("3. Resolution parameter control: low resolution merges, high resolution splits", () => {
    let rp_arr: [13]usize = std.mem.zeroes([13]usize);
    let ci_arr: [64]usize = std.mem.zeroes([64]usize);
    let w_arr: [64]f64 = std.mem.zeroes([64]f64);
    let rp_s: []usize = rp_arr[0..];
    let ci_s: []usize = ci_arr[0..];
    let w_s: []f64 = w_arr[0..];

    const nnz = buildRingOfCliques(3, 4, 1.0, 0.25, &rp_s, &ci_s, &w_s);
    const g = WeightedGraph {
        num_nodes: 12,
        row_ptr: rp_arr[0..13],
        col_idx: ci_arr[0..nnz],
        weights: w_arr[0..nnz],
    };

    let comm_arr: [12]usize = std.mem.zeroes([12]usize);
    let deg_arr: [12]f64 = std.mem.zeroes([12]f64);
    let tot_arr: [12]f64 = std.mem.zeroes([12]f64);
    let cw_arr: [12]f64 = std.mem.zeroes([12]f64);
    let touched_arr: [12]usize = std.mem.zeroes([12]usize);
    let relabel_arr: [12]i32 = std.mem.zeroes([12]i32);
    let ws = LouvainWorkspace {
        node_deg: deg_arr[0..],
        comm_tot: tot_arr[0..],
        comm_weights: cw_arr[0..],
        touched_comms: touched_arr[0..],
        relabel_map: relabel_arr[0..],
    };
    let comm_out: []usize = comm_arr[0..];

    // Low resolution merges into 1 community
    let cfg_low = LouvainConfig.default();
    cfg_low.resolution = 0.05;
    const res_low = louvainDetect(g, cfg_low, &comm_out, &ws) ?? unreachable;
    expect_eq(res_low.num_communities, 1);

    // Standard resolution discovers 3 communities
    const res_mid = louvainDetect(g, LouvainConfig.default(), &comm_out, &ws) ?? unreachable;
    expect_eq(res_mid.num_communities, 3);

    // Very high resolution splits into finer communities (> 3)
    let cfg_high = LouvainConfig.default();
    cfg_high.resolution = 12.0;
    const res_high = louvainDetect(g, cfg_high, &comm_out, &ws) ?? unreachable;
    expect_eq(res_high.num_communities > 3, true);
});

test("4. Edge topologies: single complete clique K5 and isolated edgeless graph", () => {
    // 4a. Complete clique K5
    let rp_k5: [6]usize = std.mem.zeroes([6]usize);
    let ci_k5: [20]usize = std.mem.zeroes([20]usize);
    let w_k5: [20]f64 = std.mem.zeroes([20]f64);
    let rp_s: []usize = rp_k5[0..];
    let ci_s: []usize = ci_k5[0..];
    let w_s: []f64 = w_k5[0..];
    const nnz = buildRingOfCliques(1, 5, 1.0, 0.0, &rp_s, &ci_s, &w_s);
    const g_k5 = WeightedGraph {
        num_nodes: 5,
        row_ptr: rp_k5[0..6],
        col_idx: ci_k5[0..nnz],
        weights: w_k5[0..nnz],
    };

    let comm_arr: [5]usize = std.mem.zeroes([5]usize);
    let deg_arr: [5]f64 = std.mem.zeroes([5]f64);
    let tot_arr: [5]f64 = std.mem.zeroes([5]f64);
    let cw_arr: [5]f64 = std.mem.zeroes([5]f64);
    let touched_arr: [5]usize = std.mem.zeroes([5]usize);
    let relabel_arr: [5]i32 = std.mem.zeroes([5]i32);
    let ws = LouvainWorkspace {
        node_deg: deg_arr[0..],
        comm_tot: tot_arr[0..],
        comm_weights: cw_arr[0..],
        touched_comms: touched_arr[0..],
        relabel_map: relabel_arr[0..],
    };
    let comm_out: []usize = comm_arr[0..];

    const res_k5 = louvainDetect(g_k5, LouvainConfig.default(), &comm_out, &ws) ?? unreachable;
    expect_eq(res_k5.num_communities, 1);
    expect_eq(absF64(res_k5.modularity) < 1.0e-12, true);

    // 4b. Isolated edgeless graph (5 nodes, 0 edges)
    const rp_iso: [6]usize = [0, 0, 0, 0, 0, 0];
    const ci_iso: [0]usize = [];
    const w_iso: [0]f64 = [];
    const g_iso = WeightedGraph {
        num_nodes: 5,
        row_ptr: rp_iso[0..],
        col_idx: ci_iso[0..],
        weights: w_iso[0..],
    };
    expect_eq(g_iso.validate(), true);
    const res_iso = louvainDetect(g_iso, LouvainConfig.default(), &comm_out, &ws) ?? unreachable;
    expect_eq(res_iso.num_communities, 5);
    expect_eq(res_iso.modularity == 0.0, true);
    expect_eq(res_iso.moves, 0);
});

test("5. Defensive guards: invalid CSR, negative/NaN weights, invalid resolution, short buffers", () => {
    const valid_rp: [4]usize = [0, 1, 2, 2];
    const valid_ci: [2]usize = [1, 0];
    const valid_w: [2]f64 = [1.0, 1.0];
    const g_ok = WeightedGraph {
        num_nodes: 3,
        row_ptr: valid_rp[0..],
        col_idx: valid_ci[0..],
        weights: valid_w[0..],
    };
    expect_eq(g_ok.validate(), true);

    // Non-monotonic row_ptr
    const bad_rp: [4]usize = [0, 2, 1, 2];
    const g_bad_rp = WeightedGraph { num_nodes: 3, row_ptr: bad_rp[0..], col_idx: valid_ci[0..], weights: valid_w[0..] };
    expect_eq(g_bad_rp.validate(), false);

    // Out-of-bounds col_idx
    const bad_ci: [2]usize = [3, 0];
    const g_bad_ci = WeightedGraph { num_nodes: 3, row_ptr: valid_rp[0..], col_idx: bad_ci[0..], weights: valid_w[0..] };
    expect_eq(g_bad_ci.validate(), false);

    // Negative weight
    const neg_w: [2]f64 = [-1.0, 1.0];
    const g_neg = WeightedGraph { num_nodes: 3, row_ptr: valid_rp[0..], col_idx: valid_ci[0..], weights: neg_w[0..] };
    expect_eq(g_neg.validate(), false);

    // NaN weight
    const zero_f: f64 = 0.0;
    const nan_f: f64 = zero_f / zero_f;
    const nan_w: [2]f64 = [nan_f, 1.0];
    const g_nan = WeightedGraph { num_nodes: 3, row_ptr: valid_rp[0..], col_idx: valid_ci[0..], weights: nan_w[0..] };
    expect_eq(g_nan.validate(), false);

    let comm_arr: [3]usize = std.mem.zeroes([3]usize);
    let comm_short: [2]usize = std.mem.zeroes([2]usize);
    let deg_arr: [3]f64 = std.mem.zeroes([3]f64);
    let tot_arr: [3]f64 = std.mem.zeroes([3]f64);
    let cw_arr: [3]f64 = std.mem.zeroes([3]f64);
    let touched_arr: [3]usize = std.mem.zeroes([3]usize);
    let relabel_arr: [3]i32 = std.mem.zeroes([3]i32);
    let ws = LouvainWorkspace {
        node_deg: deg_arr[0..],
        comm_tot: tot_arr[0..],
        comm_weights: cw_arr[0..],
        touched_comms: touched_arr[0..],
        relabel_map: relabel_arr[0..],
    };
    let comm_out: []usize = comm_arr[0..];
    let comm_out_short: []usize = comm_short[0..];

    expect_eq(louvainDetect(g_bad_rp, LouvainConfig.default(), &comm_out, &ws) == null, true);
    expect_eq(louvainDetect(g_neg, LouvainConfig.default(), &comm_out, &ws) == null, true);
    expect_eq(louvainDetect(g_nan, LouvainConfig.default(), &comm_out, &ws) == null, true);
    expect_eq(louvainDetect(g_ok, LouvainConfig.default(), &comm_out_short, &ws) == null, true);

    let bad_cfg = LouvainConfig.default();
    bad_cfg.resolution = -0.5;
    expect_eq(louvainDetect(g_ok, bad_cfg, &comm_out, &ws) == null, true);
    bad_cfg.resolution = nan_f;
    expect_eq(louvainDetect(g_ok, bad_cfg, &comm_out, &ws) == null, true);

    let mod_in_s: []f64 = cw_arr[0..];
    let mod_tot_s: []f64 = tot_arr[0..];
    expect_eq(computeModularity(g_neg, comm_arr[0..], 1.0, &mod_in_s, &mod_tot_s) == null, true);
    expect_eq(computeModularity(g_ok, comm_arr[0..], -1.0, &mod_in_s, &mod_tot_s) == null, true);

    let lpa_w_s: []f64 = cw_arr[0..];
    let lpa_t_s: []usize = touched_arr[0..];
    expect_eq(labelPropagation(g_bad_ci, 10, &comm_out, &lpa_w_s, &lpa_t_s) == null, true);
    expect_eq(labelPropagation(g_ok, 10, &comm_out_short, &lpa_w_s, &lpa_t_s) == null, true);
});
