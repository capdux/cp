// Source: cp/cap/test/test_anneal_production_deep.cp (Production deep tests for cap:anneal)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CoolingSchedule,
    AnnealConfig,
    temperatureAt,
    metropolisAccept,
    annealContinuous,
    annealPermutation,
} from "cap:anneal";

const PI: f64 = 3.14159265358979323846;

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn sphereObj(x: []const f64): f64 {
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < x.len) {
        sum += x[i] * x[i];
        i += 1;
    }
    return sum;
}

fn rastrigin2dObj(x: []const f64): f64 {
    return 20.0 +
        (x[0] * x[0] - 10.0 * std.math.cos(2.0 * PI * x[0])) +
        (x[1] * x[1] - 10.0 * std.math.cos(2.0 * PI * x[1]));
}

fn shiftedBoxObj(x: []const f64): f64 {
    const dx = x[0] - 10.0;
    const dy = x[1] + 10.0;
    return dx * dx + dy * dy;
}

/// 12 cities evenly spaced on a circle of radius 10.0.
/// Exact optimal tour perimeter = 12 * 2 * 10 * sin(pi / 12).
fn circleTsp12Energy(perm: []const usize): f64 {
    const n = perm.len;
    const radius: f64 = 10.0;
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const a = perm[i];
        const b = perm[(i + 1) % n];
        const ang_a = 2.0 * PI * (a as f64) / (n as f64);
        const ang_b = 2.0 * PI * (b as f64) / (n as f64);
        const dx = radius * (std.math.cos(ang_a) - std.math.cos(ang_b));
        const dy = radius * (std.math.sin(ang_a) - std.math.sin(ang_b));
        total += std.math.sqrt(dx * dx + dy * dy);
        i += 1;
    }
    return total;
}

fn nanRealObj(x: []const f64): f64 {
    const z: f64 = 0.0;
    return (x[0] * z) / z;
}

test("1. All 4 cooling schedules decrease monotonically and metropolisAccept obeys Boltzmann physics", () => {
    const schedules = [
        CoolingSchedule.Geometric,
        CoolingSchedule.Linear,
        CoolingSchedule.Fast,
        CoolingSchedule.Logarithmic,
    ];
    let s_idx: usize = 0;
    while (s_idx < 4) {
        const sch = schedules[s_idx];
        const t0 = temperatureAt(100.0, 0.01, 0.99, 0, 1000, sch);
        const t1 = temperatureAt(100.0, 0.01, 0.99, 10, 1000, sch);
        const t2 = temperatureAt(100.0, 0.01, 0.99, 100, 1000, sch);
        const t3 = temperatureAt(100.0, 0.01, 0.99, 1000, 1000, sch);
        expect_eq(absF64(t0 - 100.0) < 1.0e-12, true);
        expect_eq(t0 > t1 && t1 > t2 && t2 >= t3 && t3 >= 0.01, true);
        s_idx += 1;
    }

    // Metropolis physics:
    expect_eq(metropolisAccept(-1.0, 0.0, 0.999), true);
    expect_eq(metropolisAccept(0.0, 0.0, 0.999), true);
    expect_eq(metropolisAccept(0.1, 0.0, 0.0), false);
    // exp(-1 / 1) = 0.367879...
    expect_eq(metropolisAccept(1.0, 1.0, 0.36), true);
    expect_eq(metropolisAccept(1.0, 1.0, 0.38), false);
    // Higher temperature accepts larger uphill moves
    expect_eq(metropolisAccept(1.0, 10.0, 0.85), true);
    expect_eq(metropolisAccept(1.0, 0.1, 0.01), false);
});

test("2. 2D Rastrigin: annealContinuous escapes local minimum (3, -3) to global minimum (0, 0)", () => {
    const x0 = [3.0, -3.0];
    const lower = [-5.12, -5.12];
    const upper = [5.12, 5.12];
    let best_x: [2]f64 = std.mem.zeroes([2]f64);
    let curr: [2]f64 = std.mem.zeroes([2]f64);
    let cand: [2]f64 = std.mem.zeroes([2]f64);
    let best_s: []f64 = best_x[0..];
    let curr_s: []f64 = curr[0..];
    let cand_s: []f64 = cand[0..];

    let cfg = AnnealConfig.default();
    cfg.t_init = 25.0;
    cfg.t_min = 1.0e-7;
    cfg.cooling_rate = 0.996;
    cfg.step_scale = 0.3;
    cfg.max_steps = 8000;
    cfg.seed = 2026;

    const res = annealContinuous(rastrigin2dObj, x0[0..], lower[0..], upper[0..], cfg, &best_s, &curr_s, &cand_s) ?? unreachable;
    expect_eq(res.best_energy < 1.0e-4, true);
    expect_eq(absF64(best_x[0]) < 1.0e-2, true);
    expect_eq(absF64(best_x[1]) < 1.0e-2, true);
    expect_eq(res.accepted > 0, true);
});

