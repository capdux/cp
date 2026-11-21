// Source: cp/cap/test/test_dtw_production_deep.cp (Production deep tests for cap:dtw)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    WarpPair,
    dtwDistance1D,
    dtwAlign1D,
    dtwDistanceMulti,
    computeKeoghEnvelope,
    lbKeogh,
    zNormalize,
} from "cap:dtw";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn euclidean1D(a: []const f64, b: []const f64): f64 {
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) {
        const d = a[i] - b[i];
        sum += d * d;
        i += 1;
    }
    return @sqrt(sum);
}

test("1. Identical & time-stretched sequences, symmetry & Sakoe-Chiba band monotonicity", () => {
    const a = [0.0, 1.0, 2.0, 1.0, 0.0];
    const a_stretched = [0.0, 0.0, 1.0, 1.0, 2.0, 2.0, 1.0, 0.0];
    let prev: [12]f64 = std.mem.zeroes([12]f64);
    let curr: [12]f64 = std.mem.zeroes([12]f64);
    let p_s: []f64 = prev[0..];
    let c_s: []f64 = curr[0..];

    // Identical and time-stretched plateau sequences both have exact 0.0 DTW distance
    const d_same = dtwDistance1D(a[0..], a[0..], 0, &p_s, &c_s) ?? unreachable;
    expect_eq(absF64(d_same) < 1.0e-12, true);

    const d_stretch = dtwDistance1D(a[0..], a_stretched[0..], 0, &p_s, &c_s) ?? unreachable;
    expect_eq(absF64(d_stretch) < 1.0e-12, true);

    // Phase-shifted equal-length series: verify symmetry & monotonicity dtw(inf) <= dtw(w=2) <= dtw(w=1) <= euclidean
    const s1 = [0.0, 0.5, 2.0, 4.0, 2.0, 0.5, 0.0, 0.0];
    const s2 = [0.0, 0.0, 0.5, 2.0, 4.0, 2.0, 0.5, 0.0];

    const d_inf = dtwDistance1D(s1[0..], s2[0..], 0, &p_s, &c_s) ?? unreachable;
    const d_rev = dtwDistance1D(s2[0..], s1[0..], 0, &p_s, &c_s) ?? unreachable;
    const d_w2 = dtwDistance1D(s1[0..], s2[0..], 2, &p_s, &c_s) ?? unreachable;
    const d_w1 = dtwDistance1D(s1[0..], s2[0..], 1, &p_s, &c_s) ?? unreachable;
    const d_euc = euclidean1D(s1[0..], s2[0..]);

    expect_eq(absF64(d_inf - d_rev) < 1.0e-12, true);
    expect_eq(absF64(d_inf) < 1.0e-12, true);
    expect_eq(d_inf <= d_w2 + 1.0e-12, true);
    expect_eq(d_w2 <= d_w1 + 1.0e-12, true);
    expect_eq(d_w1 <= d_euc + 1.0e-12, true);
    expect_eq(d_euc > 2.0, true);
});

test("2. dtwAlign1D optimal path monotonicity, continuity & exact accumulated cost equality", () => {
    const a = [1.0, 2.0, 5.0, 3.0, 1.0, 0.0];
    const b = [1.0, 1.5, 2.0, 5.0, 4.5, 3.0, 1.0];

    let dp: [64]f64 = std.mem.zeroes([64]f64);
    let path: [16]WarpPair = std.mem.zeroes([16]WarpPair);
    let dp_s: []f64 = dp[0..];
    let path_s: []WarpPair = path[0..];

    const res = dtwAlign1D(a[0..], b[0..], 0, &dp_s, &path_s) ?? unreachable;
    expect_eq(res.path_len >= 7, true);
    expect_eq(path[0].i, 0 as usize);
    expect_eq(path[0].j, 0 as usize);
    expect_eq(path[res.path_len - 1].i, 5 as usize);
    expect_eq(path[res.path_len - 1].j, 6 as usize);

    // Verify step monotonicity & exact path cost sum
    let sum_sq: f64 = 0.0;
    let k: usize = 0;
    while (k < res.path_len) {
        const pi = path[k].i;
        const pj = path[k].j;
        const d = a[pi] - b[pj];
        sum_sq += d * d;
        if (k > 0) {
            const di = pi - path[k - 1].i;
            const dj = pj - path[k - 1].j;
            expect_eq(di <= 1 && dj <= 1 && (di + dj >= 1), true);
        }
        k += 1;
    }
    expect_eq(absF64(@sqrt(sum_sq) - res.distance) < 1.0e-12, true);

    // Must match rolling 2-row dtwDistance1D exactly
    let prev: [10]f64 = std.mem.zeroes([10]f64);
    let curr: [10]f64 = std.mem.zeroes([10]f64);
    let p_s: []f64 = prev[0..];
    let c_s: []f64 = curr[0..];
    const rolling = dtwDistance1D(a[0..], b[0..], 0, &p_s, &c_s) ?? unreachable;
    expect_eq(absF64(rolling - res.distance) < 1.0e-12, true);
});

