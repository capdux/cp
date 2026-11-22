// Origin: Pure-CP Stdlib (Zero-Alloc ODE Solvers: RK2, RK4, Dormand-Prince RK4(5) & Velocity Verlet) | Created: 2026-11-22

import std from "std";

/// Right-hand side derivative callback `dy/dt = f(t, y)` for first-order ODE systems.
pub type OdeRhsFn = fn(t: f64, y: []const f64, dydt: *[]f64);

/// Acceleration callback `a = acc(t, pos)` for second-order Hamiltonian / Newtonian systems.
pub type AccelFn = fn(t: f64, pos: []const f64, acc: *[]f64);

/// Diagnostics returned by adaptive Dormand-Prince RK4(5) integration.
pub struct Rk45Result {
    steps_accepted: u32,
    steps_rejected: u32,
    final_dt: f64,
    reached_end: bool,
}

fn absF64(v: f64): f64 {
    if (v < 0.0) {
        return 0.0 - v;
    }
    return v;
}

fn maxF64(a: f64, b: f64): f64 {
    if (a >= b) {
        return a;
    }
    return b;
}

/// Advances state `y` by one explicit 2nd-order Midpoint (RK2) step of size `dt`.
/// Requires `y.len >= 1` and `scratch.len >= 3 * y.len`.
pub fn rk2MidpointStep(
    f: OdeRhsFn,
    t: f64,
    dt: f64,
    y: *[]f64,
    scratch: *[]f64,
): bool {
    const n: usize = y.*.len;
    if (n == 0 || scratch.*.len < (3 as usize) * n) {
        return false;
    }

    let k1: []f64 = scratch.*[0..n];
    let k2: []f64 = scratch.*[n..((2 as usize) * n)];
    let ytmp: []f64 = scratch.*[((2 as usize) * n)..((3 as usize) * n)];

    f(t, y.*, *k1);

    const half_dt: f64 = 0.5 * dt;
    let i: usize = 0;
    while (i < n) {
        ytmp[i] = y.*[i] + half_dt * k1[i];
        i += 1;
    }

    f(t + half_dt, ytmp, *k2);

    i = 0;
    while (i < n) {
        y.*[i] += dt * k2[i];
        i += 1;
    }
    return true;
}

/// Advances state `y` in-place by one classical 4th-order Runge-Kutta (RK4) step of size `dt`.
/// Requires `y.len >= 1` and `scratch.len >= 5 * y.len`.
pub fn rk4Step(
    f: OdeRhsFn,
    t: f64,
    dt: f64,
    y: *[]f64,
    scratch: *[]f64,
): bool {
    const n: usize = y.*.len;
    if (n == 0 || scratch.*.len < (5 as usize) * n) {
        return false;
    }

    let k1: []f64 = scratch.*[0..n];
    let k2: []f64 = scratch.*[n..((2 as usize) * n)];
    let k3: []f64 = scratch.*[((2 as usize) * n)..((3 as usize) * n)];
    let k4: []f64 = scratch.*[((3 as usize) * n)..((4 as usize) * n)];
    let ytmp: []f64 = scratch.*[((4 as usize) * n)..((5 as usize) * n)];

    const half_dt: f64 = 0.5 * dt;

    f(t, y.*, *k1);

    let i: usize = 0;
    while (i < n) {
        ytmp[i] = y.*[i] + half_dt * k1[i];
        i += 1;
    }
    f(t + half_dt, ytmp, *k2);

    i = 0;
    while (i < n) {
        ytmp[i] = y.*[i] + half_dt * k2[i];
        i += 1;
    }
    f(t + half_dt, ytmp, *k3);

    i = 0;
    while (i < n) {
        ytmp[i] = y.*[i] + dt * k3[i];
        i += 1;
    }
    f(t + dt, ytmp, *k4);

    const dt6: f64 = dt / 6.0;
    i = 0;
    while (i < n) {
        y.*[i] += dt6 * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
        i += 1;
    }
    return true;
}

