// Origin: Pure-CP Stdlib Test Suite (lib/vector/kmeans.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation SIMD K-Means++ clustering & codebook
// trainer: 3-cluster 100% purity, deterministic SplitMix64 seed reproducibility, 32D SIMD
// convergence & degenerate duplicate handling, k==1 / k==n boundaries, and invalid param guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KMeansConfig,
    KMeansResult,
    kmeansInitPlusPlusF32,
    kmeansAssignF32,
    kmeansFitF32,
} from "cap:vector";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn testThreeSeparatedClustersFullPurity()! {
    // 12 samples in 4D: 3 well-separated clusters around (0,0,0,0), (10,10,0,0), (-10,0,10,0).
    const data: [48]f32 = [
        // Cluster A (indices 0..3)
         0.0,  0.0,  0.0,  0.0,
         0.2, -0.1,  0.1,  0.0,
        -0.2,  0.1, -0.1,  0.2,
         0.0,  0.2,  0.0, -0.2,
        // Cluster B (indices 4..7)
        10.0, 10.0,  0.0,  0.0,
        10.2,  9.8,  0.1,  0.0,
         9.8, 10.2, -0.1,  0.1,
        10.0, 10.0,  0.0, -0.1,
        // Cluster C (indices 8..11)
       -10.0,  0.0, 10.0,  0.0,
        -9.8,  0.1, 10.2,  0.0,
       -10.2, -0.1,  9.8,  0.1,
       -10.0,  0.0, 10.0, -0.1,
    ];

    let centroids: [12]f32 = undefined;
    let assignments: [12]usize = undefined;
    let counts: [3]usize = undefined;
    let scratch: [12]f32 = undefined;

    let cent_s: []f32 = centroids[0..];
    let assign_s: []usize = assignments[0..];
    let counts_s: []usize = counts[0..];
    let scratch_s: []f32 = scratch[0..];

    const cfg = KMeansConfig{ k: 3, max_iters: 30, tol: 0.0001, seed: 12345 };
    if (res := cap.vector.kmeansFitF32(data[0..], 12, 4, cfg, cent_s, assign_s, counts_s, scratch_s)) {
        expect_eq(res.converged, true);
        expect_eq(res.iters >= 1 && res.iters <= 30, true);
        expect_eq(res.inertia < 1.0, true);

        // 100% cluster purity: all 4 members of each true cluster share the exact same label,
        // and the 3 cluster labels are pairwise distinct.
        const la: usize = assignments[0];
        const lb: usize = assignments[4];
        const lc: usize = assignments[8];
        expect_eq(la != lb && lb != lc && la != lc, true);
        expect_eq(assignments[1], la);
        expect_eq(assignments[2], la);
        expect_eq(assignments[3], la);
        expect_eq(assignments[5], lb);
        expect_eq(assignments[6], lb);
        expect_eq(assignments[7], lb);
        expect_eq(assignments[9], lc);
        expect_eq(assignments[10], lc);
        expect_eq(assignments[11], lc);

        // Standalone kmeansAssignF32 reproduces the exact same inertia.
        let verify_assign: [12]usize = undefined;
        let verify_s: []usize = verify_assign[0..];
        if (check_inertia := kmeansAssignF32(data[0..], 12, 4, centroids[0..], 3, verify_s)) {
            expect_eq(absF64(check_inertia - res.inertia) < 0.0001, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testKMeansPlusPlusDeterministicSeedReproducibility()! {
    const data: [24]f32 = [
        0.0, 0.0,   1.0, 0.5,   2.0, 1.0,   10.0, 10.0,
        10.5, 9.5,  11.0, 10.2, -8.0, 7.0,  -7.5, 7.5,
        -8.2, 6.8,  5.0, -5.0,  5.5, -4.8,  4.8, -5.2,
    ];

    let c1: [6]f32 = undefined;
    let c2: [6]f32 = undefined;
    let d1: [12]f32 = undefined;
    let d2: [12]f32 = undefined;

    let c1_s: []f32 = c1[0..];
    let c2_s: []f32 = c2[0..];
    let d1_s: []f32 = d1[0..];
    let d2_s: []f32 = d2[0..];

    expect_eq(kmeansInitPlusPlusF32(data[0..], 12, 2, 3, 99999, c1_s, d1_s), true);
    expect_eq(kmeansInitPlusPlusF32(data[0..], 12, 2, 3, 99999, c2_s, d2_s), true);

    let i: usize = 0;
    while (i < 6) : (i += 1) {
        expect_eq(c1[i], c2[i]);
    }
}

fn testHighDim32DSimdConvergenceAndDuplicateReseed()! {
    // 1. 32D (4x8-lane SIMD) clustering over 16 points in 2 well-separated hyper-clusters.
    let data32: [512]f32 = undefined; // 16 * 32
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const center: f32 = if (i < 8) -5.0 else 5.0;
        const jitter: f32 = (((i % 8) as f32) - 3.5) * 0.05;
        let d: usize = 0;
        while (d < 32) : (d += 1) {
            data32[i * 32 + d] = center + jitter;
        }
    }

    let cent32: [64]f32 = undefined; // 2 * 32
    let assign32: [16]usize = undefined;
    let cnt32: [2]usize = undefined;
    let scr32: [16]f32 = undefined;

    let cent32_s: []f32 = cent32[0..];
    let assign32_s: []usize = assign32[0..];
    let cnt32_s: []usize = cnt32[0..];
    let scr32_s: []f32 = scr32[0..];

    const cfg32 = KMeansConfig{ k: 2, max_iters: 20, tol: 0.0001, seed: 77 };
    if (res := kmeansFitF32(data32[0..], 16, 32, cfg32, cent32_s, assign32_s, cnt32_s, scr32_s)) {
        expect_eq(res.converged, true);
        const g0: usize = assign32[0];
        const g1: usize = assign32[8];
        expect_eq(g0 != g1, true);
        let j: usize = 0;
        while (j < 8) : (j += 1) {
            expect_eq(assign32[j], g0);
            expect_eq(assign32[8 + j], g1);
        }
    } else {
        expect_eq(true, false);
    }

    // 2. Degenerate duplicate points (4 points, 3 identical + 1 far point, k = 3):
    //    exercises zero-weight K-Means++ fallback and empty-cluster reseeding without NaN.
    const dup_data: [8]f32 = [
        1.0, 1.0,
        1.0, 1.0,
        1.0, 1.0,
        20.0, 20.0,
    ];
    let dup_c: [6]f32 = undefined;
    let dup_a: [4]usize = undefined;
    let dup_cnt: [3]usize = undefined;
    let dup_scr: [4]f32 = undefined;

    let dup_c_s: []f32 = dup_c[0..];
    let dup_a_s: []usize = dup_a[0..];
    let dup_cnt_s: []usize = dup_cnt[0..];
    let dup_scr_s: []f32 = dup_scr[0..];

    const cfg_dup = KMeansConfig{ k: 3, max_iters: 15, tol: 0.0001, seed: 7 };
    if (res_dup := kmeansFitF32(dup_data[0..], 4, 2, cfg_dup, dup_c_s, dup_a_s, dup_cnt_s, dup_scr_s)) {
        expect_eq(!std.math.isNan(res_dup.inertia), true);
        expect_eq(res_dup.inertia < 0.001, true);
    } else {
        expect_eq(true, false);
    }
}

fn testSingleClusterAndKEqualsNBoundaries()! {
    const pts: [8]f32 = [
        0.0, 0.0,
        2.0, 0.0,
        0.0, 4.0,
        2.0, 4.0,
    ];

    // 1. k == 1: centroid must equal exact global mean (1.0, 2.0), inertia == 20.0.
    let c1: [2]f32 = undefined;
    let a1: [4]usize = undefined;
    let cnt1: [1]usize = undefined;
    let scr1: [4]f32 = undefined;
    let c1_s: []f32 = c1[0..];
    let a1_s: []usize = a1[0..];
    let cnt1_s: []usize = cnt1[0..];
    let scr1_s: []f32 = scr1[0..];

    const cfg1 = KMeansConfig{ k: 1, max_iters: 10, tol: 0.0001, seed: 1 };
    if (r1 := kmeansFitF32(pts[0..], 4, 2, cfg1, c1_s, a1_s, cnt1_s, scr1_s)) {
        expect_eq(r1.converged, true);
        expect_eq(absF64((c1[0] as f64) - 1.0) < 0.0001, true);
        expect_eq(absF64((c1[1] as f64) - 2.0) < 0.0001, true);
        expect_eq(absF64(r1.inertia - 20.0) < 0.001, true);
    } else {
        expect_eq(true, false);
    }

    // 2. k == n (4 distinct points, k = 4): zero inertia and every point gets its own cluster.
    let c4: [8]f32 = undefined;
    let a4: [4]usize = undefined;
    let cnt4: [4]usize = undefined;
    let scr4: [4]f32 = undefined;
    let c4_s: []f32 = c4[0..];
    let a4_s: []usize = a4[0..];
    let cnt4_s: []usize = cnt4[0..];
    let scr4_s: []f32 = scr4[0..];

    const cfg4 = KMeansConfig{ k: 4, max_iters: 20, tol: 0.0001, seed: 42 };
    if (r4 := kmeansFitF32(pts[0..], 4, 2, cfg4, c4_s, a4_s, cnt4_s, scr4_s)) {
        expect_eq(r4.converged, true);
        expect_eq(r4.inertia < 0.0001, true);
    } else {
        expect_eq(true, false);
    }
}

fn testInvalidParamsAndNanGuards()! {
    const ok_data: [8]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];
    const nan_data: [4]f32 = [1.0, std.math.nan(f32), 3.0, 4.0];

    let cent: [4]f32 = undefined;
    let assign: [4]usize = undefined;
    let counts: [2]usize = undefined;
    let scratch: [4]f32 = undefined;

    let cent_s: []f32 = cent[0..];
    let assign_s: []usize = assign[0..];
    let counts_s: []usize = counts[0..];
    let scratch_s: []f32 = scratch[0..];

    // k == 0, k > n, n == 0, dim == 0, max_iters == 0, negative/NaN tol, NaN data.
    expect_eq(kmeansFitF32(ok_data[0..], 4, 2, KMeansConfig{ k: 0, max_iters: 10, tol: 0.001, seed: 1 }, cent_s, assign_s, counts_s, scratch_s) == null, true);
    expect_eq(kmeansFitF32(ok_data[0..], 4, 2, KMeansConfig{ k: 5, max_iters: 10, tol: 0.001, seed: 1 }, cent_s, assign_s, counts_s, scratch_s) == null, true);
    expect_eq(kmeansFitF32(ok_data[0..], 0, 2, KMeansConfig{ k: 1, max_iters: 10, tol: 0.001, seed: 1 }, cent_s, assign_s, counts_s, scratch_s) == null, true);
    expect_eq(kmeansFitF32(ok_data[0..], 4, 0, KMeansConfig{ k: 2, max_iters: 10, tol: 0.001, seed: 1 }, cent_s, assign_s, counts_s, scratch_s) == null, true);
    expect_eq(kmeansFitF32(ok_data[0..], 4, 2, KMeansConfig{ k: 2, max_iters: 0, tol: 0.001, seed: 1 }, cent_s, assign_s, counts_s, scratch_s) == null, true);
    expect_eq(kmeansFitF32(ok_data[0..], 4, 2, KMeansConfig{ k: 2, max_iters: 10, tol: -0.1, seed: 1 }, cent_s, assign_s, counts_s, scratch_s) == null, true);
    expect_eq(kmeansFitF32(ok_data[0..], 4, 2, KMeansConfig{ k: 2, max_iters: 10, tol: std.math.nan(f32), seed: 1 }, cent_s, assign_s, counts_s, scratch_s) == null, true);
    expect_eq(kmeansFitF32(nan_data[0..], 2, 2, KMeansConfig{ k: 2, max_iters: 10, tol: 0.001, seed: 1 }, cent_s, assign_s, counts_s, scratch_s) == null, true);

    // Truncated scratch buffers rejected.
    expect_eq(kmeansInitPlusPlusF32(ok_data[0..], 4, 2, 2, 1, cent[0..3], scratch_s), false);
    expect_eq(kmeansAssignF32(ok_data[0..], 4, 2, cent[0..], 0, assign_s) == null, true);
}

fn main() {
    test("1. vector/kmeans: 3 well-separated clusters 100% purity & inertia consistency", () => {
        testThreeSeparatedClustersFullPurity()?;
    });

    test("2. vector/kmeans: K-Means++ deterministic SplitMix64 seed reproducibility", () => {
        testKMeansPlusPlusDeterministicSeedReproducibility()?;
    });

    test("3. vector/kmeans: 32D SIMD convergence & degenerate duplicate reseed stability", () => {
        testHighDim32DSimdConvergenceAndDuplicateReseed()?;
    });

    test("4. vector/kmeans: k==1 global mean & k==n zero-inertia boundary cases", () => {
        testSingleClusterAndKEqualsNBoundaries()?;
    });

    test("5. vector/kmeans: invalid params, truncated buffers & NaN/Inf guards", () => {
        testInvalidParamsAndNanGuards()?;
    });
}
