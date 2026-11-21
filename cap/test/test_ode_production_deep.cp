// Source: cp/cap/test/test_ode_production_deep.cp (Production deep tests for cap:ode)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Rk45Result,
    rk2MidpointStep,
    rk4Step,
    rk4Integrate,
    rk45Step,
    rk45Integrate,
    verletStep,
} from "cap:ode";

fn absDiff(a: f64, b: f64): f64 {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

// y' = y -> y(t) = y(0) * exp(t)
fn expRhs(t: f64, y: []const f64, dydt: *[]f64) {
    * = t;
    dydt.*[0] = y[0];
}

// Harmonic oscillator: x' = v, v' = -x
fn harmonicRhs(t: f64, y: []const f64, dydt: *[]f64) {
    * = t;
    dydt.*[0] = y[1];
    dydt.*[1] = 0.0 - y[0];
}

// Sharp pulse ODE: y' = -2 * t * y^2 (exact solution y(t) = 1 / (1 + t^2) for y(0)=1, or steep Lorentzian y' = -200*(t-0.5)/(1 + 100*(t-0.5)^2)^2)
fn sharpPeakRhs(t: f64, y: []const f64, dydt: *[]f64) {
    * = y;
    const u: f64 = t - 0.5;
    const denom: f64 = 1.0 + 100.0 * u * u;
    dydt.*[0] = (-200.0 * u) / (denom * denom);
}

// Harmonic oscillator acceleration: a = -x
fn harmonicAccel(t: f64, pos: []const f64, acc: *[]f64) {
    * = t;
    acc.*[0] = 0.0 - pos[0];
}

test("1. RK4 4th-order convergence (~16x error reduction on halving dt) and y'=y -> e (< 1e-10)", () => {
    const exact_e: f64 = std.math.exp(1.0);
    let scr_arr: [8]f64 = [0.0; 8];
    let scr: []f64 = scr_arr[0..];

    // Coarse (N=10) vs half-step (N=20): error ratio must be ~16.0 (2^4).
    let y10_arr: [1]f64 = [1.0];
    let y10: []f64 = y10_arr[0..];
    expect_eq(rk4Integrate(expRhs, 0.0, 1.0, 10, *y10, *scr), true);
    const err10: f64 = absDiff(y10_arr[0], exact_e);

    let y20_arr: [1]f64 = [1.0];
    let y20: []f64 = y20_arr[0..];
    expect_eq(rk4Integrate(expRhs, 0.0, 1.0, 20, *y20, *scr), true);
    const err20: f64 = absDiff(y20_arr[0], exact_e);

    const ratio: f64 = err10 / err20;
    expect_eq(ratio > 15.0 && ratio < 17.5, true);

    // Fine (N=200): accuracy < 1e-10.
    let y200_arr: [1]f64 = [1.0];
    let y200: []f64 = y200_arr[0..];
    expect_eq(rk4Integrate(expRhs, 0.0, 1.0, 200, *y200, *scr), true);
    expect_eq(absDiff(y200_arr[0], exact_e) < 1.0e-10, true);

    // RK2 Midpoint smoke test.
    let y_rk2_arr: [1]f64 = [1.0];
    let y_rk2: []f64 = y_rk2_arr[0..];
    let s: usize = 0;
    while (s < 100) {
        expect_eq(rk2MidpointStep(expRhs, (s as f64) * 0.01, 0.01, *y_rk2, *scr), true);
        s += 1;
    }
    expect_eq(absDiff(y_rk2_arr[0], exact_e) < 1.0e-4, true);
});

test("2. Harmonic oscillator x'' = -x over full 2*pi period returns precisely to (1, 0)", () => {
    let y_arr: [2]f64 = [1.0, 0.0];
    let scr_arr: [16]f64 = [0.0; 16];
    let y: []f64 = y_arr[0..];
    let scr: []f64 = scr_arr[0..];

    const two_pi: f64 = 2.0 * std.math.pi;
    expect_eq(rk4Integrate(harmonicRhs, 0.0, two_pi, 400, *y, *scr), true);
    expect_eq(absDiff(y_arr[0], 1.0) < 1.0e-8, true);
    expect_eq(absDiff(y_arr[1], 0.0) < 1.0e-8, true);
});

test("3. Adaptive Dormand-Prince rk45Integrate reaches rtol=1e-9 accuracy on sharp Lorentzian peak with few steps", () => {
    // Exact solution: y(t) = 1 / (1 + 100*(t - 0.5)^2), y(0) = 1/26, y(1) = 1/26, peak y(0.5) = 1.0.
    const y0_exact: f64 = 1.0 / 26.0;
    let y_arr: [1]f64 = [y0_exact];
    let scr_arr: [16]f64 = [0.0; 16];
    let y: []f64 = y_arr[0..];
    let scr: []f64 = scr_arr[0..];

    if (res := rk45Integrate(sharpPeakRhs, 0.0, 0.5, *y, 1.0e-9, 1.0e-11, 2000, *scr)) {
        expect_eq(res.reached_end, true);
        expect_eq(absDiff(y_arr[0], 1.0) < 1.0e-8, true);
        // Adaptive solver should take far fewer than 100 steps (uniform RK4 needs > 500 for 1e-8 here).
        expect_eq(res.steps_accepted < 100, true);
    } else {
        expect_eq(true, false);
    }

    // Continue from peak 0.5 to 1.0 -> returns to 1/26.
    if (res2 := rk45Integrate(sharpPeakRhs, 0.5, 1.0, *y, 1.0e-9, 1.0e-11, 2000, *scr)) {
        expect_eq(res2.reached_end, true);
        expect_eq(absDiff(y_arr[0], y0_exact) < 1.0e-8, true);
    } else {
        expect_eq(true, false);
    }
});

test("4. Symplectic Velocity Verlet conserves Hamiltonian energy 0.5*(x^2 + v^2) over 1000 steps (< 1e-4 drift)", () => {
    let pos_arr: [1]f64 = [1.0];
    let vel_arr: [1]f64 = [0.0];
    let acc_curr_arr: [1]f64 = [-1.0];
    let acc_next_arr: [1]f64 = [0.0];

    let pos: []f64 = pos_arr[0..];
    let vel: []f64 = vel_arr[0..];
    let acc_curr: []f64 = acc_curr_arr[0..];
    let acc_next: []f64 = acc_next_arr[0..];

    const e0: f64 = 0.5 * (pos_arr[0] * pos_arr[0] + vel_arr[0] * vel_arr[0]);
    const dt: f64 = 0.02;
    let max_drift: f64 = 0.0;

    let step: usize = 0;
    while (step < 1000) {
        const t: f64 = (step as f64) * dt;
        expect_eq(verletStep(harmonicAccel, t, dt, *pos, *vel, *acc_curr, *acc_next), true);
        const e: f64 = 0.5 * (pos_arr[0] * pos_arr[0] + vel_arr[0] * vel_arr[0]);
        const drift: f64 = absDiff(e, e0);
        if (drift > max_drift) {
            max_drift = drift;
        }
        step += 1;
    }

    expect_eq(max_drift < 1.0e-4, true);
});

test("5. Malformed input guards: empty state, short scratch, zero steps, invalid tolerances, mismatched Verlet slices", () => {
    let empty_arr: [0]f64 = undefined;
    let empty: []f64 = empty_arr[0..0];
    let scr_arr: [20]f64 = [0.0; 20];
    let scr: []f64 = scr_arr[0..];

    // Empty state rejected everywhere.
    expect_eq(rk2MidpointStep(expRhs, 0.0, 0.1, *empty, *scr), false);
    expect_eq(rk4Step(expRhs, 0.0, 0.1, *empty, *scr), false);
    expect_eq(rk4Integrate(expRhs, 0.0, 1.0, 10, *empty, *scr), false);
    if (_ := rk45Integrate(expRhs, 0.0, 1.0, *empty, 1.0e-6, 1.0e-8, 100, *scr)) {
        expect_eq(true, false);
    }

    // Short scratch buffers rejected.
    let y2_arr: [2]f64 = [1.0, 0.0];
    let y2: []f64 = y2_arr[0..];
    let scr4_arr: [4]f64 = [0.0; 4];
    let scr4: []f64 = scr4_arr[0..];
    let scr9_arr: [9]f64 = [0.0; 9];
    let scr9: []f64 = scr9_arr[0..];
    let scr19_arr: [19]f64 = [0.0; 19];
    let scr19: []f64 = scr19_arr[0..];

    expect_eq(rk2MidpointStep(harmonicRhs, 0.0, 0.1, *y2, *scr4), false);
    expect_eq(rk4Step(harmonicRhs, 0.0, 0.1, *y2, *scr9), false);
    expect_eq(rk4Integrate(harmonicRhs, 0.0, 1.0, 0, *y2, *scr), false);
    if (_ := rk45Integrate(harmonicRhs, 0.0, 1.0, *y2, 1.0e-6, 1.0e-8, 100, *scr19)) {
        expect_eq(true, false);
    }
    expect_eq(rk45Step(harmonicRhs, 0.0, 0.1, y2, *y2, *y2, *scr9), false);

    // Invalid tolerances or max_steps == 0 rejected.
    if (_ := rk45Integrate(harmonicRhs, 0.0, 1.0, *y2, 0.0, 1.0e-8, 100, *scr)) { expect_eq(true, false); }
    if (_ := rk45Integrate(harmonicRhs, 0.0, 1.0, *y2, 1.0e-6, -1.0, 100, *scr)) { expect_eq(true, false); }
    if (_ := rk45Integrate(harmonicRhs, 0.0, 1.0, *y2, 1.0e-6, 1.0e-8, 0, *scr)) { expect_eq(true, false); }

    // Mismatched Velocity Verlet slice lengths rejected.
    let v1_arr: [1]f64 = [0.0];
    let v1: []f64 = v1_arr[0..];
    expect_eq(verletStep(harmonicAccel, 0.0, 0.1, *y2, *v1, *y2, *y2), false);
});
