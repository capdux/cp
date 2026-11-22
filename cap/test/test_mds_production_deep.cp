// Source: cp/cap/test/test_mds_production_deep.cp (Production deep tests for cap:mds)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    pairwiseEuclidean,
    kruskalStress1,
    doubleCenterGram,
    classicalMds,
    smacofRefine,
} from "cap:ml";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

test("1. Classical MDS recovers 2D square and 3D regular tetrahedron with zero error (stress1 < 1e-12)", () => {
    // 1a. 2D unit square (4 points)
    const sq_pts = [
        0.0, 0.0,
        2.0, 0.0,
        2.0, 2.0,
        0.0, 2.0,
    ];
    let dist_arr: [16]f64 = std.mem.zeroes([16]f64);
    let dist_s: []f64 = dist_arr[0..];
    expect_eq(pairwiseEuclidean(sq_pts[0..], 4, 2, &dist_s), true);

    let coords_arr: [8]f64 = std.mem.zeroes([8]f64);
    let gram_arr: [16]f64 = std.mem.zeroes([16]f64);
    let rmean_arr: [4]f64 = std.mem.zeroes([4]f64);
    let evals_arr: [4]f64 = std.mem.zeroes([4]f64);
    let evecs_arr: [16]f64 = std.mem.zeroes([16]f64);
    let coords_s: []f64 = coords_arr[0..];
    let gram_s: []f64 = gram_arr[0..];
    let rmean_s: []f64 = rmean_arr[0..];
    let evals_s: []f64 = evals_arr[0..];
    let evecs_s: []f64 = evecs_arr[0..];

    const res_sq = classicalMds(dist_arr[0..], 4, 2, &coords_s, &gram_s, &rmean_s, &evals_s, &evecs_s, 60, 1.0e-13) ?? unreachable;
    expect_eq(res_sq.positive_eigenvalues, 2);
    expect_eq(absF64(res_sq.variance_explained - 1.0) < 1.0e-12, true);
    expect_eq(res_sq.stress1 < 1.0e-12, true);

    // Reconstructed pairwise distances match original distances identically
    let rec_dist: [16]f64 = std.mem.zeroes([16]f64);
    let rec_s: []f64 = rec_dist[0..];
    expect_eq(pairwiseEuclidean(coords_arr[0..], 4, 2, &rec_s), true);
    let idx: usize = 0;
    while (idx < 16) {
        expect_eq(absF64(rec_dist[idx] - dist_arr[idx]) < 1.0e-11, true);
        idx += 1;
    }

    // 1b. 3D regular tetrahedron (4 vertices, all mutual distances = 1.0)
    const tet_dist: [16]f64 = [
        0.0, 1.0, 1.0, 1.0,
        1.0, 0.0, 1.0, 1.0,
        1.0, 1.0, 0.0, 1.0,
        1.0, 1.0, 1.0, 0.0,
    ];
    let tet_coords: [12]f64 = std.mem.zeroes([12]f64);
    let tet_cs: []f64 = tet_coords[0..];
    const res_tet = classicalMds(tet_dist[0..], 4, 3, &tet_cs, &gram_s, &rmean_s, &evals_s, &evecs_s, 60, 1.0e-13) ?? unreachable;
    expect_eq(res_tet.positive_eigenvalues, 3);
    expect_eq(absF64(res_tet.variance_explained - 1.0) < 1.0e-12, true);
    expect_eq(res_tet.stress1 < 1.0e-12, true);
});

