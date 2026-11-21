// Source: cp/cap/test/test_pso_production_deep.cp (Production deep tests for cap:pso)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PsoConfig,
    PsoWorkspace,
    DeConfig,
    DeWorkspace,
    psoMinimize,
    deMinimize,
} from "cap:pso";

const PI: f64 = 3.14159265358979323846;

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

/// Sphere function: f(x) = sum_i x_i^2, unique global minimum at x = 0 with f* = 0.
fn sphereObj(x: []const f64): f64 {
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < x.len) {
        sum += x[i] * x[i];
        i += 1;
    }
    return sum;
}

/// Multimodal Rastrigin function: f(x) = 10*n + sum_i (x_i^2 - 10*cos(2*pi*x_i)), global min at 0.
fn rastriginObj(x: []const f64): f64 {
    const n = x.len;
    let sum: f64 = 10.0 * (n as f64);
    let i: usize = 0;
    while (i < n) {
        sum += x[i] * x[i] - 10.0 * std.math.cos(2.0 * PI * x[i]);
        i += 1;
    }
    return sum;
}

/// Multimodal Ackley 2D function: global min at (0, 0) with f* = 0.
fn ackley2dObj(x: []const f64): f64 {
    const sq = 0.5 * (x[0] * x[0] + x[1] * x[1]);
    const cs = 0.5 * (std.math.cos(2.0 * PI * x[0]) + std.math.cos(2.0 * PI * x[1]));
    return -20.0 * std.math.exp(-0.2 * std.math.sqrt(sq)) - std.math.exp(cs) + 20.0 + std.math.e;
}

/// Multimodal Himmelblau 2D function: 4 global minima with f* = 0.
fn himmelblau2dObj(x: []const f64): f64 {
    const a = x[0] * x[0] + x[1] - 11.0;
    const b = x[0] + x[1] * x[1] - 7.0;
    return a * a + b * b;
}

/// Shifted quadratic whose unconstrained minimum (10, -10) lies outside [1, 3] x [-4, -2].
/// Box-constrained minimum is at corner (3, -4) with f* = (3-10)^2 + (-4+10)^2 = 49 + 36 = 85.
fn shiftedBoxObj(x: []const f64): f64 {
    const dx = x[0] - 10.0;
    const dy = x[1] + 10.0;
    return dx * dx + dy * dy;
}

fn nanObj(x: []const f64): f64 {
    const zero: f64 = 0.0;
    return (x[0] * zero) / zero;
}

test("1. Sphere 4D: PSO and DE converge to global minimum < 1e-6", () => {
    const lower = [-5.0, -5.0, -5.0, -5.0];
    const upper = [5.0, 5.0, 5.0, 5.0];

    // PSO (swarm_size = 40, dim = 4 -> 160)
    let pso_best: [4]f64 = std.mem.zeroes([4]f64);
    let pos: [160]f64 = std.mem.zeroes([160]f64);
    let vel: [160]f64 = std.mem.zeroes([160]f64);
    let pbest_pos: [160]f64 = std.mem.zeroes([160]f64);
    let pbest_val: [40]f64 = std.mem.zeroes([40]f64);
    let pso_ws = PsoWorkspace {
        pos: pos[0..],
        vel: vel[0..],
        pbest_pos: pbest_pos[0..],
        pbest_val: pbest_val[0..],
    };
    let pso_out: []f64 = pso_best[0..];
    let pso_cfg = PsoConfig.default();
    pso_cfg.max_iter = 600;
    pso_cfg.tol = 1.0e-9;
    pso_cfg.seed = 12345;

    const pso_res = psoMinimize(sphereObj, lower[0..], upper[0..], 40, pso_cfg, &pso_out, &pso_ws) ?? unreachable;
    expect_eq(pso_res.best_value < 1.0e-6, true);
    expect_eq(pso_res.evals > 40, true);
    let j: usize = 0;
    while (j < 4) {
        expect_eq(absF64(pso_best[j]) < 1.0e-3, true);
        j += 1;
    }

    // DE (pop_size = 30, dim = 4 -> 120)
    let de_best: [4]f64 = std.mem.zeroes([4]f64);
    let pop: [120]f64 = std.mem.zeroes([120]f64);
    let fitness: [30]f64 = std.mem.zeroes([30]f64);
    let trial: [4]f64 = std.mem.zeroes([4]f64);
    let de_ws = DeWorkspace {
        pop: pop[0..],
        fitness: fitness[0..],
        trial: trial[0..],
    };
    let de_out: []f64 = de_best[0..];
    let de_cfg = DeConfig.default();
    de_cfg.max_iter = 400;
    de_cfg.tol = 1.0e-9;
    de_cfg.seed = 54321;

    const de_res = deMinimize(sphereObj, lower[0..], upper[0..], 30, de_cfg, &de_out, &de_ws) ?? unreachable;
    expect_eq(de_res.best_value < 1.0e-6, true);
    expect_eq(de_res.evals > 30, true);
    j = 0;
    while (j < 4) {
        expect_eq(absF64(de_best[j]) < 1.0e-3, true);
        j += 1;
    }
});

