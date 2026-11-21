// Source: cp/cap/test/test_nmf_production_deep.cp (Production deep tests for cap:nmf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NmfLoss,
    NmfConfig,
    NmfWorkspace,
    nmfFit,
    nmfTransform,
    nmfReconstruct,
    nmfFrobeniusError,
    nmfKlDivergence,
} from "cap:nmf";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn matFrobNorm(a: []const f64): f64 {
    let sq: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) {
        sq += a[i] * a[i];
        i += 1;
    }
    return @sqrt(sq);
}

fn makeSyntheticLowRank(v_out: *[]f64) {
    // W_true (6 x 2), H_true (2 x 5) -> V = W_true * H_true (6 x 5, exact rank 2)
    const w_true: [12]f64 = [
        2.0, 0.2,
        1.5, 0.5,
        0.3, 2.2,
        0.1, 1.8,
        1.2, 1.4,
        2.5, 0.8,
    ];
    const h_true: [10]f64 = [
        1.5, 0.2, 2.0, 0.4, 1.1,
        0.1, 1.8, 0.3, 2.1, 0.9,
    ];
    let ok = nmfReconstruct(w_true[0..], h_true[0..], 6, 5, 2, v_out);
    expect_eq(ok, true);
}

test("1. Exact low-rank V = W_true * H_true Frobenius NMF (< 1e-3 rel error) & strict non-negativity", () => {
    let v_buf: [30]f64 = [0.0; 30];
    let v_s: []f64 = v_buf[0..];
    makeSyntheticLowRank(*v_s);
    const v_norm = matFrobNorm(v_s);

    let w_buf: [12]f64 = [0.0; 12];
    let h_buf: [10]f64 = [0.0; 10];
    let num_buf: [12]f64 = [0.0; 12];
    let den_buf: [12]f64 = [0.0; 12];
    let wh_buf: [30]f64 = [0.0; 30];
    let rr_buf: [4]f64 = [0.0; 4];

    let w_s: []f64 = w_buf[0..];
    let h_s: []f64 = h_buf[0..];
    let ws: NmfWorkspace = {
        num_buf: num_buf[0..],
        den_buf: den_buf[0..],
        wh_buf: wh_buf[0..],
        rr_buf: rr_buf[0..],
    };

    const cfg: NmfConfig = {
        loss: NmfLoss.Frobenius,
        max_iter: 2000,
        tol: 1.0e-10,
        eps: 1.0e-12,
        seed: 42,
    };

    if (res := nmfFit(v_s, 6, 5, 2, cfg, *w_s, *h_s, *ws)) {
        expect_eq(res.iterations > 0, true);
        expect_eq(res.frobenius_error / v_norm < 1.0e-3, true);

        // Strict non-negativity of W and H
        let i: usize = 0;
        while (i < 12) {
            expect_eq(w_s[i] >= 0.0, true);
            i += 1;
        }
        let j: usize = 0;
        while (j < 10) {
            expect_eq(h_s[j] >= 0.0, true);
            j += 1;
        }

        // Cross-check nmfFrobeniusError & nmfReconstruct
        const err_helper = nmfFrobeniusError(v_s, w_s, h_s, 6, 5, 2);
        expect_eq(absF64(err_helper - res.frobenius_error) < 1.0e-12, true);

        let v_hat_buf: [30]f64 = [0.0; 30];
        let v_hat: []f64 = v_hat_buf[0..];
        expect_eq(nmfReconstruct(w_s, h_s, 6, 5, 2, *v_hat), true);
        let sq: f64 = 0.0;
        let k: usize = 0;
        while (k < 30) {
            const d = v_s[k] - v_hat[k];
            sq += d * d;
            k += 1;
        }
        expect_eq(absF64(@sqrt(sq) - res.frobenius_error) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }
});

test("2. Exact low-rank KlDivergence NMF (< 1e-3 rel error), KL helper match & non-negativity", () => {
    let v_buf: [30]f64 = [0.0; 30];
    let v_s: []f64 = v_buf[0..];
    makeSyntheticLowRank(*v_s);
    const v_norm = matFrobNorm(v_s);

    let w_buf: [12]f64 = [0.0; 12];
    let h_buf: [10]f64 = [0.0; 10];
    let num_buf: [12]f64 = [0.0; 12];
    let den_buf: [12]f64 = [0.0; 12];
    let wh_buf: [30]f64 = [0.0; 30];
    let rr_buf: [4]f64 = [0.0; 4];

    let w_s: []f64 = w_buf[0..];
    let h_s: []f64 = h_buf[0..];
    let ws: NmfWorkspace = {
        num_buf: num_buf[0..],
        den_buf: den_buf[0..],
        wh_buf: wh_buf[0..],
        rr_buf: rr_buf[0..],
    };

    const cfg: NmfConfig = {
        loss: NmfLoss.KlDivergence,
        max_iter: 2000,
        tol: 1.0e-10,
        eps: 1.0e-12,
        seed: 1337,
    };

    if (res := nmfFit(v_s, 6, 5, 2, cfg, *w_s, *h_s, *ws)) {
        expect_eq(res.frobenius_error / v_norm < 1.0e-3, true);
        expect_eq(res.loss < 1.0e-4, true);

        const kl_helper = nmfKlDivergence(v_s, w_s, h_s, 6, 5, 2, cfg.eps);
        expect_eq(absF64(kl_helper - res.loss) < 1.0e-11, true);

        let i: usize = 0;
        while (i < 12) {
            expect_eq(w_s[i] >= 0.0, true);
            i += 1;
        }
        let j: usize = 0;
        while (j < 10) {
            expect_eq(h_s[j] >= 0.0, true);
            j += 1;
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. Lee-Seung multiplicative updates guarantee monotonic non-increasing loss (Frobenius & KL)", () => {
    let v_buf: [30]f64 = [0.0; 30];
    let v_s: []f64 = v_buf[0..];
    makeSyntheticLowRank(*v_s);

    const steps: [6]usize = [1, 2, 5, 15, 50, 200];

    let w_buf: [12]f64 = [0.0; 12];
    let h_buf: [10]f64 = [0.0; 10];
    let num_buf: [12]f64 = [0.0; 12];
    let den_buf: [12]f64 = [0.0; 12];
    let wh_buf: [30]f64 = [0.0; 30];
    let rr_buf: [4]f64 = [0.0; 4];

    let w_s: []f64 = w_buf[0..];
    let h_s: []f64 = h_buf[0..];
    let ws: NmfWorkspace = {
        num_buf: num_buf[0..],
        den_buf: den_buf[0..],
        wh_buf: wh_buf[0..],
        rr_buf: rr_buf[0..],
    };

    // Check Frobenius monotonicity
    let prev_frob: f64 = 1.0e300;
    let i: usize = 0;
    while (i < 6) {
        const cfg: NmfConfig = {
            loss: NmfLoss.Frobenius,
            max_iter: steps[i],
            tol: 0.0,
            eps: 1.0e-12,
            seed: 99,
        };
        if (res := nmfFit(v_s, 6, 5, 2, cfg, *w_s, *h_s, *ws)) {
            expect_eq(res.loss <= prev_frob + 1.0e-12, true);
            prev_frob = res.loss;
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }

    // Check KL divergence monotonicity
    let prev_kl: f64 = 1.0e300;
    let j: usize = 0;
    while (j < 6) {
        const cfg: NmfConfig = {
            loss: NmfLoss.KlDivergence,
            max_iter: steps[j],
            tol: 0.0,
            eps: 1.0e-12,
            seed: 99,
        };
        if (res := nmfFit(v_s, 6, 5, 2, cfg, *w_s, *h_s, *ws)) {
            expect_eq(res.loss <= prev_kl + 1.0e-12, true);
            prev_kl = res.loss;
        } else {
            expect_eq(true, false);
        }
        j += 1;
    }
});

test("4. nmfTransform projects new samples onto fixed basis H with < 1e-3 relative reconstruction error", () => {
    const h_fixed: [10]f64 = [
        1.5, 0.2, 2.0, 0.4, 1.1,
        0.1, 1.8, 0.3, 2.1, 0.9,
    ];
    const w_new_true: [6]f64 = [
        0.8, 1.9,
        2.3, 0.4,
        1.1, 1.1,
    ];
    let v_new_buf: [15]f64 = [0.0; 15];
    let v_new: []f64 = v_new_buf[0..];
    expect_eq(nmfReconstruct(w_new_true[0..], h_fixed[0..], 3, 5, 2, *v_new), true);
    const v_new_norm = matFrobNorm(v_new);

    let w_proj_buf: [6]f64 = [0.0; 6];
    let num_buf: [6]f64 = [0.0; 6];
    let den_buf: [6]f64 = [0.0; 6];
    let wh_buf: [15]f64 = [0.0; 15];
    let rr_buf: [4]f64 = [0.0; 4];

    let w_proj: []f64 = w_proj_buf[0..];
    let ws: NmfWorkspace = {
        num_buf: num_buf[0..],
        den_buf: den_buf[0..],
        wh_buf: wh_buf[0..],
        rr_buf: rr_buf[0..],
    };

    // Frobenius transform
    const cfg_f: NmfConfig = {
        loss: NmfLoss.Frobenius,
        max_iter: 1500,
        tol: 1.0e-10,
        eps: 1.0e-12,
        seed: 7,
    };
    if (res_f := nmfTransform(v_new, 3, 5, 2, h_fixed[0..], cfg_f, *w_proj, *ws)) {
        expect_eq(res_f.frobenius_error / v_new_norm < 1.0e-3, true);
        let k: usize = 0;
        while (k < 6) {
            expect_eq(w_proj[k] >= 0.0, true);
            k += 1;
        }
    } else {
        expect_eq(true, false);
    }

    // KL divergence transform
    const cfg_kl: NmfConfig = {
        loss: NmfLoss.KlDivergence,
        max_iter: 1500,
        tol: 1.0e-10,
        eps: 1.0e-12,
        seed: 7,
    };
    if (res_kl := nmfTransform(v_new, 3, 5, 2, h_fixed[0..], cfg_kl, *w_proj, *ws)) {
        expect_eq(res_kl.frobenius_error / v_new_norm < 1.0e-3, true);
        let k: usize = 0;
        while (k < 6) {
            expect_eq(w_proj[k] >= 0.0, true);
            k += 1;
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Deterministic reproduction with identical seed & zero-matrix fast convergence", () => {
    let v_buf: [30]f64 = [0.0; 30];
    let v_s: []f64 = v_buf[0..];
    makeSyntheticLowRank(*v_s);

    let w1_buf: [12]f64 = [0.0; 12];
    let h1_buf: [10]f64 = [0.0; 10];
    let w2_buf: [12]f64 = [0.0; 12];
    let h2_buf: [10]f64 = [0.0; 10];
    let num_buf: [12]f64 = [0.0; 12];
    let den_buf: [12]f64 = [0.0; 12];
    let wh_buf: [30]f64 = [0.0; 30];
    let rr_buf: [4]f64 = [0.0; 4];

    let w1: []f64 = w1_buf[0..];
    let h1: []f64 = h1_buf[0..];
    let w2: []f64 = w2_buf[0..];
    let h2: []f64 = h2_buf[0..];
    let ws: NmfWorkspace = {
        num_buf: num_buf[0..],
        den_buf: den_buf[0..],
        wh_buf: wh_buf[0..],
        rr_buf: rr_buf[0..],
    };

    const cfg: NmfConfig = {
        loss: NmfLoss.Frobenius,
        max_iter: 120,
        tol: 1.0e-9,
        eps: 1.0e-12,
        seed: 0x123456789ABCDEF0,
    };

    const r1 = nmfFit(v_s, 6, 5, 2, cfg, *w1, *h1, *ws);
    const r2 = nmfFit(v_s, 6, 5, 2, cfg, *w2, *h2, *ws);
    expect_eq(r1 != null, true);
    expect_eq(r2 != null, true);
    if (a := r1) {
        if (b := r2) {
            expect_eq(a.iterations, b.iterations);
            expect_eq(a.loss, b.loss);
            expect_eq(a.frobenius_error, b.frobenius_error);
        }
    }
    let i: usize = 0;
    while (i < 12) {
        expect_eq(w1[i], w2[i]);
        i += 1;
    }
    let j: usize = 0;
    while (j < 10) {
        expect_eq(h1[j], h2[j]);
        j += 1;
    }

    // All-zero input matrix converges immediately with zero error
    const zeros: [6]f64 = [0.0; 6];
    let wz_buf: [4]f64 = [1.0; 4];
    let hz_buf: [6]f64 = [1.0; 6];
    let wz: []f64 = wz_buf[0..];
    let hz: []f64 = hz_buf[0..];
    if (rz := nmfFit(zeros[0..], 2, 3, 2, NmfConfig.default(), *wz, *hz, *ws)) {
        expect_eq(rz.converged, true);
        expect_eq(rz.frobenius_error, 0.0);
        expect_eq(wz[0], 0.0);
        expect_eq(hz[0], 0.0);
    } else {
        expect_eq(true, false);
    }
});

test("6. Boundary defense: negative entries, r == 0, short workspace buffers, NaN/Inf & invalid config", () => {
    const nan = std.math.nan(f64);
    const inf = std.math.inf(f64);
    const cfg = NmfConfig.default();

    const v_ok: [6]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    const v_neg: [6]f64 = [1.0, -0.5, 3.0, 4.0, 5.0, 6.0];
    const v_nan: [6]f64 = [1.0, nan, 3.0, 4.0, 5.0, 6.0];
    const v_inf: [6]f64 = [1.0, inf, 3.0, 4.0, 5.0, 6.0];

    let w_buf: [4]f64 = [0.0; 4];
    let h_buf: [6]f64 = [0.0; 6];
    let num_buf: [6]f64 = [0.0; 6];
    let den_buf: [6]f64 = [0.0; 6];
    let wh_buf: [6]f64 = [0.0; 6];
    let rr_buf: [4]f64 = [0.0; 4];
    let short_buf: [2]f64 = [0.0; 2];

    let w_s: []f64 = w_buf[0..];
    let h_s: []f64 = h_buf[0..];
    let ws_ok: NmfWorkspace = {
        num_buf: num_buf[0..],
        den_buf: den_buf[0..],
        wh_buf: wh_buf[0..],
        rr_buf: rr_buf[0..],
    };

    // Negative / NaN / Inf entries
    expect_eq(nmfFit(v_neg[0..], 2, 3, 2, cfg, *w_s, *h_s, *ws_ok) == null, true);
    expect_eq(nmfFit(v_nan[0..], 2, 3, 2, cfg, *w_s, *h_s, *ws_ok) == null, true);
    expect_eq(nmfFit(v_inf[0..], 2, 3, 2, cfg, *w_s, *h_s, *ws_ok) == null, true);

    // Zero dimensions (r == 0, m == 0, n == 0)
    expect_eq(nmfFit(v_ok[0..], 2, 3, 0, cfg, *w_s, *h_s, *ws_ok) == null, true);
    expect_eq(nmfFit(v_ok[0..], 0, 3, 2, cfg, *w_s, *h_s, *ws_ok) == null, true);
    expect_eq(nmfFit(v_ok[0..], 2, 0, 2, cfg, *w_s, *h_s, *ws_ok) == null, true);

    // Short workspace buffers
    let ws_short_num: NmfWorkspace = { num_buf: short_buf[0..], den_buf: den_buf[0..], wh_buf: wh_buf[0..], rr_buf: rr_buf[0..] };
    let ws_short_wh: NmfWorkspace = { num_buf: num_buf[0..], den_buf: den_buf[0..], wh_buf: short_buf[0..], rr_buf: rr_buf[0..] };
    let ws_short_rr: NmfWorkspace = { num_buf: num_buf[0..], den_buf: den_buf[0..], wh_buf: wh_buf[0..], rr_buf: short_buf[0..] };
    expect_eq(nmfFit(v_ok[0..], 2, 3, 2, cfg, *w_s, *h_s, *ws_short_num) == null, true);
    expect_eq(nmfFit(v_ok[0..], 2, 3, 2, cfg, *w_s, *h_s, *ws_short_wh) == null, true);
    expect_eq(nmfFit(v_ok[0..], 2, 3, 2, cfg, *w_s, *h_s, *ws_short_rr) == null, true);

    // Invalid configs
    const bad_iter: NmfConfig = { loss: NmfLoss.Frobenius, max_iter: 0, tol: 1.0e-6, eps: 1.0e-12, seed: 1 };
    const bad_eps: NmfConfig = { loss: NmfLoss.Frobenius, max_iter: 10, tol: 1.0e-6, eps: 0.0, seed: 1 };
    const bad_tol: NmfConfig = { loss: NmfLoss.Frobenius, max_iter: 10, tol: -1.0, eps: 1.0e-12, seed: 1 };
    expect_eq(nmfFit(v_ok[0..], 2, 3, 2, bad_iter, *w_s, *h_s, *ws_ok) == null, true);
    expect_eq(nmfFit(v_ok[0..], 2, 3, 2, bad_eps, *w_s, *h_s, *ws_ok) == null, true);
    expect_eq(nmfFit(v_ok[0..], 2, 3, 2, bad_tol, *w_s, *h_s, *ws_ok) == null, true);

    // nmfTransform negative fixed basis & helper NaN/false returns
    expect_eq(nmfTransform(v_ok[0..], 2, 3, 2, v_neg[0..], cfg, *w_s, *ws_ok) == null, true);
    let out_short: []f64 = short_buf[0..];
    expect_eq(nmfReconstruct(w_s, h_s, 2, 3, 2, *out_short), false);
    expect_eq(std.math.isNan(nmfFrobeniusError(v_ok[0..], w_s, h_s, 2, 3, 0)), true);
    expect_eq(std.math.isNan(nmfKlDivergence(v_neg[0..], w_s, h_s, 2, 3, 2, 1.0e-12)), true);
    expect_eq(std.math.isNan(nmfKlDivergence(v_ok[0..], w_s, h_s, 2, 3, 2, 0.0)), true);
});