/// Integrates `y` from `t0` to `t1` using `steps` uniform RK4 steps (`steps >= 1`).
/// Requires `y.len >= 1` and `scratch.len >= 5 * y.len`.
pub fn rk4Integrate(
    f: OdeRhsFn,
    t0: f64,
    t1: f64,
    steps: usize,
    y: *[]f64,
    scratch: *[]f64,
): bool {
    const n: usize = y.*.len;
    if (n == 0 || steps == 0 || scratch.*.len < (5 as usize) * n) {
        return false;
    }

    const dt: f64 = (t1 - t0) / (steps as f64);
    let s: usize = 0;
    while (s < steps) {
        const t: f64 = t0 + (s as f64) * dt;
        if (!rk4Step(f, t, dt, y, scratch)) {
            return false;
        }
        s += 1;
    }
    return true;
}

// Dormand-Prince RK4(5) Butcher tableau coefficients.
const DP_C2: f64 = 1.0 / 5.0;
const DP_C3: f64 = 3.0 / 10.0;
const DP_C4: f64 = 4.0 / 5.0;
const DP_C5: f64 = 8.0 / 9.0;

const DP_A21: f64 = 1.0 / 5.0;
const DP_A31: f64 = 3.0 / 40.0;
const DP_A32: f64 = 9.0 / 40.0;
const DP_A41: f64 = 44.0 / 45.0;
const DP_A42: f64 = -56.0 / 15.0;
const DP_A43: f64 = 32.0 / 9.0;
const DP_A51: f64 = 19372.0 / 6561.0;
const DP_A52: f64 = -25360.0 / 2187.0;
const DP_A53: f64 = 64448.0 / 6561.0;
const DP_A54: f64 = -212.0 / 729.0;
const DP_A61: f64 = 9017.0 / 3168.0;
const DP_A62: f64 = -355.0 / 33.0;
const DP_A63: f64 = 46732.0 / 5247.0;
const DP_A64: f64 = 49.0 / 176.0;
const DP_A65: f64 = -5103.0 / 18656.0;

// 5th-order solution weights (b_i).
const DP_B1: f64 = 35.0 / 384.0;
const DP_B3: f64 = 500.0 / 1113.0;
const DP_B4: f64 = 125.0 / 192.0;
const DP_B5: f64 = -2187.0 / 6784.0;
const DP_B6: f64 = 11.0 / 84.0;

// Embedded error weights (e_i = b_i - b_hat_i).
const DP_E1: f64 = 71.0 / 57600.0;
const DP_E3: f64 = -71.0 / 16695.0;
const DP_E4: f64 = 71.0 / 1920.0;
const DP_E5: f64 = -17253.0 / 339200.0;
const DP_E6: f64 = 22.0 / 525.0;
const DP_E7: f64 = -1.0 / 40.0;

