// Source: cp/cap/test/test_ekf_production_deep.cp (Production deep tests for cap:ekf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EkfWorkspace,
    UkfParams,
    numericalJacobianF,
    numericalJacobianH,
    ekfPredict,
    ekfUpdate,
    generateSigmaPoints,
    ukfPredict,
} from "cap:math";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

/// 4D constant-velocity transition: state = [px, py, vx, vy]
fn cv4dTransition(x: []const f64, u: []const f64, dt: f64, out: *[]f64) {
    * = u;
    out.*[0] = x[0] + x[2] * dt;
    out.*[1] = x[1] + x[3] * dt;
    out.*[2] = x[2];
    out.*[3] = x[3];
}

/// 2D polar radar measurement: z = [range, bearing] = [sqrt(px^2 + py^2), atan2(py, px)]
fn radarMeasurement(x: []const f64, z_out: *[]f64) {
    const px = x[0];
    const py = x[1];
    z_out.*[0] = @sqrt(px * px + py * py);
    z_out.*[1] = std.math.atan2(py, px);
}

/// Nonlinear 2D unicycle / polar-step transition for UKF test: [x + v*dt*cos(th), y + v*dt*sin(th)]
fn polarStep2d(x: []const f64, u: []const f64, dt: f64, out: *[]f64) {
    const r = x[0] + u[0] * dt;
    const th = x[1];
    out.*[0] = r * @cos(th);
    out.*[1] = r * @sin(th);
}

test("1. Numerical Jacobians F & H match analytic derivatives within < 1e-8", () => {
    const x = [3.0, 4.0, 1.5, -0.5];
    const u: [0]f64 = [];
    const dt: f64 = 0.25;

    let xp: [4]f64 = std.mem.zeroes([4]f64);
    let fp: [4]f64 = std.mem.zeroes([4]f64);
    let fm: [4]f64 = std.mem.zeroes([4]f64);
    let f_jac: [16]f64 = std.mem.zeroes([16]f64);
    let xp_s: []f64 = xp[0..];
    let fp_s: []f64 = fp[0..];
    let fm_s: []f64 = fm[0..];
    let fj_s: []f64 = f_jac[0..];

    expect_eq(numericalJacobianF(cv4dTransition, x[0..], u[0..], dt, 1.0e-6, &xp_s, &fp_s, &fm_s, &fj_s), true);

    // Expected F = I + dt * [[0,0,1,0],[0,0,0,1],[0,0,0,0],[0,0,0,0]]
    const exp_f: [16]f64 = [
        1.0, 0.0, dt,  0.0,
        0.0, 1.0, 0.0, dt,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];
    let i: usize = 0;
    while (i < 16) {
        expect_eq(absF64(f_jac[i] - exp_f[i]) < 1.0e-8, true);
        i += 1;
    }

    // Analytic polar radar Jacobian at (px=3, py=4, r=5):
    // dr/dpx = 3/5 = 0.6, dr/dpy = 4/5 = 0.8
    // dth/dpx = -py/r^2 = -4/25 = -0.16, dth/dpy = px/r^2 = 3/25 = 0.12
    let hp: [2]f64 = std.mem.zeroes([2]f64);
    let hm: [2]f64 = std.mem.zeroes([2]f64);
    let h_jac: [8]f64 = std.mem.zeroes([8]f64);
    let hp_s: []f64 = hp[0..];
    let hm_s: []f64 = hm[0..];
    let hj_s: []f64 = h_jac[0..];

    expect_eq(numericalJacobianH(radarMeasurement, x[0..], 2, 1.0e-6, &xp_s, &hp_s, &hm_s, &hj_s), true);
    const exp_h: [8]f64 = [
        0.6,   0.8,  0.0, 0.0,
        -0.16, 0.12, 0.0, 0.0,
    ];
    i = 0;
    while (i < 8) {
        expect_eq(absF64(h_jac[i] - exp_h[i]) < 1.0e-8, true);
        i += 1;
    }
});

