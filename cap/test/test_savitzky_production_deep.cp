// Source: cp/cap/test/test_savitzky_production_deep.cp (Production deep tests for cap:savitzky)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BoundaryMode,
    SavgolWorkspace,
    LoessConfig,
    savgolCoeffs,
    savgolFilter,
    loessSmooth1D,
} from "cap:savitzky";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn sumSlice(xs: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        s += xs[i];
        i += 1;
    }
    return s;
}

fn rmseSlice(a: []const f64, b: []const f64): f64 {
    let sq: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) {
        const d = a[i] - b[i];
        sq += d * d;
        i += 1;
    }
    return @sqrt(sq / (a.len as f64));
}

test("1. savgolCoeffs matches classic 5/7-point literature weights & sum invariants", () => {
    let ata: [16]f64 = [0.0; 16];
    let rhs: [4]f64 = [0.0; 4];
    let sol: [4]f64 = [0.0; 4];
    let piv: [4]usize = [0; 4];
    let ws: SavgolWorkspace = { ata: ata[0..], rhs: rhs[0..], sol: sol[0..], piv: piv[0..] };

    // 5-point quadratic/cubic smoothing: [-3, 12, 17, 12, -3] / 35
    let k5_buf: [5]f64 = [0.0; 5];
    let k5: []f64 = k5_buf[0..];
    expect_eq(savgolCoeffs(5, 2, 0, 1.0, *k5, *ws), true);
    const exp5: [5]f64 = [-3.0 / 35.0, 12.0 / 35.0, 17.0 / 35.0, 12.0 / 35.0, -3.0 / 35.0];
    let i: usize = 0;
    while (i < 5) {
        expect_eq(absF64(k5[i] - exp5[i]) < 1.0e-13, true);
        i += 1;
    }
    expect_eq(absF64(sumSlice(k5) - 1.0) < 1.0e-13, true);

    // 7-point quadratic smoothing: [-2, 3, 6, 7, 6, 3, -2] / 21
    let k7_buf: [7]f64 = [0.0; 7];
    let k7: []f64 = k7_buf[0..];
    expect_eq(savgolCoeffs(7, 2, 0, 1.0, *k7, *ws), true);
    const exp7: [7]f64 = [-2.0 / 21.0, 3.0 / 21.0, 6.0 / 21.0, 7.0 / 21.0, 6.0 / 21.0, 3.0 / 21.0, -2.0 / 21.0];
    let j: usize = 0;
    while (j < 7) {
        expect_eq(absF64(k7[j] - exp7[j]) < 1.0e-13, true);
        j += 1;
    }
    expect_eq(absF64(sumSlice(k7) - 1.0) < 1.0e-13, true);

    // 5-point quadratic 1st derivative: [-2, -1, 0, 1, 2] / 10 (sum == 0)
    expect_eq(savgolCoeffs(5, 2, 1, 1.0, *k5, *ws), true);
    const exp_d1: [5]f64 = [-0.2, -0.1, 0.0, 0.1, 0.2];
    let d1_i: usize = 0;
    while (d1_i < 5) {
        expect_eq(absF64(k5[d1_i] - exp_d1[d1_i]) < 1.0e-13, true);
        d1_i += 1;
    }
    expect_eq(absF64(sumSlice(k5)) < 1.0e-13, true);

    // 5-point quadratic 2nd derivative: [2, -1, -2, -1, 2] / 7 (sum == 0)
    expect_eq(savgolCoeffs(5, 2, 2, 1.0, *k5, *ws), true);
    const exp_d2: [5]f64 = [2.0 / 7.0, -1.0 / 7.0, -2.0 / 7.0, -1.0 / 7.0, 2.0 / 7.0];
    let d2_i: usize = 0;
    while (d2_i < 5) {
        expect_eq(absF64(k5[d2_i] - exp_d2[d2_i]) < 1.0e-13, true);
        d2_i += 1;
    }
    expect_eq(absF64(sumSlice(k5)) < 1.0e-13, true);
});