test("2. Multimodal Rastrigin 2D: PSO and DE escape local minima to (0, 0)", () => {
    const lower = [-5.12, -5.12];
    const upper = [5.12, 5.12];

    // PSO (swarm_size = 48, dim = 2 -> 96)
    let pso_best: [2]f64 = std.mem.zeroes([2]f64);
    let pos: [96]f64 = std.mem.zeroes([96]f64);
    let vel: [96]f64 = std.mem.zeroes([96]f64);
    let pbest_pos: [96]f64 = std.mem.zeroes([96]f64);
    let pbest_val: [48]f64 = std.mem.zeroes([48]f64);
    let pso_ws = PsoWorkspace {
        pos: pos[0..],
        vel: vel[0..],
        pbest_pos: pbest_pos[0..],
        pbest_val: pbest_val[0..],
    };
    let pso_out: []f64 = pso_best[0..];
    let pso_cfg = PsoConfig.default();
    pso_cfg.max_iter = 500;
    pso_cfg.seed = 777;

    const pso_res = psoMinimize(rastriginObj, lower[0..], upper[0..], 48, pso_cfg, &pso_out, &pso_ws) ?? unreachable;
    expect_eq(pso_res.best_value < 1.0e-5, true);
    expect_eq(absF64(pso_best[0]) < 1.0e-3, true);
    expect_eq(absF64(pso_best[1]) < 1.0e-3, true);

    // DE (pop_size = 32, dim = 2 -> 64)
    let de_best: [2]f64 = std.mem.zeroes([2]f64);
    let pop: [64]f64 = std.mem.zeroes([64]f64);
    let fitness: [32]f64 = std.mem.zeroes([32]f64);
    let trial: [2]f64 = std.mem.zeroes([2]f64);
    let de_ws = DeWorkspace {
        pop: pop[0..],
        fitness: fitness[0..],
        trial: trial[0..],
    };
    let de_out: []f64 = de_best[0..];
    let de_cfg = DeConfig.default();
    de_cfg.max_iter = 400;
    de_cfg.seed = 888;

    const de_res = deMinimize(rastriginObj, lower[0..], upper[0..], 32, de_cfg, &de_out, &de_ws) ?? unreachable;
    expect_eq(de_res.best_value < 1.0e-6, true);
    expect_eq(absF64(de_best[0]) < 1.0e-3, true);
    expect_eq(absF64(de_best[1]) < 1.0e-3, true);
});