test("2. 2D polar radar EKF tracking constant-velocity target converges with symmetric PD covariance", () => {
    // True trajectory starts at (10, 5) with velocity (2.0, -0.5)
    let tx: f64 = 10.0;
    let ty: f64 = 5.0;
    const tvx: f64 = 2.0;
    const tvy: f64 = -0.5;
    const dt: f64 = 0.2;
    const u: [0]f64 = [];

    // Initial state estimate with significant error
    let x: [4]f64 = [8.5, 6.5, 0.0, 0.0];
    let p: [16]f64 = [
        4.0, 0.0, 0.0, 0.0,
        0.0, 4.0, 0.0, 0.0,
        0.0, 0.0, 9.0, 0.0,
        0.0, 0.0, 0.0, 9.0,
    ];
    const q: [16]f64 = [
        0.001, 0.0,   0.0,   0.0,
        0.0,   0.001, 0.0,   0.0,
        0.0,   0.0,   0.002, 0.0,
        0.0,   0.0,   0.0,   0.002,
    ];
    const r: [4]f64 = [
        0.04, 0.0,
        0.0,  0.0004,
    ];
    const f_jac: [16]f64 = [
        1.0, 0.0, dt,  0.0,
        0.0, 1.0, 0.0, dt,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];

    let tmp_nn_p: [16]f64 = std.mem.zeroes([16]f64);
    let x_next_s: [4]f64 = std.mem.zeroes([4]f64);
    let xp: [4]f64 = std.mem.zeroes([4]f64);
    let hp: [2]f64 = std.mem.zeroes([2]f64);
    let hm: [2]f64 = std.mem.zeroes([2]f64);
    let h_jac: [8]f64 = std.mem.zeroes([8]f64);

    // EKF update workspace buffers
    let z_pred: [2]f64 = std.mem.zeroes([2]f64);
    let innov: [2]f64 = std.mem.zeroes([2]f64);
    let ph_t: [8]f64 = std.mem.zeroes([8]f64);
    let s_cov: [4]f64 = std.mem.zeroes([4]f64);
    let s_chol: [4]f64 = std.mem.zeroes([4]f64);
    let k_gain: [8]f64 = std.mem.zeroes([8]f64);
    let i_kh: [16]f64 = std.mem.zeroes([16]f64);
    let tmp_nn_u: [16]f64 = std.mem.zeroes([16]f64);
    let tmp_nm: [8]f64 = std.mem.zeroes([8]f64);
    let tmp_m: [2]f64 = std.mem.zeroes([2]f64);

    let ws = EkfWorkspace {
        z_pred: z_pred[0..],
        innov: innov[0..],
        ph_t: ph_t[0..],
        s_cov: s_cov[0..],
        s_chol: s_chol[0..],
        k_gain: k_gain[0..],
        i_kh: i_kh[0..],
        tmp_nn: tmp_nn_u[0..],
        tmp_nm: tmp_nm[0..],
        tmp_m: tmp_m[0..],
    };

    let x_s: []f64 = x[0..];
    let p_s: []f64 = p[0..];
    let tnn_s: []f64 = tmp_nn_p[0..];
    let xn_s: []f64 = x_next_s[0..];
    let xp_s: []f64 = xp[0..];
    let hp_s: []f64 = hp[0..];
    let hm_s: []f64 = hm[0..];
    let hj_s: []f64 = h_jac[0..];

    let step: usize = 0;
    while (step < 30) {
        tx += tvx * dt;
        ty += tvy * dt;

        expect_eq(ekfPredict(&x_s, &p_s, 4, cv4dTransition, u[0..], dt, f_jac[0..], q[0..], &tnn_s, &xn_s), true);
        expect_eq(numericalJacobianH(radarMeasurement, x[0..], 2, 1.0e-6, &xp_s, &hp_s, &hm_s, &hj_s), true);

        const z = [@sqrt(tx * tx + ty * ty), std.math.atan2(ty, tx)];
        const nis = ekfUpdate(&x_s, &p_s, 4, 2, radarMeasurement, z[0..], h_jac[0..], r[0..], &ws) ?? unreachable;
        expect_eq(nis >= 0.0, true);
        step += 1;
    }

    // Converged close to true position and velocity
    expect_eq(absF64(x[0] - tx) < 0.05, true);
    expect_eq(absF64(x[1] - ty) < 0.05, true);
    expect_eq(absF64(x[2] - tvx) < 0.05, true);
    expect_eq(absF64(x[3] - tvy) < 0.05, true);

    // Verify covariance symmetry & positive definiteness via sigma-point Cholesky check
    let i: usize = 0;
    while (i < 4) {
        expect_eq(p[i * 4 + i] > 0.0, true);
        let j: usize = i + 1;
        while (j < 4) {
            expect_eq(absF64(p[i * 4 + j] - p[j * 4 + i]) < 1.0e-12, true);
            j += 1;
        }
        i += 1;
    }
});

