// Source: cp/cap/test/test_markov_production_deep.cp (Production deep tests for cap:markov)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    isStochasticMatrix,
    stepDistribution,
    transitionPower,
    StationaryResult,
    stationaryPowerIter,
    absorbingFundamental,
    expectedAbsorptionSteps,
    absorptionProbabilities,
    sampleTrajectory,
    entropyRate,
} from "cap:math";

fn absDiff(a: f64, b: f64): f64 {
    const d = a - b;
    return if (d < 0.0) 0.0 - d else d;
}

test("1. 3-state weather chain: transitionPower binary exponentiation matches iterative stepDistribution", () => {
    const p: [9]f64 = [
        0.7, 0.2, 0.1,
        0.3, 0.4, 0.3,
        0.2, 0.3, 0.5,
    ];
    expect_eq(isStochasticMatrix(p[0..], 3, 1.0e-12), true);

    let out: [9]f64 = std.mem.zeroes([9]f64);
    let base: [9]f64 = std.mem.zeroes([9]f64);
    let tmp: [9]f64 = std.mem.zeroes([9]f64);

    // P^0 == I_3
    expect_eq(transitionPower(p[0..], 3, 0, out[0..], base[0..], tmp[0..]), true);
    expect_eq(out[0], 1.0);
    expect_eq(out[4], 1.0);
    expect_eq(out[8], 1.0);
    expect_eq(out[1], 0.0);

    // P^13 via fast exponentiation vs 13 sequential stepDistribution calls
    expect_eq(transitionPower(p[0..], 3, 13, out[0..], base[0..], tmp[0..]), true);
    expect_eq(isStochasticMatrix(out[0..], 3, 1.0e-12), true);

    let dist: [3]f64 = [1.0, 0.0, 0.0];
    let next: [3]f64 = std.mem.zeroes([3]f64);
    let s: usize = 0;
    while (s < 13) {
        expect_eq(stepDistribution(dist[0..], p[0..], 3, next[0..]), true);
        dist[0] = next[0];
        dist[1] = next[1];
        dist[2] = next[2];
        s += 1;
    }

    // Row 0 of P^13 must equal [1, 0, 0] * P^13
    expect_eq(absDiff(out[0], dist[0]) < 1.0e-12, true);
    expect_eq(absDiff(out[1], dist[1]) < 1.0e-12, true);
    expect_eq(absDiff(out[2], dist[2]) < 1.0e-12, true);
});

test("2. Stationary distribution pi * P == pi matches exact analytical solution (21/46, 13/46, 12/46)", () => {
    const p: [9]f64 = [
        0.7, 0.2, 0.1,
        0.3, 0.4, 0.3,
        0.2, 0.3, 0.5,
    ];
    let pi: [3]f64 = std.mem.zeroes([3]f64);
    let tmp: [3]f64 = std.mem.zeroes([3]f64);

    const res = stationaryPowerIter(p[0..], 3, pi[0..], tmp[0..], 500, 1.0e-14) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.residual_l1 <= 1.0e-14, true);

    expect_eq(absDiff(pi[0] + pi[1] + pi[2], 1.0) < 1.0e-12, true);
    expect_eq(absDiff(pi[0], 21.0 / 46.0) < 1.0e-12, true);
    expect_eq(absDiff(pi[1], 13.0 / 46.0) < 1.0e-12, true);
    expect_eq(absDiff(pi[2], 12.0 / 46.0) < 1.0e-12, true);

    // Verify pi * P == pi
    expect_eq(stepDistribution(pi[0..], p[0..], 3, tmp[0..]), true);
    expect_eq(absDiff(tmp[0], pi[0]) < 1.0e-12, true);
    expect_eq(absDiff(tmp[1], pi[1]) < 1.0e-12, true);
    expect_eq(absDiff(tmp[2], pi[2]) < 1.0e-12, true);
});

