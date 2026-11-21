// Source: cp/cap/test/test_kalman_production_deep.cp (Production deep tests for cap:kalman)

import { test, expect_eq } from "cap:test";
import {
    Kalman1D,
    Kalman2D,
    AlphaBetaFilter,
} from "cap:kalman";

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

test("1. Kalman1D converges rapidly to true constant state with much lower variance than raw noise", () => {
    // True constant value = 25.0, measurement noise +/- 4.0 (variance = 16.0)
    let kf = Kalman1D.init(0.0, 100.0, 0.001, 16.0) ?? unreachable;

    let raw_sq_err: f64 = 0.0;
    let filt_sq_err: f64 = 0.0;
    let i: usize = 0;
    while (i < 100) {
        const noise: f64 = if ((i % 2) == 0) 4.0 else -4.0;
        const z = 25.0 + noise;
        const est = kf.step(z);
        if (i >= 40) {
            raw_sq_err += noise * noise;
            const err = est - 25.0;
            filt_sq_err += err * err;
        }
        // Joseph-form variance stays strictly non-negative
        expect_eq(kf.p >= 0.0, true);
        i += 1;
    }

    // Converged close to 25.0 and filtered MSE is orders of magnitude lower than raw noise MSE
    expect_eq(absF64(kf.x - 25.0) < 0.6, true);
    expect_eq(filt_sq_err < raw_sq_err * 0.05, true);
    expect_eq(kf.last_gain > 0.0 && kf.last_gain < 1.0, true);

    // Control input prediction check
    const before = kf.x;
    kf.predict(2.0, 3.0);
    expect_eq(absF64(kf.x - (before + 6.0)) < 1e-12, true);
});

test("2. Kalman2D constant-velocity tracking (pos = 10 + 2.5*t) recovers both position & velocity", () => {
    let kf = Kalman2D.init(0.0, 0.0, 100.0, 25.0, 0.01, 0.5) ?? unreachable;
    const dt: f64 = 0.2;

    let step_idx: usize = 1;
    while (step_idx <= 120) {
        expect_eq(kf.predict(dt), true);
        const t = (step_idx as f64) * dt;
        const true_pos = 10.0 + 2.5 * t;
        const noise: f64 = if ((step_idx % 2) == 0) 0.4 else -0.4;
        * = kf.updatePosition(true_pos + noise);

        // Positive-semidefinite covariance matrix invariant at every step
        expect_eq(kf.p00 >= 0.0, true);
        expect_eq(kf.p11 >= 0.0, true);
        expect_eq(kf.p00 * kf.p11 >= kf.p01 * kf.p01 - 1e-12, true);

        step_idx += 1;
    }

    const final_t = 120.0 * dt;
    const expected_pos = 10.0 + 2.5 * final_t;
    expect_eq(absF64(kf.pos - expected_pos) < 0.3, true);
    expect_eq(absF64(kf.vel - 2.5) < 0.15, true);

    // Velocity sensor fusion refinement
    const v_fused = kf.updateVelocity(2.5, 0.05) ?? unreachable;
    expect_eq(absF64(v_fused - 2.5) < 0.08, true);
    expect_eq(kf.p00 >= 0.0 && kf.p11 >= 0.0 && kf.p00 * kf.p11 >= kf.p01 * kf.p01 - 1e-12, true);
});

test("3. Kalman2D mahalanobisSq outlier gating distinguishes normal vs spike measurements", () => {
    let kf = Kalman2D.init(50.0, 1.0, 1.0, 0.5, 0.05, 1.0) ?? unreachable;
    expect_eq(kf.predict(1.0), true); // predicted pos = 51.0

    const d2_normal = kf.mahalanobisSq(51.5);
    const d2_outlier = kf.mahalanobisSq(95.0);

    // Chi-squared 1-DOF 99.9% gate is ~10.83
    expect_eq(d2_normal < 1.0, true);
    expect_eq(d2_outlier > 100.0, true);
});

test("4. AlphaBetaFilter tracks constant-velocity target accurately", () => {
    let ab = AlphaBetaFilter.init(0.0, 0.0, 0.35, 0.1) ?? unreachable;
    const dt: f64 = 0.5;

    let k: usize = 1;
    while (k <= 80) {
        const t = (k as f64) * dt;
        const true_pos = 5.0 + 3.0 * t;
        const est = ab.update(true_pos, dt) ?? unreachable;
        if (k == 80) {
            expect_eq(absF64(est - true_pos) < 0.05, true);
            expect_eq(absF64(ab.vel - 3.0) < 0.05, true);
        }
        k += 1;
    }
});

test("5. Invalid parameters rejection (negative variance, zero meas_var, non-positive dt, invalid alpha/beta, NaN)", () => {
    const zero: f64 = 0.0;
    const nan_val: f64 = zero / zero;

    // Kalman1D invalid init
    expect_eq(Kalman1D.init(0.0, -1.0, 0.1, 1.0) == null, true);
    expect_eq(Kalman1D.init(0.0, 1.0, -0.1, 1.0) == null, true);
    expect_eq(Kalman1D.init(0.0, 1.0, 0.1, 0.0) == null, true);
    expect_eq(Kalman1D.init(0.0, 1.0, 0.1, -2.0) == null, true);
    expect_eq(Kalman1D.init(nan_val, 1.0, 0.1, 1.0) == null, true);

    // Kalman2D invalid init, dt & updateVelocity
    expect_eq(Kalman2D.init(0.0, 0.0, -1.0, 1.0, 0.1, 1.0) == null, true);
    expect_eq(Kalman2D.init(0.0, 0.0, 1.0, -1.0, 0.1, 1.0) == null, true);
    expect_eq(Kalman2D.init(0.0, 0.0, 1.0, 1.0, -0.1, 1.0) == null, true);
    expect_eq(Kalman2D.init(0.0, 0.0, 1.0, 1.0, 0.1, 0.0) == null, true);
    expect_eq(Kalman2D.init(nan_val, 0.0, 1.0, 1.0, 0.1, 1.0) == null, true);

    let kf2 = Kalman2D.init(0.0, 0.0, 1.0, 1.0, 0.1, 1.0) ?? unreachable;
    expect_eq(kf2.predict(0.0), false);
    expect_eq(kf2.predict(-0.5), false);
    expect_eq(kf2.predict(nan_val), false);
    expect_eq(kf2.updateVelocity(1.0, 0.0) == null, true);
    expect_eq(kf2.updateVelocity(1.0, -0.5) == null, true);
    expect_eq(kf2.updateVelocity(nan_val, 1.0) == null, true);

    // AlphaBetaFilter invalid init & update
    expect_eq(AlphaBetaFilter.init(0.0, 0.0, 0.0, 0.5) == null, true);
    expect_eq(AlphaBetaFilter.init(0.0, 0.0, 1.0, 0.5) == null, true);
    expect_eq(AlphaBetaFilter.init(0.0, 0.0, 0.5, 0.0) == null, true);
    expect_eq(AlphaBetaFilter.init(0.0, 0.0, 0.5, 2.5) == null, true);
    expect_eq(AlphaBetaFilter.init(nan_val, 0.0, 0.5, 0.5) == null, true);

    let ab = AlphaBetaFilter.init(0.0, 0.0, 0.5, 0.5) ?? unreachable;
    expect_eq(ab.update(1.0, 0.0) == null, true);
    expect_eq(ab.update(1.0, -1.0) == null, true);
    expect_eq(ab.update(nan_val, 0.5) == null, true);
});