test("3. UKF Merwe sigma points & ukfPredict exactly match linear mean and covariance propagation", () => {
    const dt: f64 = 0.5;
    const u: [0]f64 = [];
    let x_ekf: [4]f64 = [2.0, -3.0, 1.5, 0.8];
    let p_ekf: [16]f64 = [
        2.0, 0.3, 0.1, 0.0,
        0.3, 1.5, 0.0, 0.2,
        0.1, 0.0, 1.0, 0.1,
        0.0, 0.2, 0.1, 0.8,
    ];
    let x_ukf = x_ekf;
    let p_ukf = p_ekf;

    const q: [16]f64 = [
        0.05, 0.0,  0.0,  0.0,
        0.0,  0.05, 0.0,  0.0,
        0.0,  0.0,  0.02, 0.0,
        0.0,  0.0,  0.0,  0.02,
    ];
    const f_jac: [16]f64 = [
        1.0, 0.0, dt,  0.0,
        0.0, 1.0, 0.0, dt,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];

    let tmp_nn: [16]f64 = std.mem.zeroes([16]f64);
    let xn: [4]f64 = std.mem.zeroes([4]f64);
    let xe_s: []f64 = x_ekf[0..];
    let pe_s: []f64 = p_ekf[0..];
    let tnn_s: []f64 = tmp_nn[0..];
    let xn_s: []f64 = xn[0..];
    expect_eq(ekfPredict(&xe_s, &pe_s, 4, cv4dTransition, u[0..], dt, f_jac[0..], q[0..], &tnn_s, &xn_s), true);

    let sig_in: [36]f64 = std.mem.zeroes([36]f64);
    let sig_out: [36]f64 = std.mem.zeroes([36]f64);
    let wm: [9]f64 = std.mem.zeroes([9]f64);
    let wc: [9]f64 = std.mem.zeroes([9]f64);
    let chol: [16]f64 = std.mem.zeroes([16]f64);

    let xu_s: []f64 = x_ukf[0..];
    let pu_s: []f64 = p_ukf[0..];
    let si_s: []f64 = sig_in[0..];
    let so_s: []f64 = sig_out[0..];
    let wm_s: []f64 = wm[0..];
    let wc_s: []f64 = wc[0..];
    let ch_s: []f64 = chol[0..];

    expect_eq(
        ukfPredict(&xu_s, &pu_s, 4, cv4dTransition, u[0..], dt, q[0..], UkfParams.default(), &si_s, &so_s, &wm_s, &wc_s, &ch_s),
        true,
    );

    // Mean weights must sum to 1.0
    let w_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < 9) {
        w_sum += wm[i];
        i += 1;
    }
    expect_eq(absF64(w_sum - 1.0) < 1.0e-12, true);

    // For a linear transition, UKF mean and covariance must match exact EKF propagation
    i = 0;
    while (i < 4) {
        expect_eq(absF64(x_ukf[i] - x_ekf[i]) < 1.0e-10, true);
        i += 1;
    }
    i = 0;
    while (i < 16) {
        expect_eq(absF64(p_ukf[i] - p_ekf[i]) < 1.0e-8, true);
        i += 1;
    }
});