test("3. 4D Sphere convergence & strict box constraints enforcement", () => {
    const x0 = [3.0, -2.5, 1.8, -3.2];
    const lower = [-4.0, -4.0, -4.0, -4.0];
    const upper = [4.0, 4.0, 4.0, 4.0];
    let best_x: [4]f64 = std.mem.zeroes([4]f64);
    let curr: [4]f64 = std.mem.zeroes([4]f64);
    let cand: [4]f64 = std.mem.zeroes([4]f64);
    let best_s: []f64 = best_x[0..];
    let curr_s: []f64 = curr[0..];
    let cand_s: []f64 = cand[0..];

    let cfg = AnnealConfig.default();
    cfg.t_init = 5.0;
    cfg.t_min = 1.0e-7;
    cfg.cooling_rate = 0.996;
    cfg.step_scale = 0.25;
    cfg.max_steps = 8000;
    cfg.seed = 777;

    const res = annealContinuous(sphereObj, x0[0..], lower[0..], upper[0..], cfg, &best_s, &curr_s, &cand_s) ?? unreachable;
    expect_eq(res.best_energy < 1.0e-4, true);
    let j: usize = 0;
    while (j < 4) {
        expect_eq(best_x[j] >= lower[j] && best_x[j] <= upper[j], true);
        j += 1;
    }

    // Optimum outside box [1, 3] x [-4, -2] must clamp strictly to corner (3, -4)
    const bx0 = [2.0, -3.0];
    const blo = [1.0, -4.0];
    const bhi = [3.0, -2.0];
    let b_best: [2]f64 = std.mem.zeroes([2]f64);
    let b_curr: [2]f64 = std.mem.zeroes([2]f64);
    let b_cand: [2]f64 = std.mem.zeroes([2]f64);
    let b_best_s: []f64 = b_best[0..];
    let b_curr_s: []f64 = b_curr[0..];
    let b_cand_s: []f64 = b_cand[0..];

    const b_res = annealContinuous(shiftedBoxObj, bx0[0..], blo[0..], bhi[0..], cfg, &b_best_s, &b_curr_s, &b_cand_s) ?? unreachable;
    expect_eq(b_best[0] >= blo[0] && b_best[0] <= bhi[0], true);
    expect_eq(b_best[1] >= blo[1] && b_best[1] <= bhi[1], true);
    expect_eq(absF64(b_res.best_energy - 85.0) < 1.0e-2, true);
});

test("4. 12-city Circle TSP permutation annealing finds exact shortest perimeter within 5000 steps", () => {
    const init_perm: [12]usize = [0, 6, 3, 9, 1, 7, 4, 10, 2, 8, 5, 11];
    let best_p: [12]usize = std.mem.zeroes([12]usize);
    let curr_p: [12]usize = std.mem.zeroes([12]usize);
    let cand_p: [12]usize = std.mem.zeroes([12]usize);
    let best_s: []usize = best_p[0..];
    let curr_s: []usize = curr_p[0..];
    let cand_s: []usize = cand_p[0..];

    let cfg = AnnealConfig.default();
    cfg.t_init = 20.0;
    cfg.t_min = 1.0e-4;
    cfg.cooling_rate = 0.997;
    cfg.max_steps = 5000;
    cfg.seed = 1234;

    const init_len = circleTsp12Energy(init_perm[0..]);
    const exact_opt = 12.0 * 2.0 * 10.0 * std.math.sin(PI / 12.0);
    expect_eq(init_len > exact_opt * 2.0, true);

    const res = annealPermutation(circleTsp12Energy, init_perm[0..], cfg, &best_s, &curr_s, &cand_s) ?? unreachable;
    expect_eq(absF64(res.best_energy - exact_opt) < 1.0e-6, true);

    // Verify best_p is still a valid permutation of 0..11
    let seen: [12]u8 = std.mem.zeroes([12]u8);
    let i: usize = 0;
    while (i < 12) {
        expect_eq(best_p[i] < 12, true);
        expect_eq(seen[best_p[i]], 0);
        seen[best_p[i]] = 1;
        i += 1;
    }
});