test("3. Multimodal Ackley 2D (DE) & Himmelblau 2D (PSO) benchmarks", () => {
    const lower = [-5.0, -5.0];
    const upper = [5.0, 5.0];

    // Ackley 2D via DE
    let ack_best: [2]f64 = std.mem.zeroes([2]f64);
    let pop: [64]f64 = std.mem.zeroes([64]f64);
    let fitness: [32]f64 = std.mem.zeroes([32]f64);
    let trial: [2]f64 = std.mem.zeroes([2]f64);
    let de_ws = DeWorkspace {
        pop: pop[0..],
        fitness: fitness[0..],
        trial: trial[0..],
    };
    let ack_out: []f64 = ack_best[0..];
    let de_cfg = DeConfig.default();
    de_cfg.max_iter = 400;
    de_cfg.seed = 2026;

    const ack_res = deMinimize(ackley2dObj, lower[0..], upper[0..], 32, de_cfg, &ack_out, &de_ws) ?? unreachable;
    expect_eq(ack_res.best_value < 1.0e-5, true);
    expect_eq(absF64(ack_best[0]) < 1.0e-4, true);
    expect_eq(absF64(ack_best[1]) < 1.0e-4, true);

    // Himmelblau 2D via PSO
    let him_best: [2]f64 = std.mem.zeroes([2]f64);
    let pos: [80]f64 = std.mem.zeroes([80]f64);
    let vel: [80]f64 = std.mem.zeroes([80]f64);
    let pbest_pos: [80]f64 = std.mem.zeroes([80]f64);
    let pbest_val: [40]f64 = std.mem.zeroes([40]f64);
    let pso_ws = PsoWorkspace {
        pos: pos[0..],
        vel: vel[0..],
        pbest_pos: pbest_pos[0..],
        pbest_val: pbest_val[0..],
    };
    let him_out: []f64 = him_best[0..];
    let pso_cfg = PsoConfig.default();
    pso_cfg.max_iter = 500;
    pso_cfg.seed = 9999;

    const him_res = psoMinimize(himmelblau2dObj, lower[0..], upper[0..], 40, pso_cfg, &him_out, &pso_ws) ?? unreachable;
    expect_eq(him_res.best_value < 1.0e-6, true);
    expect_eq(him_best[0] >= lower[0] && him_best[0] <= upper[0], true);
    expect_eq(him_best[1] >= lower[1] && him_best[1] <= upper[1], true);
});

test("4. Strict box constraints: optimum outside box converges cleanly to corner", () => {
    const lower = [1.0, -4.0];
    const upper = [3.0, -2.0];

    // PSO
    let pso_best: [2]f64 = std.mem.zeroes([2]f64);
    let pos: [60]f64 = std.mem.zeroes([60]f64);
    let vel: [60]f64 = std.mem.zeroes([60]f64);
    let pbest_pos: [60]f64 = std.mem.zeroes([60]f64);
    let pbest_val: [30]f64 = std.mem.zeroes([30]f64);
    let pso_ws = PsoWorkspace {
        pos: pos[0..],
        vel: vel[0..],
        pbest_pos: pbest_pos[0..],
        pbest_val: pbest_val[0..],
    };
    let pso_out: []f64 = pso_best[0..];
    let pso_cfg = PsoConfig.default();
    pso_cfg.max_iter = 300;
    pso_cfg.seed = 314;

    const pso_res = psoMinimize(shiftedBoxObj, lower[0..], upper[0..], 30, pso_cfg, &pso_out, &pso_ws) ?? unreachable;
    expect_eq(pso_best[0] >= lower[0] && pso_best[0] <= upper[0], true);
    expect_eq(pso_best[1] >= lower[1] && pso_best[1] <= upper[1], true);
    expect_eq(absF64(pso_best[0] - 3.0) < 1.0e-4, true);
    expect_eq(absF64(pso_best[1] - (-4.0)) < 1.0e-4, true);
    expect_eq(absF64(pso_res.best_value - 85.0) < 1.0e-2, true);

    // DE
    let de_best: [2]f64 = std.mem.zeroes([2]f64);
    let pop: [48]f64 = std.mem.zeroes([48]f64);
    let fitness: [24]f64 = std.mem.zeroes([24]f64);
    let trial: [2]f64 = std.mem.zeroes([2]f64);
    let de_ws = DeWorkspace {
        pop: pop[0..],
        fitness: fitness[0..],
        trial: trial[0..],
    };
    let de_out: []f64 = de_best[0..];
    let de_cfg = DeConfig.default();
    de_cfg.max_iter = 300;
    de_cfg.seed = 271;

    const de_res = deMinimize(shiftedBoxObj, lower[0..], upper[0..], 24, de_cfg, &de_out, &de_ws) ?? unreachable;
    expect_eq(de_best[0] >= lower[0] && de_best[0] <= upper[0], true);
    expect_eq(de_best[1] >= lower[1] && de_best[1] <= upper[1], true);
    expect_eq(absF64(de_best[0] - 3.0) < 1.0e-4, true);
    expect_eq(absF64(de_best[1] - (-4.0)) < 1.0e-4, true);
    expect_eq(absF64(de_res.best_value - 85.0) < 1.0e-2, true);
});