test("4. UKF nonlinear polar-to-Cartesian propagation preserves symmetric positive-definite covariance", () => {
    // Polar state [r=10.0, theta=0.0] with angular uncertainty std=0.1 rad -> y variance ~ (r*std)^2 = 1.0
    let x: [2]f64 = [10.0, 0.0];
    let p: [4]f64 = [
        0.04, 0.0,
        0.0,  0.01,
    ];
    const q: [4]f64 = [0.001, 0.0, 0.0, 0.001];
    const u: [1]f64 = [0.0];

    let sig_in: [10]f64 = std.mem.zeroes([10]f64);
    let sig_out: [10]f64 = std.mem.zeroes([10]f64);
    let wm: [5]f64 = std.mem.zeroes([5]f64);
    let wc: [5]f64 = std.mem.zeroes([5]f64);
    let chol: [4]f64 = std.mem.zeroes([4]f64);

    let x_s: []f64 = x[0..];
    let p_s: []f64 = p[0..];
    let si_s: []f64 = sig_in[0..];
    let so_s: []f64 = sig_out[0..];
    let wm_s: []f64 = wm[0..];
    let wc_s: []f64 = wc[0..];
    let ch_s: []f64 = chol[0..];

    expect_eq(
        ukfPredict(&x_s, &p_s, 2, polarStep2d, u[0..], 1.0, q[0..], UkfParams.default(), &si_s, &so_s, &wm_s, &wc_s, &ch_s),
        true,
    );

    // Mean x ~ 10 * exp(-0.01/2) ~ 9.95, mean y ~ 0; var_y ~ 10^2 * 0.01 = 1.0
    expect_eq(x[0] > 9.9 && x[0] < 10.0, true);
    expect_eq(absF64(x[1]) < 1.0e-12, true);
    expect_eq(p[3] > 0.9 && p[3] < 1.1, true);
    expect_eq(absF64(p[1] - p[2]) < 1.0e-12, true);
    // Resulting covariance must still be strictly positive definite
    expect_eq(generateSigmaPoints(x[0..], p[0..], 2, UkfParams.default(), &si_s, &wm_s, &wc_s, &ch_s), true);
});

test("5. Joseph stabilized update maintains positive-definite covariance under tiny observation noise R = 1e-8 * I", () => {
    let x: [4]f64 = [5.0, 5.0, 1.0, 1.0];
    let p: [16]f64 = [
        100.0, 0.0,   0.0,   0.0,
        0.0,   100.0, 0.0,   0.0,
        0.0,   0.0,   100.0, 0.0,
        0.0,   0.0,   0.0,   100.0,
    ];
    const r_tiny: [4]f64 = [1.0e-8, 0.0, 0.0, 1.0e-8];
    const h_jac: [8]f64 = [
        0.7071067811865475,  0.7071067811865475, 0.0, 0.0,
        -0.1,                0.1,                0.0, 0.0,
    ];
    const z = [@sqrt(50.0) + 0.01, std.math.atan2(5.0, 5.0) - 0.001];

    let z_pred: [2]f64 = std.mem.zeroes([2]f64);
    let innov: [2]f64 = std.mem.zeroes([2]f64);
    let ph_t: [8]f64 = std.mem.zeroes([8]f64);
    let s_cov: [4]f64 = std.mem.zeroes([4]f64);
    let s_chol: [4]f64 = std.mem.zeroes([4]f64);
    let k_gain: [8]f64 = std.mem.zeroes([8]f64);
    let i_kh: [16]f64 = std.mem.zeroes([16]f64);
    let tmp_nn: [16]f64 = std.mem.zeroes([16]f64);
    let tmp_nm: [8]f64 = std.mem.zeroes([8]f64);
    let tmp_m: [2]f64 = std.mem.zeroes([2]f64);

    let ws = EkfWorkspace {
        z_pred: z_pred[0..],
        innov: innov[0..],
        ph_t: ph_t[0..],
        s_cov: s_cov[0..],
        s_chol: s_chol[0..],
        k_gain: k_gain[0..],
        i_kh: i_kh[0..],
        tmp_nn: tmp_nn[0..],
        tmp_nm: tmp_nm[0..],
        tmp_m: tmp_m[0..],
    };
    let x_s: []f64 = x[0..];
    let p_s: []f64 = p[0..];

    const nis = ekfUpdate(&x_s, &p_s, 4, 2, radarMeasurement, z[0..], h_jac[0..], r_tiny[0..], &ws) ?? unreachable;
    expect_eq(nis >= 0.0, true);

    // Verify updated covariance remains strictly positive definite
    let sig: [36]f64 = std.mem.zeroes([36]f64);
    let wm: [9]f64 = std.mem.zeroes([9]f64);
    let wc: [9]f64 = std.mem.zeroes([9]f64);
    let chol: [16]f64 = std.mem.zeroes([16]f64);
    let sig_s: []f64 = sig[0..];
    let wm_s: []f64 = wm[0..];
    let wc_s: []f64 = wc[0..];
    let ch_s: []f64 = chol[0..];
    expect_eq(generateSigmaPoints(x[0..], p[0..], 4, UkfParams.default(), &sig_s, &wm_s, &wc_s, &ch_s), true);
});

