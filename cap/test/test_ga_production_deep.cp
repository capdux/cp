// Source: cp/cap/test/test_ga_production_deep.cp (Production deep tests for cap:ga)

import { test, expect_eq } from "cap:test";
import {
    RealObjectiveFn,
    PermObjectiveFn,
    RealGaConfig,
    PermGaConfig,
    GaResult,
    RealGaWorkspace,
    PermGaWorkspace,
    isValidPermutation,
    orderCrossover,
    realGaMinimize,
    permGaMinimize,
} from "cap:ga";

const TWO_PI: f64 = 6.28318530717958647692;

fn absF64(v: f64): f64 {
    return if (v < 0.0) (0.0 - v) else v;
}

fn sphere4d(x: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < x.len) {
        s += x[i] * x[i];
        i += 1;
    }
    return s;
}

fn rastrigin2d(x: []const f64): f64 {
    let s: f64 = 10.0 * (x.len as f64);
    let i: usize = 0;
    while (i < x.len) {
        const v: f64 = x[i];
        s += (v * v) - (10.0 * @cos(TWO_PI * v));
        i += 1;
    }
    return s;
}

fn tspCircle10(perm: []const usize): f64 {
    const n: usize = perm.len;
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const a: usize = perm[i];
        const b: usize = perm[(i + 1) % n];
        const ang_a: f64 = TWO_PI * (a as f64) / (n as f64);
        const ang_b: f64 = TWO_PI * (b as f64) / (n as f64);
        const dx: f64 = @cos(ang_a) - @cos(ang_b);
        const dy: f64 = @sin(ang_a) - @sin(ang_b);
        total += @sqrt((dx * dx) + (dy * dy));
        i += 1;
    }
    return total;
}

fn nanObjective(x: []const f64): f64 {
    * = x;
    const z: f64 = 0.0;
    return z / z;
}

test("1. Real-Coded GA: 4D Sphere & 2D Rastrigin convergence + monotonic elite fitness", () => {
    // (a) 4D Sphere over [-5, 5]^4
    const lo4: [4]f64 = [-5.0, -5.0, -5.0, -5.0];
    const hi4: [4]f64 = [5.0, 5.0, 5.0, 5.0];
    let pa4: [240]f64 = [0.0; 240];
    let pb4: [240]f64 = [0.0; 240];
    let fit4: [60]f64 = [0.0; 60];
    let ord4: [60]usize = [0; 60];
    let best4_b: [4]f64 = [0.0; 4];
    let best4: []f64 = best4_b[0..];

    let ws4 = RealGaWorkspace {
        pop_a: pa4[0..],
        pop_b: pb4[0..],
        fitness: fit4[0..],
        order: ord4[0..],
    };

    // Check monotonic non-increasing elite fitness across generation checkpoints (same seed)
    let prev_best: f64 = 1.0e30;
    const checkpoints: [4]usize = [0, 20, 80, 200];
    let ci: usize = 0;
    while (ci < 4) {
        let cfg: RealGaConfig = RealGaConfig.default();
        cfg.generations = checkpoints[ci];
        cfg.mutation_sigma = 0.12;
        cfg.seed = 2026;
        const r: GaResult = realGaMinimize(sphere4d, lo4[0..], hi4[0..], 60, cfg, *best4, *ws4) ?? unreachable;
        expect_eq(r.best_value <= prev_best + 1.0e-12, true);
        prev_best = r.best_value;
        ci += 1;
    }
    expect_eq(prev_best < 1.0e-3, true);

    // Verify box bounds respected
    let d: usize = 0;
    while (d < 4) {
        expect_eq(best4[d] >= lo4[d] && best4[d] <= hi4[d], true);
        d += 1;
    }

    // (b) Multimodal 2D Rastrigin over [-5.12, 5.12]^2 (global minimum = 0 at (0, 0))
    const lo2: [2]f64 = [-5.12, -5.12];
    const hi2: [2]f64 = [5.12, 5.12];
    let pa2: [160]f64 = [0.0; 160];
    let pb2: [160]f64 = [0.0; 160];
    let fit2: [80]f64 = [0.0; 80];
    let ord2: [80]usize = [0; 80];
    let best2_b: [2]f64 = [0.0; 2];
    let best2: []f64 = best2_b[0..];

    let ws2 = RealGaWorkspace {
        pop_a: pa2[0..],
        pop_b: pb2[0..],
        fitness: fit2[0..],
        order: ord2[0..],
    };

    let cfg_r: RealGaConfig = RealGaConfig.default();
    cfg_r.generations = 250;
    cfg_r.mutation_rate = 0.25;
    cfg_r.mutation_sigma = 0.25;
    cfg_r.seed = 1337;
    const res_r: GaResult = realGaMinimize(rastrigin2d, lo2[0..], hi2[0..], 80, cfg_r, *best2, *ws2) ?? unreachable;
    expect_eq(res_r.best_value < 0.05, true);
});

