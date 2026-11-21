// Source: cp/cap/test/test_spectral_production_deep.cp (Production deep tests for cap:spectral)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LaplacianKind,
    buildLaplacian,
    fiedlerBisection,
    spectralEmbedding,
    normalizedCut,
    rbfAffinityMatrix,
} from "cap:spectral";

const TWO_PI: f64 = 6.28318530717958647692;

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn fillTwoCliques(adj: *[]f64, bridge_weight: f64) {
    let i: usize = 0;
    while (i < 64) {
        adj.*[i] = 0.0;
        i += 1;
    }
    // Clique A: {0, 1, 2, 3}, Clique B: {4, 5, 6, 7}
    let a: usize = 0;
    while (a < 4) {
        let b: usize = a + 1;
        while (b < 4) {
            adj.*[a * 8 + b] = 1.0;
            adj.*[b * 8 + a] = 1.0;
            adj.*[(a + 4) * 8 + (b + 4)] = 1.0;
            adj.*[(b + 4) * 8 + (a + 4)] = 1.0;
            b += 1;
        }
        a += 1;
    }
    if (bridge_weight > 0.0) {
        adj.*[3 * 8 + 4] = bridge_weight;
        adj.*[4 * 8 + 3] = bridge_weight;
    }
}

fn checkExactTwoGroupSplit(part: []const u8, split_idx: usize): bool {
    const g0 = part[0];
    const g1 = part[split_idx];
    if (g0 == g1) {
        return false;
    }
    let i: usize = 0;
    while (i < split_idx) {
        if (part[i] != g0) {
            return false;
        }
        i += 1;
    }
    let j = split_idx;
    while (j < part.len) {
        if (part[j] != g1) {
            return false;
        }
        j += 1;
    }
    return true;
}