test("2. High-dimensional 4D -> 2D low-rank reduction checks variance_explained and stress", () => {
    // 6 points in 4D: dominant energy in dims 0..1, small perturbation in dims 2..3
    const pts4d: [24]f64 = [
        -4.0, -3.0,  0.10, -0.05,
         4.0, -3.0, -0.10,  0.08,
         4.0,  3.0,  0.12,  0.06,
        -4.0,  3.0, -0.08, -0.09,
         0.0,  5.0,  0.05,  0.11,
         0.0, -5.0, -0.09, -0.11,
    ];
    let dist_arr: [36]f64 = std.mem.zeroes([36]f64);
    let dist_s: []f64 = dist_arr[0..];
    expect_eq(pairwiseEuclidean(pts4d[0..], 6, 4, &dist_s), true);

    let coords2d: [12]f64 = std.mem.zeroes([12]f64);
    let gram_arr: [36]f64 = std.mem.zeroes([36]f64);
    let rmean_arr: [6]f64 = std.mem.zeroes([6]f64);
    let evals_arr: [6]f64 = std.mem.zeroes([6]f64);
    let evecs_arr: [36]f64 = std.mem.zeroes([36]f64);
    let coords_s: []f64 = coords2d[0..];
    let gram_s: []f64 = gram_arr[0..];
    let rmean_s: []f64 = rmean_arr[0..];
    let evals_s: []f64 = evals_arr[0..];
    let evecs_s: []f64 = evecs_arr[0..];

    const res2 = classicalMds(dist_arr[0..], 6, 2, &coords_s, &gram_s, &rmean_s, &evals_s, &evecs_s, 80, 1.0e-12) ?? unreachable;
    expect_eq(res2.variance_explained > 0.99, true);
    expect_eq(res2.variance_explained < 1.0, true);
    expect_eq(res2.stress1 < 0.02, true);

    // Full 4D recovery achieves variance_explained == 1.0 and near-zero stress
    let coords4d: [24]f64 = std.mem.zeroes([24]f64);
    let coords4_s: []f64 = coords4d[0..];
    const res4 = classicalMds(dist_arr[0..], 6, 4, &coords4_s, &gram_s, &rmean_s, &evals_s, &evecs_s, 80, 1.0e-12) ?? unreachable;
    expect_eq(absF64(res4.variance_explained - 1.0) < 1.0e-11, true);
    expect_eq(res4.stress1 < 1.0e-11, true);
});

test("3. SMACOF refines non-Euclidean / noisy distance matrix and monotonically reduces Stress-1", () => {
    // 5-point non-Euclidean dissimilarity matrix (violates triangle inequality slightly / noisy)
    const noisy_dist: [25]f64 = [
        0.0, 1.0, 2.5, 3.0, 1.8,
        1.0, 0.0, 1.1, 2.2, 2.6,
        2.5, 1.1, 0.0, 1.0, 2.9,
        3.0, 2.2, 1.0, 0.0, 1.2,
        1.8, 2.6, 2.9, 1.2, 0.0,
    ];

    let coords_arr: [10]f64 = std.mem.zeroes([10]f64);
    let gram_arr: [25]f64 = std.mem.zeroes([25]f64);
    let rmean_arr: [5]f64 = std.mem.zeroes([5]f64);
    let evals_arr: [5]f64 = std.mem.zeroes([5]f64);
    let evecs_arr: [25]f64 = std.mem.zeroes([25]f64);
    let coords_s: []f64 = coords_arr[0..];
    let gram_s: []f64 = gram_arr[0..];
    let rmean_s: []f64 = rmean_arr[0..];
    let evals_s: []f64 = evals_arr[0..];
    let evecs_s: []f64 = evecs_arr[0..];

    const init_mds = classicalMds(noisy_dist[0..], 5, 2, &coords_s, &gram_s, &rmean_s, &evals_s, &evecs_s, 80, 1.0e-12) ?? unreachable;
    const init_stress = init_mds.stress1;

    let b_arr: [25]f64 = std.mem.zeroes([25]f64);
    let next_arr: [10]f64 = std.mem.zeroes([10]f64);
    let b_s: []f64 = b_arr[0..];
    let next_s: []f64 = next_arr[0..];

    // Step-by-step monotonicity check (1 iter -> 5 iters -> convergence)
    const s1 = smacofRefine(noisy_dist[0..], 5, 2, &coords_s, 1, 1.0e-12, &b_s, &next_s) ?? unreachable;
    expect_eq(s1.stress1 <= init_stress + 1.0e-12, true);

    const s5 = smacofRefine(noisy_dist[0..], 5, 2, &coords_s, 5, 1.0e-12, &b_s, &next_s) ?? unreachable;
    expect_eq(s5.stress1 <= s1.stress1 + 1.0e-12, true);

    const s_final = smacofRefine(noisy_dist[0..], 5, 2, &coords_s, 200, 1.0e-10, &b_s, &next_s) ?? unreachable;
    expect_eq(s_final.converged, true);
    expect_eq(s_final.stress1 < init_stress, true);
    const check_stress = kruskalStress1(noisy_dist[0..], coords_arr[0..], 5, 2) ?? unreachable;
    expect_eq(absF64(check_stress - s_final.stress1) < 1.0e-12, true);
});