test("2. orderCrossover preserves 100% permutation validity across all cut combinations", () => {
    const p1: [8]usize = [0, 1, 2, 3, 4, 5, 6, 7];
    const p2: [8]usize = [3, 7, 5, 1, 6, 0, 2, 4];

    let child_b: [8]usize = [0; 8];
    let child: []usize = child_b[0..];
    let mark_b: [8]u8 = [0; 8];
    let mark: []u8 = mark_b[0..];

    // Exhaustively test all (cut1, cut2) pairs including cut1 > cut2 and cut1 == cut2
    let c1: usize = 0;
    while (c1 < 8) {
        let c2: usize = 0;
        while (c2 < 8) {
            expect_eq(orderCrossover(p1[0..], p2[0..], c1, c2, *child, *mark), true);
            expect_eq(isValidPermutation(child, *mark), true);

            // Segment [min(c1, c2)..=max(c1, c2)] must match p1 verbatim
            const lo: usize = if (c1 <= c2) c1 else c2;
            const hi: usize = if (c1 <= c2) c2 else c1;
            let k: usize = lo;
            while (k <= hi) {
                expect_eq(child[k], p1[k]);
                k += 1;
            }
            c2 += 1;
        }
        c1 += 1;
    }

    // Invalid cuts and invalid parent permutations rejected
    expect_eq(orderCrossover(p1[0..], p2[0..], 0, 8, *child, *mark), false);
    const bad_p: [8]usize = [0, 1, 2, 2, 4, 5, 6, 7];
    expect_eq(isValidPermutation(bad_p[0..], *mark), false);
    expect_eq(orderCrossover(bad_p[0..], p2[0..], 1, 4, *child, *mark), false);
});

test("3. Permutation GA: 10-city circular TSP reaches theoretical shortest perimeter", () => {
    // 10 cities evenly placed on the unit circle.
    // Optimal tour perimeter = 10 * 2 * sin(pi / 10) ~= 6.1803398875
    const optimal_perimeter: f64 = 10.0 * 2.0 * @sin(TWO_PI / 20.0);

    let pa: [800]usize = [0; 800];
    let pb: [800]usize = [0; 800];
    let fit: [80]f64 = [0.0; 80];
    let ord: [80]usize = [0; 80];
    let mark_b: [10]u8 = [0; 10];
    let best_b: [10]usize = [0; 10];
    let best: []usize = best_b[0..];

    let ws = PermGaWorkspace {
        pop_a: pa[0..],
        pop_b: pb[0..],
        fitness: fit[0..],
        order: ord[0..],
        used_mark: mark_b[0..],
    };

    let cfg: PermGaConfig = PermGaConfig.default();
    cfg.generations = 250;
    cfg.crossover_rate = 0.85;
    cfg.mutation_rate = 0.3;
    cfg.elitism_count = 4;
    cfg.seed = 777;

    const res: GaResult = permGaMinimize(tspCircle10, 10, 80, cfg, *best, *ws) ?? unreachable;

    let seen_b: [10]u8 = [0; 10];
    let seen: []u8 = seen_b[0..];
    expect_eq(isValidPermutation(best, *seen), true);
    expect_eq(absF64(res.best_value - optimal_perimeter) < 1.0e-6, true);
    expect_eq(absF64(tspCircle10(best) - res.best_value) < 1.0e-12, true);
});

test("4. Bit-exact deterministic reproducibility with identical seed (Real GA & Perm GA)", () => {
    // (a) Real GA determinism
    const lo: [3]f64 = [-4.0, -4.0, -4.0];
    const hi: [3]f64 = [4.0, 4.0, 4.0];
    let pa1: [90]f64 = [0.0; 90];
    let pb1: [90]f64 = [0.0; 90];
    let fit1: [30]f64 = [0.0; 30];
    let ord1: [30]usize = [0; 30];
    let bx1_b: [3]f64 = [0.0; 3];
    let bx1: []f64 = bx1_b[0..];

    let ws1 = RealGaWorkspace { pop_a: pa1[0..], pop_b: pb1[0..], fitness: fit1[0..], order: ord1[0..] };

    let cfg_real: RealGaConfig = RealGaConfig.default();
    cfg_real.generations = 80;
    cfg_real.seed = 987654321;

    const r1: GaResult = realGaMinimize(sphere4d, lo[0..], hi[0..], 30, cfg_real, *bx1, *ws1) ?? unreachable;

    let bx2_b: [3]f64 = [0.0; 3];
    let bx2: []f64 = bx2_b[0..];
    const r2: GaResult = realGaMinimize(sphere4d, lo[0..], hi[0..], 30, cfg_real, *bx2, *ws1) ?? unreachable;

    expect_eq(r1.best_value, r2.best_value);
    expect_eq(r1.evals, r2.evals);
    expect_eq(bx1[0], bx2[0]);
    expect_eq(bx1[1], bx2[1]);
    expect_eq(bx1[2], bx2[2]);

    // (b) Perm GA determinism
    let ppa: [240]usize = [0; 240];
    let ppb: [240]usize = [0; 240];
    let pfit: [30]f64 = [0.0; 30];
    let pord: [30]usize = [0; 30];
    let pmark: [8]u8 = [0; 8];
    let bp1_b: [8]usize = [0; 8];
    let bp2_b: [8]usize = [0; 8];
    let bp1: []usize = bp1_b[0..];
    let bp2: []usize = bp2_b[0..];

    let pws = PermGaWorkspace {
        pop_a: ppa[0..],
        pop_b: ppb[0..],
        fitness: pfit[0..],
        order: pord[0..],
        used_mark: pmark[0..],
    };

    let cfg_perm: PermGaConfig = PermGaConfig.default();
    cfg_perm.generations = 60;
    cfg_perm.seed = 123456789;

    const pr1: GaResult = permGaMinimize(tspCircle10, 8, 30, cfg_perm, *bp1, *pws) ?? unreachable;
    const pr2: GaResult = permGaMinimize(tspCircle10, 8, 30, cfg_perm, *bp2, *pws) ?? unreachable;

    expect_eq(pr1.best_value, pr2.best_value);
    expect_eq(pr1.evals, pr2.evals);
    let k: usize = 0;
    while (k < 8) {
        expect_eq(bp1[k], bp2[k]);
        k += 1;
    }
});