test("5. Bit-exact determinism with identical seed for continuous & permutation SA", () => {
    const x0 = [2.0, -2.0];
    const lo = [-5.0, -5.0];
    const hi = [5.0, 5.0];
    let cfg = AnnealConfig.default();
    cfg.max_steps = 1000;
    cfg.seed = 0xFACEB00C55AA;

    let bx1: [2]f64 = std.mem.zeroes([2]f64);
    let bx2: [2]f64 = std.mem.zeroes([2]f64);
    let c1: [2]f64 = std.mem.zeroes([2]f64);
    let d1: [2]f64 = std.mem.zeroes([2]f64);
    let bx1_s: []f64 = bx1[0..];
    let bx2_s: []f64 = bx2[0..];
    let c1_s: []f64 = c1[0..];
    let d1_s: []f64 = d1[0..];

    const r1 = annealContinuous(rastrigin2dObj, x0[0..], lo[0..], hi[0..], cfg, &bx1_s, &c1_s, &d1_s) ?? unreachable;
    const r2 = annealContinuous(rastrigin2dObj, x0[0..], lo[0..], hi[0..], cfg, &bx2_s, &c1_s, &d1_s) ?? unreachable;
    expect_eq(r1.best_energy == r2.best_energy, true);
    expect_eq(r1.final_energy == r2.final_energy, true);
    expect_eq(r1.accepted, r2.accepted);
    expect_eq(bx1[0] == bx2[0] && bx1[1] == bx2[1], true);

    const init_p: [6]usize = [0, 3, 1, 4, 2, 5];
    let bp1: [6]usize = std.mem.zeroes([6]usize);
    let bp2: [6]usize = std.mem.zeroes([6]usize);
    let cp: [6]usize = std.mem.zeroes([6]usize);
    let dp: [6]usize = std.mem.zeroes([6]usize);
    let bp1_s: []usize = bp1[0..];
    let bp2_s: []usize = bp2[0..];
    let cp_s: []usize = cp[0..];
    let dp_s: []usize = dp[0..];

    const p1 = annealPermutation(circleTsp12Energy, init_p[0..], cfg, &bp1_s, &cp_s, &dp_s) ?? unreachable;
    const p2 = annealPermutation(circleTsp12Energy, init_p[0..], cfg, &bp2_s, &cp_s, &dp_s) ?? unreachable;
    expect_eq(p1.best_energy == p2.best_energy, true);
    expect_eq(p1.accepted, p2.accepted);
    let i: usize = 0;
    while (i < 6) {
        expect_eq(bp1[i], bp2[i]);
        i += 1;
    }
});

test("6. Defensive guards: invalid temps/bounds/permutations, short buffers, and NaN", () => {
    const x0 = [1.0, 1.0];
    const lo = [-2.0, -2.0];
    const hi = [2.0, 2.0];
    const bad_hi = [-2.0, 2.0];
    const zero_f: f64 = 0.0;
    const nan_f: f64 = zero_f / zero_f;
    const nan_x0 = [nan_f, 1.0];

    let bx: [2]f64 = std.mem.zeroes([2]f64);
    let c: [2]f64 = std.mem.zeroes([2]f64);
    let d: [2]f64 = std.mem.zeroes([2]f64);
    let short_buf: [1]f64 = std.mem.zeroes([1]f64);
    let bx_s: []f64 = bx[0..];
    let c_s: []f64 = c[0..];
    let d_s: []f64 = d[0..];
    let short_s: []f64 = short_buf[0..];

    let bad_cfg = AnnealConfig.default();
    bad_cfg.t_init = 0.0;
    expect_eq(annealContinuous(sphereObj, x0[0..], lo[0..], hi[0..], bad_cfg, &bx_s, &c_s, &d_s) == null, true);
    expect_eq(annealContinuous(sphereObj, x0[0..], lo[0..], bad_hi[0..], AnnealConfig.default(), &bx_s, &c_s, &d_s) == null, true);
    expect_eq(annealContinuous(sphereObj, nan_x0[0..], lo[0..], hi[0..], AnnealConfig.default(), &bx_s, &c_s, &d_s) == null, true);
    expect_eq(annealContinuous(nanRealObj, x0[0..], lo[0..], hi[0..], AnnealConfig.default(), &bx_s, &c_s, &d_s) == null, true);
    expect_eq(annealContinuous(sphereObj, x0[0..], lo[0..], hi[0..], AnnealConfig.default(), &short_s, &c_s, &d_s) == null, true);

    // Invalid permutations (duplicate element, out-of-range element, short buffer)
    const dup_perm: [4]usize = [0, 1, 1, 3];
    const oob_perm: [4]usize = [0, 1, 2, 9];
    const ok_perm: [4]usize = [0, 1, 2, 3];
    let bp: [4]usize = std.mem.zeroes([4]usize);
    let cp: [4]usize = std.mem.zeroes([4]usize);
    let dp: [4]usize = std.mem.zeroes([4]usize);
    let short_p: [2]usize = std.mem.zeroes([2]usize);
    let bp_s: []usize = bp[0..];
    let cp_s: []usize = cp[0..];
    let dp_s: []usize = dp[0..];
    let short_ps: []usize = short_p[0..];

    expect_eq(annealPermutation(circleTsp12Energy, dup_perm[0..], AnnealConfig.default(), &bp_s, &cp_s, &dp_s) == null, true);
    expect_eq(annealPermutation(circleTsp12Energy, oob_perm[0..], AnnealConfig.default(), &bp_s, &cp_s, &dp_s) == null, true);
    expect_eq(annealPermutation(circleTsp12Energy, ok_perm[0..], AnnealConfig.default(), &short_ps, &cp_s, &dp_s) == null, true);
    expect_eq(annealPermutation(circleTsp12Energy, ok_perm[0..], bad_cfg, &bp_s, &cp_s, &dp_s) == null, true);
});