test("3. Classic 5-state fair gambler's ruin absorbing chain: exact N = (I-Q)^-1, expected steps & absorption probs", () => {
    // States: absorbing {0, 4}, transient {1, 2, 3}, fair coin p = 0.5 left/right
    const q: [9]f64 = [
        0.0, 0.5, 0.0,
        0.5, 0.0, 0.5,
        0.0, 0.5, 0.0,
    ];
    const r: [6]f64 = [
        0.5, 0.0,
        0.0, 0.0,
        0.0, 0.5,
    ];

    let fund: [9]f64 = std.mem.zeroes([9]f64);
    let lu_s: [9]f64 = std.mem.zeroes([9]f64);
    let piv: [3]usize = std.mem.zeroes([3]usize);
    let rhs_s: [3]f64 = std.mem.zeroes([3]f64);
    let col_s: [3]f64 = std.mem.zeroes([3]f64);

    expect_eq(absorbingFundamental(q[0..], 3, fund[0..], lu_s[0..], piv[0..], rhs_s[0..], col_s[0..]), true);

    // Expected steps from transient states {1, 2, 3}: i*(4-i) = [3, 4, 3]
    let steps: [3]f64 = std.mem.zeroes([3]f64);
    expect_eq(expectedAbsorptionSteps(fund[0..], 3, steps[0..]), true);
    expect_eq(absDiff(steps[0], 3.0) < 1.0e-12, true);
    expect_eq(absDiff(steps[1], 4.0) < 1.0e-12, true);
    expect_eq(absDiff(steps[2], 3.0) < 1.0e-12, true);

    // Absorption probabilities B = N * R: [[0.75, 0.25], [0.5, 0.5], [0.25, 0.75]]
    let probs: [6]f64 = std.mem.zeroes([6]f64);
    expect_eq(absorptionProbabilities(fund[0..], r[0..], 3, 2, probs[0..]), true);
    expect_eq(absDiff(probs[0], 0.75) < 1.0e-12, true);
    expect_eq(absDiff(probs[1], 0.25) < 1.0e-12, true);
    expect_eq(absDiff(probs[2], 0.50) < 1.0e-12, true);
    expect_eq(absDiff(probs[3], 0.50) < 1.0e-12, true);
    expect_eq(absDiff(probs[4], 0.25) < 1.0e-12, true);
    expect_eq(absDiff(probs[5], 0.75) < 1.0e-12, true);
});

test("4. Exact entropyRate for symmetric fair coin, 4-state uniform walk & deterministic cycle", () => {
    // 2-state fair coin -> H = 1.0 bit
    const p2: [4]f64 = [0.5, 0.5, 0.5, 0.5];
    const pi2: [2]f64 = [0.5, 0.5];
    const h2 = entropyRate(p2[0..], pi2[0..], 2) ?? unreachable;
    expect_eq(absDiff(h2, 1.0) < 1.0e-12, true);

    // 4-state uniform walk -> H = log2(4) = 2.0 bits
    const p4: [16]f64 = [
        0.25, 0.25, 0.25, 0.25,
        0.25, 0.25, 0.25, 0.25,
        0.25, 0.25, 0.25, 0.25,
        0.25, 0.25, 0.25, 0.25,
    ];
    const pi4: [4]f64 = [0.25, 0.25, 0.25, 0.25];
    const h4 = entropyRate(p4[0..], pi4[0..], 4) ?? unreachable;
    expect_eq(absDiff(h4, 2.0) < 1.0e-12, true);

    // 3-state deterministic cycle 0->1->2->0 -> H = 0.0 bits
    const p_det: [9]f64 = [
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
        1.0, 0.0, 0.0,
    ];
    const pi3: [3]f64 = [1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0];
    const h_det = entropyRate(p_det[0..], pi3[0..], 3) ?? unreachable;
    expect_eq(absDiff(h_det, 0.0) < 1.0e-12, true);
});