test("1. Two bridged 4-cliques: Fiedler bisection 100% exact & low Normalized Cut", () => {
    let adj_buf: [64]f64 = [0.0; 64];
    let adj: []f64 = adj_buf[0..];
    fillTwoCliques(*adj, 1.0);

    let fvec_b: [8]f64 = [0.0; 8];
    let part_b: [8]u8 = [0; 8];
    let lap_b: [64]f64 = [0.0; 64];
    let deg_b: [8]f64 = [0.0; 8];
    let evals_b: [8]f64 = [0.0; 8];
    let evecs_b: [64]f64 = [0.0; 64];

    let fvec: []f64 = fvec_b[0..];
    let part: []u8 = part_b[0..];
    let lap: []f64 = lap_b[0..];
    let deg: []f64 = deg_b[0..];
    let evals: []f64 = evals_b[0..];
    let evecs: []f64 = evecs_b[0..];

    // Unnormalized Laplacian
    if (res := fiedlerBisection(adj, 8, LaplacianKind.Unnormalized, *fvec, *part, *lap, *deg, *evals, *evecs, 100, 1.0e-12)) {
        expect_eq(res.connected_components, 1);
        expect_eq(res.algebraic_connectivity > 0.1, true);
        expect_eq(checkExactTwoGroupSplit(part, 4), true);

        // Compare NCut of spectral partition vs interleaved bad partition
        const bad_part: [8]u8 = [0, 1, 0, 1, 0, 1, 0, 1];
        if (ncut_good := normalizedCut(adj, 8, part)) {
            if (ncut_bad := normalizedCut(adj, 8, bad_part[0..])) {
                // Good cut = 1/13 + 1/13 = 2/13 ≈ 0.1538; bad cut is much larger
                expect_eq(absF64(ncut_good - (2.0 / 13.0)) < 1.0e-12, true);
                expect_eq(ncut_good < ncut_bad * 0.25, true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Symmetric normalized Laplacian
    if (res_sym := fiedlerBisection(adj, 8, LaplacianKind.SymmetricNormalized, *fvec, *part, *lap, *deg, *evals, *evecs, 100, 1.0e-12)) {
        expect_eq(res_sym.connected_components, 1);
        expect_eq(res_sym.algebraic_connectivity > 0.05, true);
        expect_eq(checkExactTwoGroupSplit(part, 4), true);
    } else {
        expect_eq(true, false);
    }
});

test("2. Disconnected 2-component graph: connected_components == 2 & lambda_2 ~ 0", () => {
    let adj_buf: [64]f64 = [0.0; 64];
    let adj: []f64 = adj_buf[0..];
    fillTwoCliques(*adj, 0.0); // No bridge

    let fvec_b: [8]f64 = [0.0; 8];
    let part_b: [8]u8 = [0; 8];
    let lap_b: [64]f64 = [0.0; 64];
    let deg_b: [8]f64 = [0.0; 8];
    let evals_b: [8]f64 = [0.0; 8];
    let evecs_b: [64]f64 = [0.0; 64];

    let fvec: []f64 = fvec_b[0..];
    let part: []u8 = part_b[0..];
    let lap: []f64 = lap_b[0..];
    let deg: []f64 = deg_b[0..];
    let evals: []f64 = evals_b[0..];
    let evecs: []f64 = evecs_b[0..];

    if (res := fiedlerBisection(adj, 8, LaplacianKind.SymmetricNormalized, *fvec, *part, *lap, *deg, *evals, *evecs, 100, 1.0e-12)) {
        expect_eq(res.connected_components, 2);
        expect_eq(res.algebraic_connectivity < 1.0e-10, true);
    } else {
        expect_eq(true, false);
    }
});

test("3. Two concentric circles non-convex point cloud separated 100% via RBF + Fiedler", () => {
    // 16 points in 2D: 8 on inner circle (r = 1.0), 8 on outer circle (r = 4.0)
    let pts: [32]f64 = [0.0; 32];
    let i: usize = 0;
    while (i < 8) {
        const theta = TWO_PI * (i as f64) / 8.0;
        pts[i * 2] = 1.0 * @cos(theta);
        pts[i * 2 + 1] = 1.0 * @sin(theta);
        pts[(i + 8) * 2] = 4.0 * @cos(theta);
        pts[(i + 8) * 2 + 1] = 4.0 * @sin(theta);
        i += 1;
    }

    let adj_b: [256]f64 = [0.0; 256];
    let adj: []f64 = adj_b[0..];
    expect_eq(rbfAffinityMatrix(pts[0..], 16, 2, 1.2, *adj), true);
    // Diagonal of RBF affinity must be strictly 0
    expect_eq(adj[0], 0.0);
    expect_eq(adj[15 * 16 + 15], 0.0);

    let fvec_b: [16]f64 = [0.0; 16];
    let part_b: [16]u8 = [0; 16];
    let lap_b: [256]f64 = [0.0; 256];
    let deg_b: [16]f64 = [0.0; 16];
    let evals_b: [16]f64 = [0.0; 16];
    let evecs_b: [256]f64 = [0.0; 256];

    let fvec: []f64 = fvec_b[0..];
    let part: []u8 = part_b[0..];
    let lap: []f64 = lap_b[0..];
    let deg: []f64 = deg_b[0..];
    let evals: []f64 = evals_b[0..];
    let evecs: []f64 = evecs_b[0..];

    if (res := fiedlerBisection(adj, 16, LaplacianKind.SymmetricNormalized, *fvec, *part, *lap, *deg, *evals, *evecs, 150, 1.0e-12)) {
        expect_eq(res.connected_components, 1);
        expect_eq(checkExactTwoGroupSplit(part, 8), true);
    } else {
        expect_eq(true, false);
    }
});

test("4. NJW spectralEmbedding produces unit-norm rows & tight intra-cluster geometry", () => {
    let adj_buf: [64]f64 = [0.0; 64];
    let adj: []f64 = adj_buf[0..];
    fillTwoCliques(*adj, 0.5);

    let emb_b: [16]f64 = [0.0; 16];
    let lap_b: [64]f64 = [0.0; 64];
    let deg_b: [8]f64 = [0.0; 8];
    let evals_b: [8]f64 = [0.0; 8];
    let evecs_b: [64]f64 = [0.0; 64];

    let emb: []f64 = emb_b[0..];
    let lap: []f64 = lap_b[0..];
    let deg: []f64 = deg_b[0..];
    let evals: []f64 = evals_b[0..];
    let evecs: []f64 = evecs_b[0..];

    expect_eq(spectralEmbedding(adj, 8, 2, *emb, *lap, *deg, *evals, *evecs, 100, 1.0e-12), true);

    // Every row must have unit L2 norm
    let r: usize = 0;
    while (r < 8) {
        const norm = @sqrt(emb[r * 2] * emb[r * 2] + emb[r * 2 + 1] * emb[r * 2 + 1]);
        expect_eq(absF64(norm - 1.0) < 1.0e-12, true);
        r += 1;
    }

    // Intra-clique distance (0 vs 1) must be much smaller than inter-clique distance (0 vs 5)
    const d_intra = @sqrt((emb[0] - emb[2]) * (emb[0] - emb[2]) + (emb[1] - emb[3]) * (emb[1] - emb[3]));
    const d_inter = @sqrt((emb[0] - emb[10]) * (emb[0] - emb[10]) + (emb[1] - emb[11]) * (emb[1] - emb[11]));
    expect_eq(d_intra < 0.05, true);
    expect_eq(d_inter > 1.0, true);
});

test("5. buildLaplacian handles isolated vertices safely & satisfies zero row-sum for unnormalized L", () => {
    // 3 vertices: edge (0,1) weight 2.0, vertex 2 isolated (deg = 0)
    const adj: [9]f64 = [
        0.0, 2.0, 0.0,
        2.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
    ];
    let lap_b: [9]f64 = [0.0; 9];
    let deg_b: [3]f64 = [0.0; 3];
    let lap: []f64 = lap_b[0..];
    let deg: []f64 = deg_b[0..];

    expect_eq(buildLaplacian(adj[0..], 3, LaplacianKind.Unnormalized, *lap, *deg), true);
    expect_eq(deg[0], 2.0);
    expect_eq(deg[2], 0.0);
    // Unnormalized Laplacian rows sum to 0
    let r: usize = 0;
    while (r < 3) {
        const s = lap[r * 3] + lap[r * 3 + 1] + lap[r * 3 + 2];
        expect_eq(absF64(s) < 1.0e-15, true);
        r += 1;
    }

    // SymmetricNormalized with isolated vertex 2 must have 0 on row/col 2 (no NaN)
    expect_eq(buildLaplacian(adj[0..], 3, LaplacianKind.SymmetricNormalized, *lap, *deg), true);
    expect_eq(absF64(lap[0] - 1.0) < 1.0e-12, true);
    expect_eq(absF64(lap[1] + 1.0) < 1.0e-12, true);
    expect_eq(lap[8], 0.0);
});

test("6. Boundary defense: asymmetric/negative adj, k == 0, k > n, short buffers & NaN", () => {
    const nan = std.math.nan(f64);
    const asym: [4]f64 = [0.0, 1.0, 0.0, 0.0];
    const neg: [4]f64 = [0.0, -1.0, -1.0, 0.0];
    const adj_nan: [4]f64 = [0.0, nan, nan, 0.0];
    const adj_ok: [4]f64 = [0.0, 1.0, 1.0, 0.0];

    let lap_b: [4]f64 = [0.0; 4];
    let deg_b: [2]f64 = [0.0; 2];
    let evals_b: [2]f64 = [0.0; 2];
    let evecs_b: [4]f64 = [0.0; 4];
    let emb_b: [4]f64 = [0.0; 4];
    let short_b: [1]f64 = [0.0];

    let lap: []f64 = lap_b[0..];
    let deg: []f64 = deg_b[0..];
    let evals: []f64 = evals_b[0..];
    let evecs: []f64 = evecs_b[0..];
    let emb: []f64 = emb_b[0..];
    let short_s: []f64 = short_b[0..];

    expect_eq(buildLaplacian(asym[0..], 2, LaplacianKind.Unnormalized, *lap, *deg), false);
    expect_eq(buildLaplacian(neg[0..], 2, LaplacianKind.Unnormalized, *lap, *deg), false);
    expect_eq(buildLaplacian(adj_nan[0..], 2, LaplacianKind.Unnormalized, *lap, *deg), false);
    expect_eq(buildLaplacian(adj_ok[0..], 2, LaplacianKind.Unnormalized, *short_s, *deg), false);

    // spectralEmbedding invalid k and short buffers
    expect_eq(spectralEmbedding(adj_ok[0..], 2, 0, *emb, *lap, *deg, *evals, *evecs, 50, 1.0e-10), false);
    expect_eq(spectralEmbedding(adj_ok[0..], 2, 3, *emb, *lap, *deg, *evals, *evecs, 50, 1.0e-10), false);
    expect_eq(spectralEmbedding(adj_ok[0..], 2, 2, *short_s, *lap, *deg, *evals, *evecs, 50, 1.0e-10), false);

    // normalizedCut degenerate/invalid partition
    const all_zero: [2]u8 = [0, 0];
    const bad_label: [2]u8 = [0, 2];
    expect_eq(normalizedCut(adj_ok[0..], 2, all_zero[0..]) == null, true);
    expect_eq(normalizedCut(adj_ok[0..], 2, bad_label[0..]) == null, true);

    // rbfAffinityMatrix invalid sigma & NaN points
    const pts_ok: [4]f64 = [0.0, 0.0, 1.0, 1.0];
    const pts_nan: [4]f64 = [0.0, nan, 1.0, 1.0];
    expect_eq(rbfAffinityMatrix(pts_ok[0..], 2, 2, 0.0, *lap), false);
    expect_eq(rbfAffinityMatrix(pts_nan[0..], 2, 2, 1.0, *lap), false);
    expect_eq(rbfAffinityMatrix(pts_ok[0..], 2, 2, 1.0, *short_s), false);
});