/// Computes one Dormand-Prince RK4(5) trial step of size `dt` from `(t, y)`, writing the 5th-order
/// candidate state into `y_out` and the 5th-minus-4th order error estimate into `err_out`.
/// Requires `y.len >= 1`, `y_out.len >= y.len`, `err_out.len >= y.len`, and `scratch.len >= 8 * y.len`.
pub fn rk45Step(
    f: OdeRhsFn,
    t: f64,
    dt: f64,
    y: []const f64,
    y_out: *[]f64,
    err_out: *[]f64,
    scratch: *[]f64,
): bool {
    const n: usize = y.len;
    if (n == 0 || y_out.*.len < n || err_out.*.len < n || scratch.*.len < (8 as usize) * n) {
        return false;
    }

    let k1: []f64 = scratch.*[0..n];
    let k2: []f64 = scratch.*[n..((2 as usize) * n)];
    let k3: []f64 = scratch.*[((2 as usize) * n)..((3 as usize) * n)];
    let k4: []f64 = scratch.*[((3 as usize) * n)..((4 as usize) * n)];
    let k5: []f64 = scratch.*[((4 as usize) * n)..((5 as usize) * n)];
    let k6: []f64 = scratch.*[((5 as usize) * n)..((6 as usize) * n)];
    let k7: []f64 = scratch.*[((6 as usize) * n)..((7 as usize) * n)];
    let ytmp: []f64 = scratch.*[((7 as usize) * n)..((8 as usize) * n)];

    // Stage 1
    f(t, y, *k1);

    // Stage 2
    let i: usize = 0;
    while (i < n) {
        ytmp[i] = y[i] + dt * (DP_A21 * k1[i]);
        i += 1;
    }
    f(t + DP_C2 * dt, ytmp, *k2);

    // Stage 3
    i = 0;
    while (i < n) {
        ytmp[i] = y[i] + dt * (DP_A31 * k1[i] + DP_A32 * k2[i]);
        i += 1;
    }
    f(t + DP_C3 * dt, ytmp, *k3);

    // Stage 4
    i = 0;
    while (i < n) {
        ytmp[i] = y[i] + dt * (DP_A41 * k1[i] + DP_A42 * k2[i] + DP_A43 * k3[i]);
        i += 1;
    }
    f(t + DP_C4 * dt, ytmp, *k4);

    // Stage 5
    i = 0;
    while (i < n) {
        ytmp[i] = y[i] + dt * (DP_A51 * k1[i] + DP_A52 * k2[i] + DP_A53 * k3[i] + DP_A54 * k4[i]);
        i += 1;
    }
    f(t + DP_C5 * dt, ytmp, *k5);

    // Stage 6
    i = 0;
    while (i < n) {
        ytmp[i] = y[i] + dt * (DP_A61 * k1[i] + DP_A62 * k2[i] + DP_A63 * k3[i] + DP_A64 * k4[i] + DP_A65 * k5[i]);
        i += 1;
    }
    f(t + dt, ytmp, *k6);

    // 5th-order candidate y_out
    i = 0;
    while (i < n) {
        y_out.*[i] = y[i] + dt * (DP_B1 * k1[i] + DP_B3 * k3[i] + DP_B4 * k4[i] + DP_B5 * k5[i] + DP_B6 * k6[i]);
        i += 1;
    }

    // Stage 7 (FSAL evaluation at y_out)
    f(t + dt, y_out.*, *k7);

    // Embedded error vector
    i = 0;
    while (i < n) {
        err_out.*[i] = dt * (DP_E1 * k1[i] + DP_E3 * k3[i] + DP_E4 * k4[i] + DP_E5 * k5[i] + DP_E6 * k6[i] + DP_E7 * k7[i]);
        i += 1;
    }
    return true;
}