test("5. Deterministic sampleTrajectory reproducibility & long-run empirical frequency convergence", () => {
    const p: [9]f64 = [
        0.7, 0.2, 0.1,
        0.3, 0.4, 0.3,
        0.2, 0.3, 0.5,
    ];
    let path1: [10000]usize = std.mem.zeroes([10000]usize);
    let path2: [10000]usize = std.mem.zeroes([10000]usize);

    expect_eq(sampleTrajectory(p[0..], 3, 0, 202601, path1[0..]), true);
    expect_eq(sampleTrajectory(p[0..], 3, 0, 202601, path2[0..]), true);
    expect_eq(path1[0], 0 as usize);

    let counts: [3]usize = [0, 0, 0];
    let i: usize = 0;
    while (i < 10000) {
        expect_eq(path1[i], path2[i]); // 100% deterministic
        expect_eq(path1[i] < 3, true);
        counts[path1[i]] += 1;
        i += 1;
    }

    const f0 = (counts[0] as f64) / 10000.0;
    const f1 = (counts[1] as f64) / 10000.0;
    const f2 = (counts[2] as f64) / 10000.0;
    expect_eq(absDiff(f0, 21.0 / 46.0) < 0.025, true);
    expect_eq(absDiff(f1, 13.0 / 46.0) < 0.025, true);
    expect_eq(absDiff(f2, 12.0 / 46.0) < 0.025, true);
});

test("6. Boundary guards: non-stochastic matrices, singular (I-Q), OOB start_state, bad pi & NaN", () => {
    const bad_neg: [4]f64 = [-0.1, 1.1, 0.5, 0.5];
    const bad_sum: [4]f64 = [0.6, 0.6, 0.5, 0.5];
    const bad_nan: [4]f64 = [std.math.nan(f64), 0.5, 0.5, 0.5];
    const ok_p: [4]f64 = [0.5, 0.5, 0.5, 0.5];

    expect_eq(isStochasticMatrix(bad_neg[0..], 2, 1.0e-9), false);
    expect_eq(isStochasticMatrix(bad_sum[0..], 2, 1.0e-9), false);
    expect_eq(isStochasticMatrix(bad_nan[0..], 2, 1.0e-9), false);
    expect_eq(isStochasticMatrix(ok_p[0..], 0, 1.0e-9), false);

    let pi2: [2]f64 = std.mem.zeroes([2]f64);
    let tmp2: [2]f64 = std.mem.zeroes([2]f64);
    let small1: [1]f64 = std.mem.zeroes([1]f64);
    expect_eq(stationaryPowerIter(bad_sum[0..], 2, pi2[0..], tmp2[0..], 100, 1.0e-9) == null, true);
    expect_eq(stationaryPowerIter(ok_p[0..], 2, small1[0..], tmp2[0..], 100, 1.0e-9) == null, true);
    expect_eq(stationaryPowerIter(ok_p[0..], 2, pi2[0..], tmp2[0..], 100, 0.0) == null, true);

    // Closed transient class Q = [[0, 1], [1, 0]] => (I - Q) is singular -> rejected
    const q_sing: [4]f64 = [0.0, 1.0, 1.0, 0.0];
    let fund: [4]f64 = std.mem.zeroes([4]f64);
    let lu_s: [4]f64 = std.mem.zeroes([4]f64);
    let piv: [2]usize = std.mem.zeroes([2]usize);
    expect_eq(absorbingFundamental(q_sing[0..], 2, fund[0..], lu_s[0..], piv[0..], pi2[0..], tmp2[0..]), false);

    // OOB start_state rejected
    let path: [8]usize = std.mem.zeroes([8]usize);
    expect_eq(sampleTrajectory(ok_p[0..], 2, 2, 1, path[0..]), false);
    expect_eq(sampleTrajectory(bad_sum[0..], 2, 0, 1, path[0..]), false);

    // Invalid pi in entropyRate rejected
    const bad_pi: [2]f64 = [0.8, 0.8];
    expect_eq(entropyRate(ok_p[0..], bad_pi[0..], 2) == null, true);
    expect_eq(entropyRate(bad_neg[0..], pi2[0..], 2) == null, true);
});