test("4. doubleCenterGram enforces symmetry and zero row/column sum invariants", () => {
    const d: [16]f64 = [
        0.0, 1.2, 2.3, 1.7,
        1.2, 0.0, 1.5, 2.1,
        2.3, 1.5, 0.0, 1.4,
        1.7, 2.1, 1.4, 0.0,
    ];
    let gram: [16]f64 = std.mem.zeroes([16]f64);
    let rmean: [4]f64 = std.mem.zeroes([4]f64);
    let gram_s: []f64 = gram[0..];
    let rmean_s: []f64 = rmean[0..];

    expect_eq(doubleCenterGram(d[0..], 4, &gram_s, &rmean_s), true);
    let i: usize = 0;
    while (i < 4) {
        let rsum: f64 = 0.0;
        let csum: f64 = 0.0;
        let j: usize = 0;
        while (j < 4) {
            expect_eq(absF64(gram[i * 4 + j] - gram[j * 4 + i]) < 1.0e-12, true);
            rsum += gram[i * 4 + j];
            csum += gram[j * 4 + i];
            j += 1;
        }
        expect_eq(absF64(rsum) < 1.0e-12, true);
        expect_eq(absF64(csum) < 1.0e-12, true);
        i += 1;
    }
});

test("5. Defensive guards: asymmetric, negative, nonzero-diag, out_dims==0, short buffers, and NaN", () => {
    const asym: [4]f64 = [0.0, 1.0, 2.0, 0.0];
    const neg: [4]f64 = [0.0, -1.0, -1.0, 0.0];
    const bad_diag: [4]f64 = [0.1, 1.0, 1.0, 0.0];
    const zero_f: f64 = 0.0;
    const nan_f: f64 = zero_f / zero_f;
    const nan_d: [4]f64 = [0.0, nan_f, nan_f, 0.0];
    const ok_d: [4]f64 = [0.0, 1.0, 1.0, 0.0];

    let coords: [4]f64 = std.mem.zeroes([4]f64);
    let short_c: [1]f64 = std.mem.zeroes([1]f64);
    let gram: [4]f64 = std.mem.zeroes([4]f64);
    let rmean: [2]f64 = std.mem.zeroes([2]f64);
    let evals: [2]f64 = std.mem.zeroes([2]f64);
    let evecs: [4]f64 = std.mem.zeroes([4]f64);
    let coords_s: []f64 = coords[0..];
    let short_cs: []f64 = short_c[0..];
    let gram_s: []f64 = gram[0..];
    let rmean_s: []f64 = rmean[0..];
    let evals_s: []f64 = evals[0..];
    let evecs_s: []f64 = evecs[0..];

    expect_eq(doubleCenterGram(asym[0..], 2, &gram_s, &rmean_s), false);
    expect_eq(doubleCenterGram(neg[0..], 2, &gram_s, &rmean_s), false);
    expect_eq(doubleCenterGram(bad_diag[0..], 2, &gram_s, &rmean_s), false);
    expect_eq(doubleCenterGram(nan_d[0..], 2, &gram_s, &rmean_s), false);

    expect_eq(classicalMds(asym[0..], 2, 1, &coords_s, &gram_s, &rmean_s, &evals_s, &evecs_s, 20, 1.0e-10) == null, true);
    expect_eq(classicalMds(ok_d[0..], 2, 0, &coords_s, &gram_s, &rmean_s, &evals_s, &evecs_s, 20, 1.0e-10) == null, true);
    expect_eq(classicalMds(ok_d[0..], 2, 2, &short_cs, &gram_s, &rmean_s, &evals_s, &evecs_s, 20, 1.0e-10) == null, true);
    expect_eq(smacofRefine(neg[0..], 2, 1, &coords_s, 10, 1.0e-6, &gram_s, &evecs_s) == null, true);
    expect_eq(smacofRefine(ok_d[0..], 2, 0, &coords_s, 10, 1.0e-6, &gram_s, &evecs_s) == null, true);
    expect_eq(kruskalStress1(asym[0..], coords[0..], 2, 1) == null, true);
});