/// Adaptive-step Dormand-Prince RK4(5) integrator from `t0` to `t1` (supports forward and backward integration).
/// Requires `y.len >= 1`, `rtol > 0`, `atol > 0`, `max_steps >= 1`, and `scratch.len >= 10 * y.len`.
pub fn rk45Integrate(
    f: OdeRhsFn,
    t0: f64,
    t1: f64,
    y: *[]f64,
    rtol: f64,
    atol: f64,
    max_steps: u32,
    scratch: *[]f64,
): Rk45Result? {
    const n: usize = y.*.len;
    if (n == 0 || rtol <= 0.0 || atol <= 0.0 || max_steps == 0 || scratch.*.len < (10 as usize) * n) {
        return null;
    }

    const span: f64 = t1 - t0;
    if (span == 0.0) {
        return Rk45Result {
            steps_accepted: 0,
            steps_rejected: 0,
            final_dt: 0.0,
            reached_end: true,
        };
    }

    let step_scratch: []f64 = scratch.*[0..((8 as usize) * n)];
    let y_cand: []f64 = scratch.*[((8 as usize) * n)..((9 as usize) * n)];
    let err_vec: []f64 = scratch.*[((9 as usize) * n)..((10 as usize) * n)];

    let dir: f64 = 1.0;
    if (span < 0.0) {
        dir = -1.0;
    }

    let t: f64 = t0;
    let dt: f64 = span * 0.05;
    if (absF64(dt) > 0.1) {
        dt = dir * 0.1;
    }

    let accepted: u32 = 0;
    let rejected: u32 = 0;
    let total_iters: u32 = 0;

    while (total_iters < max_steps) {
        const rem: f64 = t1 - t;
        if (absF64(rem) <= 1.0e-15 * maxF64(1.0, absF64(t1))) {
            return Rk45Result {
                steps_accepted: accepted,
                steps_rejected: rejected,
                final_dt: dt,
                reached_end: true,
            };
        }

        let h: f64 = dt;
        let is_final: bool = false;
        if (absF64(h) >= absF64(rem)) {
            h = rem;
            is_final = true;
        }

        if (!rk45Step(f, t, h, y.*, *y_cand, *err_vec, *step_scratch)) {
            return null;
        }
        total_iters += 1;

        // Hairer-Norsett-Wanner scaled RMS error norm.
        let sum_sq: f64 = 0.0;
        let i: usize = 0;
        while (i < n) {
            const sc: f64 = atol + rtol * maxF64(absF64(y.*[i]), absF64(y_cand[i]));
            const ratio: f64 = err_vec[i] / sc;
            sum_sq += ratio * ratio;
            i += 1;
        }
        const err_norm: f64 = std.math.sqrt(sum_sq / (n as f64));

        if (err_norm <= 1.0) {
            // Accept step.
            t += h;
            i = 0;
            while (i < n) {
                y.*[i] = y_cand[i];
                i += 1;
            }
            accepted += 1;

            if (is_final) {
                return Rk45Result {
                    steps_accepted: accepted,
                    steps_rejected: rejected,
                    final_dt: h,
                    reached_end: true,
                };
            }

            let grow: f64 = 5.0;
            if (err_norm > 0.0) {
                grow = 0.9 * std.math.pow(f64, err_norm, -0.2);
                if (grow > 5.0) {
                    grow = 5.0;
                }
                if (grow < 0.2) {
                    grow = 0.2;
                }
            }
            dt = h * grow;
        } else {
            // Reject step and shrink step size.
            rejected += 1;
            let shrink: f64 = 0.9 * std.math.pow(f64, err_norm, -0.2);
            if (shrink < 0.1) {
                shrink = 0.1;
            }
            if (shrink > 0.5) {
                shrink = 0.5;
            }
            dt = h * shrink;
            if (absF64(dt) <= 1.0e-15 * maxF64(1.0, absF64(t))) {
                break;
            }
        }
    }

    return Rk45Result {
        steps_accepted: accepted,
        steps_rejected: rejected,
        final_dt: dt,
        reached_end: false,
    };
}

/// Symplectic Velocity Verlet step for second-order Hamiltonian systems `x'' = acc(t, x)`.
/// Updates `pos`, `vel`, and rolls `acc_curr` forward to `acc_next` for the subsequent step.
/// Requires all 4 slices to share the same non-zero length.
pub fn verletStep(
    acc_fn: AccelFn,
    t: f64,
    dt: f64,
    pos: *[]f64,
    vel: *[]f64,
    acc_curr: *[]f64,
    acc_next: *[]f64,
): bool {
    const n: usize = pos.*.len;
    if (n == 0 || vel.*.len != n || acc_curr.*.len != n || acc_next.*.len != n) {
        return false;
    }

    const half_dt: f64 = 0.5 * dt;
    const half_dt2: f64 = 0.5 * dt * dt;

    let i: usize = 0;
    while (i < n) {
        pos.*[i] += vel.*[i] * dt + acc_curr.*[i] * half_dt2;
        i += 1;
    }

    acc_fn(t + dt, pos.*, acc_next);

    i = 0;
    while (i < n) {
        vel.*[i] += (acc_curr.*[i] + acc_next.*[i]) * half_dt;
        acc_curr.*[i] = acc_next.*[i];
        i += 1;
    }
    return true;
}