test("2. Cubic polynomial zero-distortion smoothing & exact 1st/2nd derivatives (< 1e-10)", () => {
    const dt: f64 = 0.25;
    let sig: [25]f64 = [0.0; 25];
    let i: usize = 0;
    while (i < 25) {
        const t = (i as f64) * dt;
        // y(t) = 0.5*t^3 - 2*t^2 + 3*t + 1
        sig[i] = 0.5 * t * t * t - 2.0 * t * t + 3.0 * t + 1.0;
        i += 1;
    }

    let ata: [16]f64 = [0.0; 16];
    let rhs: [4]f64 = [0.0; 4];
    let sol: [4]f64 = [0.0; 4];
    let piv: [4]usize = [0; 4];
    let ws: SavgolWorkspace = { ata: ata[0..], rhs: rhs[0..], sol: sol[0..], piv: piv[0..] };

    let k_buf: [7]f64 = [0.0; 7];
    let k: []f64 = k_buf[0..];
    let out_buf: [25]f64 = [0.0; 25];
    let out_s: []f64 = out_buf[0..];

    // 0th derivative (smoothing) preserves cubic polynomial exactly on interior
    expect_eq(savgolCoeffs(7, 3, 0, dt, *k, *ws), true);
    expect_eq(savgolFilter(sig[0..], k, BoundaryMode.Nearest, *out_s), true);
    let p: usize = 3;
    while (p < 22) {
        expect_eq(absF64(out_s[p] - sig[p]) < 1.0e-10, true);
        p += 1;
    }

    // 1st derivative: y'(t) = 1.5*t^2 - 4*t + 3
    expect_eq(savgolCoeffs(7, 3, 1, dt, *k, *ws), true);
    expect_eq(savgolFilter(sig[0..], k, BoundaryMode.Nearest, *out_s), true);
    let q: usize = 3;
    while (q < 22) {
        const t = (q as f64) * dt;
        const dy = 1.5 * t * t - 4.0 * t + 3.0;
        expect_eq(absF64(out_s[q] - dy) < 1.0e-10, true);
        q += 1;
    }

    // 2nd derivative: y''(t) = 3*t - 4
    expect_eq(savgolCoeffs(7, 3, 2, dt, *k, *ws), true);
    expect_eq(savgolFilter(sig[0..], k, BoundaryMode.Nearest, *out_s), true);
    let r: usize = 3;
    while (r < 22) {
        const t = (r as f64) * dt;
        const d2y = 3.0 * t - 4.0;
        expect_eq(absF64(out_s[r] - d2y) < 1.0e-10, true);
        r += 1;
    }
});

test("3. Noisy sine signal denoising & all BoundaryModes (Mirror, Nearest, ConstantZero)", () => {
    const n: usize = 64;
    let clean: [64]f64 = [0.0; 64];
    let noisy: [64]f64 = [0.0; 64];
    let i: usize = 0;
    while (i < n) {
        const x = (i as f64) * 0.1;
        const c = @sin(x);
        // Deterministic alternating high-frequency noise
        const noise = if ((i % 2) == 0) 0.25 else -0.25;
        clean[i] = c;
        noisy[i] = c + noise;
        i += 1;
    }

    let ata: [16]f64 = [0.0; 16];
    let rhs: [4]f64 = [0.0; 4];
    let sol: [4]f64 = [0.0; 4];
    let piv: [4]usize = [0; 4];
    let ws: SavgolWorkspace = { ata: ata[0..], rhs: rhs[0..], sol: sol[0..], piv: piv[0..] };

    let k_buf: [11]f64 = [0.0; 11];
    let k: []f64 = k_buf[0..];
    expect_eq(savgolCoeffs(11, 3, 0, 1.0, *k, *ws), true);

    let out_buf: [64]f64 = [0.0; 64];
    let out_s: []f64 = out_buf[0..];
    expect_eq(savgolFilter(noisy[0..], k, BoundaryMode.Mirror, *out_s), true);

    const raw_err = rmseSlice(noisy[5..59], clean[5..59]);
    const filt_err = rmseSlice(out_s[5..59], clean[5..59]);
    expect_eq(filt_err < raw_err * 0.35, true);

    // Exercise Nearest and ConstantZero modes as well
    expect_eq(savgolFilter(noisy[0..], k, BoundaryMode.Nearest, *out_s), true);
    expect_eq(savgolFilter(noisy[0..], k, BoundaryMode.ConstantZero, *out_s), true);
});