test("3. Multi-dimensional 3D trajectory alignment via dtwDistanceMulti", () => {
    // 4-point 3D trajectory vs time-warped 6-point identical 3D path
    const traj_a = [
        0.0, 0.0, 0.0,
        1.0, 2.0, 0.0,
        2.0, 2.0, 1.0,
        3.0, 0.0, 1.0,
    ];
    const traj_b = [
        0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        1.0, 2.0, 0.0,
        2.0, 2.0, 1.0,
        2.0, 2.0, 1.0,
        3.0, 0.0, 1.0,
    ];

    let prev: [8]f64 = std.mem.zeroes([8]f64);
    let curr: [8]f64 = std.mem.zeroes([8]f64);
    let p_s: []f64 = prev[0..];
    let c_s: []f64 = curr[0..];

    const d_zero = dtwDistanceMulti(traj_a[0..], 4, traj_b[0..], 6, 3, 0, &p_s, &c_s) ?? unreachable;
    expect_eq(absF64(d_zero) < 1.0e-12, true);

    // Shift traj_a by +1.0 along Z on all 4 frames -> squared frame cost = 1.0 * 4 = 4.0 -> dist = 2.0
    const traj_shifted = [
        0.0, 0.0, 1.0,
        1.0, 2.0, 1.0,
        2.0, 2.0, 2.0,
        3.0, 0.0, 2.0,
    ];
    const d_shift = dtwDistanceMulti(traj_a[0..], 4, traj_shifted[0..], 4, 3, 1, &p_s, &c_s) ?? unreachable;
    expect_eq(absF64(d_shift - 2.0) < 1.0e-12, true);
});

test("4. LB_Keogh strict lower bound theorem: lbKeogh(q, env(c, w)) <= dtwDistance1D(q, c, w) across 12 candidates", () => {
    const n: usize = 12;
    let query: [12]f64 = std.mem.zeroes([12]f64);
    let i: usize = 0;
    while (i < n) {
        query[i] = @sin((i as f64) * 0.5) * 2.0;
        i += 1;
    }

    let upper: [12]f64 = std.mem.zeroes([12]f64);
    let lower: [12]f64 = std.mem.zeroes([12]f64);
    let prev: [13]f64 = std.mem.zeroes([13]f64);
    let curr: [13]f64 = std.mem.zeroes([13]f64);
    let u_s: []f64 = upper[0..];
    let l_s: []f64 = lower[0..];
    let p_s: []f64 = prev[0..];
    let c_s: []f64 = curr[0..];

    let cand: [12]f64 = std.mem.zeroes([12]f64);
    let c_idx: usize = 0;
    while (c_idx < 12) {
        const shift = ((c_idx as f64) - 5.0) * 0.35;
        const freq = 0.3 + (c_idx as f64) * 0.07;
        i = 0;
        while (i < n) {
            cand[i] = @cos((i as f64) * freq) * 1.8 + shift;
            i += 1;
        }

        const windows: [4]usize = [1, 2, 3, 0];
        let wi: usize = 0;
        while (wi < 4) {
            const w = windows[wi];
            expect_eq(computeKeoghEnvelope(cand[0..], w, &u_s, &l_s), true);
            const lb = lbKeogh(query[0..], upper[0..], lower[0..]) ?? unreachable;
            const dtw = dtwDistance1D(query[0..], cand[0..], w, &p_s, &c_s) ?? unreachable;
            expect_eq(lb >= 0.0, true);
            expect_eq(lb <= dtw + 1.0e-11, true);
            wi += 1;
        }
        c_idx += 1;
    }
});

test("5. zNormalize yields exact zero mean, unit standard deviation & handles constant series", () => {
    const seq = [10.0, 20.0, 30.0, 40.0, 50.0];
    let out: [5]f64 = std.mem.zeroes([5]f64);
    let out_s: []f64 = out[0..];
    expect_eq(zNormalize(seq[0..], &out_s), true);

    let sum: f64 = 0.0;
    let sq_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < 5) {
        sum += out[i];
        sq_sum += out[i] * out[i];
        i += 1;
    }
    expect_eq(absF64(sum / 5.0) < 1.0e-12, true);
    expect_eq(absF64(@sqrt(sq_sum / 5.0) - 1.0) < 1.0e-12, true);

    // Constant series maps cleanly to all zeros
    const flat = [7.5, 7.5, 7.5, 7.5, 7.5];
    expect_eq(zNormalize(flat[0..], &out_s), true);
    i = 0;
    while (i < 5) {
        expect_eq(out[i], 0.0);
        i += 1;
    }
});

test("6. Defensive validation: empty series, undersized buffers, inverted envelope & NaN rejection", () => {
    const empty: [0]f64 = [];
    const ok = [1.0, 2.0, 3.0];
    let prev: [4]f64 = std.mem.zeroes([4]f64);
    let curr: [4]f64 = std.mem.zeroes([4]f64);
    let p_s: []f64 = prev[0..];
    let c_s: []f64 = curr[0..];
    let short_s: []f64 = prev[0..3];

    expect_eq(dtwDistance1D(empty[0..], ok[0..], 0, &p_s, &c_s) == null, true);
    expect_eq(dtwDistance1D(ok[0..], ok[0..], 0, &short_s, &c_s) == null, true);

    const zero: f64 = 0.0;
    const with_nan = [1.0, zero / zero, 3.0];
    expect_eq(dtwDistance1D(with_nan[0..], ok[0..], 0, &p_s, &c_s) == null, true);
    expect_eq(computeKeoghEnvelope(with_nan[0..], 1, &p_s, &c_s), false);
    expect_eq(zNormalize(with_nan[0..], &p_s), false);

    // Inverted envelope (lower > upper) rejected by lbKeogh
    const bad_upper = [0.0, 0.0, 0.0];
    const bad_lower = [1.0, 1.0, 1.0];
    expect_eq(lbKeogh(ok[0..], bad_upper[0..], bad_lower[0..]) == null, true);

    // Undersized path buffer rejected by dtwAlign1D
    let dp: [16]f64 = std.mem.zeroes([16]f64);
    let tiny_path: [2]WarpPair = std.mem.zeroes([2]WarpPair);
    let dp_s: []f64 = dp[0..];
    let tp_s: []WarpPair = tiny_path[0..];
    expect_eq(dtwAlign1D(ok[0..], ok[0..], 0, &dp_s, &tp_s) == null, true);
});