test("5. Boundary defense: invalid pop_size, lower >= upper, bad rates, short buffers & NaN", () => {
    const lo_ok: [2]f64 = [-2.0, -2.0];
    const hi_ok: [2]f64 = [2.0, 2.0];
    let pa: [20]f64 = [0.0; 20];
    let pb: [20]f64 = [0.0; 20];
    let fit: [10]f64 = [0.0; 10];
    let ord: [10]usize = [0; 10];
    let bx_b: [2]f64 = [0.0; 2];
    let bx: []f64 = bx_b[0..];

    let ws = RealGaWorkspace { pop_a: pa[0..], pop_b: pb[0..], fitness: fit[0..], order: ord[0..] };
    const cfg_ok: RealGaConfig = RealGaConfig.default();

    // pop_size < 2
    expect_eq(realGaMinimize(sphere4d, lo_ok[0..], hi_ok[0..], 1, cfg_ok, *bx, *ws) == null, true);
    expect_eq(realGaMinimize(sphere4d, lo_ok[0..], hi_ok[0..], 0, cfg_ok, *bx, *ws) == null, true);

    // lower >= upper
    const lo_bad: [2]f64 = [2.0, -2.0];
    const hi_bad: [2]f64 = [2.0, 2.0];
    expect_eq(realGaMinimize(sphere4d, lo_bad[0..], hi_bad[0..], 10, cfg_ok, *bx, *ws) == null, true);

    // Invalid crossover / mutation rates & elitism >= pop_size
    let bad_cx: RealGaConfig = cfg_ok;
    bad_cx.crossover_rate = 1.5;
    expect_eq(realGaMinimize(sphere4d, lo_ok[0..], hi_ok[0..], 10, bad_cx, *bx, *ws) == null, true);

    let bad_mut: RealGaConfig = cfg_ok;
    bad_mut.mutation_rate = -0.1;
    expect_eq(realGaMinimize(sphere4d, lo_ok[0..], hi_ok[0..], 10, bad_mut, *bx, *ws) == null, true);

    let bad_elite: RealGaConfig = cfg_ok;
    bad_elite.elitism_count = 10;
    expect_eq(realGaMinimize(sphere4d, lo_ok[0..], hi_ok[0..], 10, bad_elite, *bx, *ws) == null, true);

    // NaN bounds & NaN objective return
    const z: f64 = 0.0;
    const nan_v: f64 = z / z;
    const lo_nan: [2]f64 = [nan_v, -2.0];
    expect_eq(realGaMinimize(sphere4d, lo_nan[0..], hi_ok[0..], 10, cfg_ok, *bx, *ws) == null, true);
    expect_eq(realGaMinimize(nanObjective, lo_ok[0..], hi_ok[0..], 10, cfg_ok, *bx, *ws) == null, true);

    // Short workspace buffer
    let ws_short = RealGaWorkspace { pop_a: pa[0..10], pop_b: pb[0..20], fitness: fit[0..10], order: ord[0..10] };
    expect_eq(realGaMinimize(sphere4d, lo_ok[0..], hi_ok[0..], 10, cfg_ok, *bx, *ws_short) == null, true);

    // Perm GA invalid inputs
    let ppa: [20]usize = [0; 20];
    let ppb: [20]usize = [0; 20];
    let pmark: [4]u8 = [0; 4];
    let bp_b: [4]usize = [0; 4];
    let bp: []usize = bp_b[0..];
    let pws = PermGaWorkspace { pop_a: ppa[0..], pop_b: ppb[0..], fitness: fit[0..], order: ord[0..], used_mark: pmark[0..] };
    const pcfg: PermGaConfig = PermGaConfig.default();

    expect_eq(permGaMinimize(tspCircle10, 0, 5, pcfg, *bp, *pws) == null, true);
    expect_eq(permGaMinimize(tspCircle10, 4, 1, pcfg, *bp, *pws) == null, true);
    // pop_a length 20 < 6 * 4 = 24
    expect_eq(permGaMinimize(tspCircle10, 4, 6, pcfg, *bp, *pws) == null, true);
});