test("4. loessSmooth1D recovers smooth trend on non-uniformly sampled noisy curve (degree 1 & 2)", () => {
    const n: usize = 30;
    let xs: [30]f64 = [0.0; 30];
    let clean: [30]f64 = [0.0; 30];
    let noisy: [30]f64 = [0.0; 30];
    let i: usize = 0;
    while (i < n) {
        // Non-uniform spacing via mild quadratic warp
        const u = (i as f64) / ((n - 1) as f64);
        const x = 3.0 * (0.7 * u + 0.3 * u * u);
        xs[i] = x;
        clean[i] = @sin(x);
        const jitter = if ((i % 3) == 0) 0.18 else if ((i % 3) == 1) -0.15 else 0.04;
        noisy[i] = clean[i] + jitter;
        i += 1;
    }

    let y_out_buf: [30]f64 = [0.0; 30];
    let rob_buf: [30]f64 = [0.0; 30];
    let dist_buf: [30]f64 = [0.0; 30];
    let y_out: []f64 = y_out_buf[0..];
    let rob_w: []f64 = rob_buf[0..];
    let dist_s: []f64 = dist_buf[0..];

    const raw_rmse = rmseSlice(noisy[0..], clean[0..]);

    // Degree 1 (local linear)
    const cfg1: LoessConfig = { bandwidth: 0.35, robust_iters: 1, degree: 1 };
    expect_eq(loessSmooth1D(xs[0..], noisy[0..], cfg1, *y_out, *rob_w, *dist_s), true);
    const rmse1 = rmseSlice(y_out, clean[0..]);
    expect_eq(rmse1 < raw_rmse * 0.5, true);

    // Degree 2 (local quadratic)
    const cfg2: LoessConfig = { bandwidth: 0.45, robust_iters: 1, degree: 2 };
    expect_eq(loessSmooth1D(xs[0..], noisy[0..], cfg2, *y_out, *rob_w, *dist_s), true);
    const rmse2 = rmseSlice(y_out, clean[0..]);
    expect_eq(rmse2 < raw_rmse * 0.5, true);
});

test("5. loessSmooth1D robust bisquare iterations reject extreme single-point outlier", () => {
    const n: usize = 25;
    let xs: [25]f64 = [0.0; 25];
    let clean: [25]f64 = [0.0; 25];
    let ys: [25]f64 = [0.0; 25];
    let i: usize = 0;
    while (i < n) {
        const x = (i as f64) * 0.2;
        xs[i] = x;
        clean[i] = 0.5 * x * x - 1.2 * x + 2.0;
        ys[i] = clean[i];
        i += 1;
    }
    // Inject massive outlier at index 12
    ys[12] += 50.0;

    let y_out_buf: [25]f64 = [0.0; 25];
    let rob_buf: [25]f64 = [0.0; 25];
    let dist_buf: [25]f64 = [0.0; 25];
    let y_out: []f64 = y_out_buf[0..];
    let rob_w: []f64 = rob_buf[0..];
    let dist_s: []f64 = dist_buf[0..];

    const cfg: LoessConfig = { bandwidth: 0.4, robust_iters: 3, degree: 2 };
    expect_eq(loessSmooth1D(xs[0..], ys[0..], cfg, *y_out, *rob_w, *dist_s), true);

    // Outlier must be down-weighted to 0 and smoothed value must recover clean curve accurately
    expect_eq(rob_w[12] == 0.0, true);
    expect_eq(absF64(y_out[12] - clean[12]) < 0.05, true);
});