test("5. Bit-exact determinism with identical seed for both PSO and DE", () => {
    const lower = [-5.12, -5.12];
    const upper = [5.12, 5.12];

    // PSO run 1 vs run 2
    let p_best1: [2]f64 = std.mem.zeroes([2]f64);
    let p_best2: [2]f64 = std.mem.zeroes([2]f64);
    let pos: [40]f64 = std.mem.zeroes([40]f64);
    let vel: [40]f64 = std.mem.zeroes([40]f64);
    let pbest_pos: [40]f64 = std.mem.zeroes([40]f64);
    let pbest_val: [20]f64 = std.mem.zeroes([20]f64);
    let pso_ws = PsoWorkspace {
        pos: pos[0..],
        vel: vel[0..],
        pbest_pos: pbest_pos[0..],
        pbest_val: pbest_val[0..],
    };
    let p_cfg = PsoConfig.default();
    p_cfg.max_iter = 120;
    p_cfg.seed = 0xCAFEBABE12345678;

    let p_out1: []f64 = p_best1[0..];
    let p_out2: []f64 = p_best2[0..];
    const pr1 = psoMinimize(rastriginObj, lower[0..], upper[0..], 20, p_cfg, &p_out1, &pso_ws) ?? unreachable;
    const pr2 = psoMinimize(rastriginObj, lower[0..], upper[0..], 20, p_cfg, &p_out2, &pso_ws) ?? unreachable;

    expect_eq(pr1.iterations, pr2.iterations);
    expect_eq(pr1.evals, pr2.evals);
    expect_eq(pr1.converged, pr2.converged);
    expect_eq(pr1.best_value == pr2.best_value, true);
    expect_eq(p_best1[0] == p_best2[0], true);
    expect_eq(p_best1[1] == p_best2[1], true);

    // DE run 1 vs run 2
    let d_best1: [2]f64 = std.mem.zeroes([2]f64);
    let d_best2: [2]f64 = std.mem.zeroes([2]f64);
    let pop: [40]f64 = std.mem.zeroes([40]f64);
    let fitness: [20]f64 = std.mem.zeroes([20]f64);
    let trial: [2]f64 = std.mem.zeroes([2]f64);
    let de_ws = DeWorkspace {
        pop: pop[0..],
        fitness: fitness[0..],
        trial: trial[0..],
    };
    let d_cfg = DeConfig.default();
    d_cfg.max_iter = 120;
    d_cfg.seed = 0xDEADBEEF87654321;

    let d_out1: []f64 = d_best1[0..];
    let d_out2: []f64 = d_best2[0..];
    const dr1 = deMinimize(rastriginObj, lower[0..], upper[0..], 20, d_cfg, &d_out1, &de_ws) ?? unreachable;
    const dr2 = deMinimize(rastriginObj, lower[0..], upper[0..], 20, d_cfg, &d_out2, &de_ws) ?? unreachable;

    expect_eq(dr1.iterations, dr2.iterations);
    expect_eq(dr1.evals, dr2.evals);
    expect_eq(dr1.converged, dr2.converged);
    expect_eq(dr1.best_value == dr2.best_value, true);
    expect_eq(d_best1[0] == d_best2[0], true);
    expect_eq(d_best1[1] == d_best2[1], true);
});