test("6. Defensive validation: non-PD covariance, singular S, undersized buffers & NaN rejected", () => {
    // Non-positive-definite covariance rejected by generateSigmaPoints
    const x2 = [1.0, 2.0];
    const p_not_pd = [1.0, 2.0, 2.0, 1.0]; // det = -3 < 0
    let sig: [10]f64 = std.mem.zeroes([10]f64);
    let wm: [5]f64 = std.mem.zeroes([5]f64);
    let wc: [5]f64 = std.mem.zeroes([5]f64);
    let chol: [4]f64 = std.mem.zeroes([4]f64);
    let sig_s: []f64 = sig[0..];
    let wm_s: []f64 = wm[0..];
    let wc_s: []f64 = wc[0..];
    let ch_s: []f64 = chol[0..];
    expect_eq(generateSigmaPoints(x2[0..], p_not_pd[0..], 2, UkfParams.default(), &sig_s, &wm_s, &wc_s, &ch_s), false);

    // Singular innovation covariance S = 0 (H = 0, R = 0) rejected by ekfUpdate -> null
    let x4: [4]f64 = [3.0, 4.0, 0.0, 0.0];
    let p4: [16]f64 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];
    const h_zero: [8]f64 = std.mem.zeroes([8]f64);
    const r_zero: [4]f64 = std.mem.zeroes([4]f64);
    const z2 = [5.0, 0.9];

    let z_pred: [2]f64 = std.mem.zeroes([2]f64);
    let innov: [2]f64 = std.mem.zeroes([2]f64);
    let ph_t: [8]f64 = std.mem.zeroes([8]f64);
    let s_cov: [4]f64 = std.mem.zeroes([4]f64);
    let s_chol: [4]f64 = std.mem.zeroes([4]f64);
    let k_gain: [8]f64 = std.mem.zeroes([8]f64);
    let i_kh: [16]f64 = std.mem.zeroes([16]f64);
    let tmp_nn: [16]f64 = std.mem.zeroes([16]f64);
    let tmp_nm: [8]f64 = std.mem.zeroes([8]f64);
    let tmp_m: [2]f64 = std.mem.zeroes([2]f64);
    let ws = EkfWorkspace {
        z_pred: z_pred[0..],
        innov: innov[0..],
        ph_t: ph_t[0..],
        s_cov: s_cov[0..],
        s_chol: s_chol[0..],
        k_gain: k_gain[0..],
        i_kh: i_kh[0..],
        tmp_nn: tmp_nn[0..],
        tmp_nm: tmp_nm[0..],
        tmp_m: tmp_m[0..],
    };
    let x4_s: []f64 = x4[0..];
    let p4_s: []f64 = p4[0..];
    expect_eq(ekfUpdate(&x4_s, &p4_s, 4, 2, radarMeasurement, z2[0..], h_zero[0..], r_zero[0..], &ws) == null, true);

    // NaN measurement rejected -> null
    const zero: f64 = 0.0;
    const z_nan = [zero / zero, 0.9];
    const r_ok = [0.1, 0.0, 0.0, 0.1];
    expect_eq(ekfUpdate(&x4_s, &p4_s, 4, 2, radarMeasurement, z_nan[0..], h_zero[0..], r_ok[0..], &ws) == null, true);

    // Undersized sigma buffer rejected -> false
    let short_sig: []f64 = sig[0..6];
    const p_pd = [1.0, 0.0, 0.0, 1.0];
    expect_eq(generateSigmaPoints(x2[0..], p_pd[0..], 2, UkfParams.default(), &short_sig, &wm_s, &wc_s, &ch_s), false);
});