test("6. Boundary defense: even window, poly_order >= window_len, short buffers & NaN", () => {
    const nan = std.math.nan(f64);
    let ata: [16]f64 = [0.0; 16];
    let rhs: [4]f64 = [0.0; 4];
    let sol: [4]f64 = [0.0; 4];
    let piv: [4]usize = [0; 4];
    let short_f: [2]f64 = [0.0; 2];
    let ws: SavgolWorkspace = { ata: ata[0..], rhs: rhs[0..], sol: sol[0..], piv: piv[0..] };
    let ws_short: SavgolWorkspace = { ata: short_f[0..], rhs: rhs[0..], sol: sol[0..], piv: piv[0..] };

    let k_buf: [5]f64 = [0.0; 5];
    let k: []f64 = k_buf[0..];
    let k_short: []f64 = short_f[0..];

    // savgolCoeffs invalid parameters
    expect_eq(savgolCoeffs(4, 2, 0, 1.0, *k, *ws), false);      // even window
    expect_eq(savgolCoeffs(1, 0, 0, 1.0, *k, *ws), false);      // window < 3
    expect_eq(savgolCoeffs(5, 5, 0, 1.0, *k, *ws), false);      // poly_order >= window_len
    expect_eq(savgolCoeffs(5, 2, 3, 1.0, *k, *ws), false);      // deriv_order > poly_order
    expect_eq(savgolCoeffs(5, 2, 0, 0.0, *k, *ws), false);      // delta <= 0
    expect_eq(savgolCoeffs(5, 2, 0, nan, *k, *ws), false);      // NaN delta
    expect_eq(savgolCoeffs(5, 2, 0, 1.0, *k_short, *ws), false);// short output
    expect_eq(savgolCoeffs(5, 2, 0, 1.0, *k, *ws_short), false);// short workspace

    // savgolFilter invalid inputs
    const sig_ok: [5]f64 = [1.0, 2.0, 3.0, 4.0, 5.0];
    const sig_nan: [5]f64 = [1.0, nan, 3.0, 4.0, 5.0];
    const k_even: [4]f64 = [0.25; 4];
    let out_buf: [5]f64 = [0.0; 5];
    let out_s: []f64 = out_buf[0..];
    expect_eq(savgolCoeffs(5, 2, 0, 1.0, *k, *ws), true);
    expect_eq(savgolFilter(sig_ok[0..0], k, BoundaryMode.Mirror, *out_s), false);
    expect_eq(savgolFilter(sig_ok[0..], k_even[0..], BoundaryMode.Mirror, *out_s), false);
    expect_eq(savgolFilter(sig_nan[0..], k, BoundaryMode.Mirror, *out_s), false);
    expect_eq(savgolFilter(sig_ok[0..], k, BoundaryMode.Mirror, *k_short), false);

    // loessSmooth1D invalid inputs
    const xs_ok: [4]f64 = [0.0, 1.0, 2.0, 3.0];
    const ys_ok: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const ys_nan: [4]f64 = [1.0, nan, 3.0, 4.0];
    let y4_buf: [4]f64 = [0.0; 4];
    let r4_buf: [4]f64 = [0.0; 4];
    let d4_buf: [4]f64 = [0.0; 4];
    let y4: []f64 = y4_buf[0..];
    let r4: []f64 = r4_buf[0..];
    let d4: []f64 = d4_buf[0..];

    const cfg_ok = LoessConfig.default();
    const cfg_bad_bw: LoessConfig = { bandwidth: 0.0, robust_iters: 1, degree: 1 };
    const cfg_bad_deg: LoessConfig = { bandwidth: 0.5, robust_iters: 1, degree: 3 };
    expect_eq(loessSmooth1D(xs_ok[0..], ys_ok[0..], cfg_bad_bw, *y4, *r4, *d4), false);
    expect_eq(loessSmooth1D(xs_ok[0..], ys_ok[0..], cfg_bad_deg, *y4, *r4, *d4), false);
    expect_eq(loessSmooth1D(xs_ok[0..], ys_nan[0..], cfg_ok, *y4, *r4, *d4), false);
    expect_eq(loessSmooth1D(xs_ok[0..], ys_ok[0..], cfg_ok, *k_short, *r4, *d4), false);
    expect_eq(loessSmooth1D(xs_ok[0..1], ys_ok[0..1], cfg_ok, *y4, *r4, *d4), false);
});