test("6. Defensive guards: invalid sizes, bounds, workspaces, configs, and NaN", () => {
    const lower = [-2.0, -2.0];
    const upper = [2.0, 2.0];
    const bad_eq_hi = [-2.0, 2.0];
    const bad_inv_hi = [-3.0, 2.0];
    const zero_f: f64 = 0.0;
    const nan_val: f64 = zero_f / zero_f;
    const nan_lo = [nan_val, -2.0];

    let best_x: [2]f64 = std.mem.zeroes([2]f64);
    let best_short: [1]f64 = std.mem.zeroes([1]f64);
    let pos: [20]f64 = std.mem.zeroes([20]f64);
    let vel: [20]f64 = std.mem.zeroes([20]f64);
    let pbest_pos: [20]f64 = std.mem.zeroes([20]f64);
    let pbest_val: [10]f64 = std.mem.zeroes([10]f64);
    let pso_ws = PsoWorkspace {
        pos: pos[0..],
        vel: vel[0..],
        pbest_pos: pbest_pos[0..],
        pbest_val: pbest_val[0..],
    };
    let bx_s: []f64 = best_x[0..];
    let bx_short: []f64 = best_short[0..];

    // PSO: swarm_size == 0
    expect_eq(psoMinimize(sphereObj, lower[0..], upper[0..], 0, PsoConfig.default(), &bx_s, &pso_ws) == null, true);
    // PSO: lower == upper / lower > upper / NaN bound
    expect_eq(psoMinimize(sphereObj, lower[0..], bad_eq_hi[0..], 10, PsoConfig.default(), &bx_s, &pso_ws) == null, true);
    expect_eq(psoMinimize(sphereObj, lower[0..], bad_inv_hi[0..], 10, PsoConfig.default(), &bx_s, &pso_ws) == null, true);
    expect_eq(psoMinimize(sphereObj, nan_lo[0..], upper[0..], 10, PsoConfig.default(), &bx_s, &pso_ws) == null, true);
    // PSO: best_x_out too short & workspace too small (request swarm_size = 11 > 10)
    expect_eq(psoMinimize(sphereObj, lower[0..], upper[0..], 10, PsoConfig.default(), &bx_short, &pso_ws) == null, true);
    expect_eq(psoMinimize(sphereObj, lower[0..], upper[0..], 11, PsoConfig.default(), &bx_s, &pso_ws) == null, true);
    // PSO: all-NaN objective
    expect_eq(psoMinimize(nanObj, lower[0..], upper[0..], 10, PsoConfig.default(), &bx_s, &pso_ws) == null, true);

    // DE workspace (capacity 10)
    let pop: [20]f64 = std.mem.zeroes([20]f64);
    let fitness: [10]f64 = std.mem.zeroes([10]f64);
    let trial: [2]f64 = std.mem.zeroes([2]f64);
    let de_ws = DeWorkspace {
        pop: pop[0..],
        fitness: fitness[0..],
        trial: trial[0..],
    };

    // DE: pop_size < 4
    expect_eq(deMinimize(sphereObj, lower[0..], upper[0..], 3, DeConfig.default(), &bx_s, &de_ws) == null, true);
    // DE: lower >= upper / NaN bound
    expect_eq(deMinimize(sphereObj, lower[0..], bad_eq_hi[0..], 10, DeConfig.default(), &bx_s, &de_ws) == null, true);
    expect_eq(deMinimize(sphereObj, lower[0..], bad_inv_hi[0..], 10, DeConfig.default(), &bx_s, &de_ws) == null, true);
    expect_eq(deMinimize(sphereObj, nan_lo[0..], upper[0..], 10, DeConfig.default(), &bx_s, &de_ws) == null, true);
    // DE: workspace too small (request pop_size = 11 > 10) & best_x_out too short
    expect_eq(deMinimize(sphereObj, lower[0..], upper[0..], 11, DeConfig.default(), &bx_s, &de_ws) == null, true);
    expect_eq(deMinimize(sphereObj, lower[0..], upper[0..], 10, DeConfig.default(), &bx_short, &de_ws) == null, true);
    // DE: invalid config (mutation_f == 0, crossover_cr > 1)
    let bad_de_cfg = DeConfig.default();
    bad_de_cfg.mutation_f = 0.0;
    expect_eq(deMinimize(sphereObj, lower[0..], upper[0..], 10, bad_de_cfg, &bx_s, &de_ws) == null, true);
    bad_de_cfg = DeConfig.default();
    bad_de_cfg.crossover_cr = 1.5;
    expect_eq(deMinimize(sphereObj, lower[0..], upper[0..], 10, bad_de_cfg, &bx_s, &de_ws) == null, true);
    // DE: all-NaN objective
    expect_eq(deMinimize(nanObj, lower[0..], upper[0..], 10, DeConfig.default(), &bx_s, &de_ws) == null, true);
});
