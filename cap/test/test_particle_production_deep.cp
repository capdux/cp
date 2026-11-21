// Source: cp/cap/test/test_particle_production_deep.cp (Production deep tests for cap:particle)

import { test, expect_eq } from "cap:test";
import {
    ResampleKind,
    PropagateFn,
    LogLikelihoodFn,
    normalizeLogWeights,
    effectiveSampleSize,
    resampleIndices,
    ParticleFilter,
} from "cap:particle";

fn absF64(v: f64): f64 {
    return if (v < 0.0) (0.0 - v) else v;
}

fn propNonlinear1D(st: *[]f64, u: []const f64, dt: f64, noise: []const f64) {
    * = dt;
    const x: f64 = st.*[0];
    st.*[0] = (0.5 * x) + ((25.0 * x) / (1.0 + (x * x))) + (8.0 * @cos(1.2 * u[0])) + noise[0];
}

fn llNonlinear1D(st: []const f64, z: []const f64): f64 {
    // Nonlinear observation y = x^2 / 20 + v, sigma_v = 1.0
    const pred_y: f64 = (st[0] * st[0]) / 20.0;
    const diff: f64 = z[0] - pred_y;
    return -0.5 * diff * diff;
}

fn propCv2D(st: *[]f64, u: []const f64, dt: f64, noise: []const f64) {
    * = u;
    // Constant velocity 2D model: [x, y, vx, vy]
    st.*[0] = st.*[0] + (st.*[2] * dt) + noise[0];
    st.*[1] = st.*[1] + (st.*[3] * dt) + noise[1];
    st.*[2] = st.*[2] + noise[2];
    st.*[3] = st.*[3] + noise[3];
}

fn llRange2D(st: []const f64, z: []const f64): f64 {
    // Two fixed range-only beacons at (0, 0) and (20, 0), sigma_r = 0.25
    const x: f64 = st[0];
    const y: f64 = st[1];
    const r0: f64 = @sqrt((x * x) + (y * y));
    const dx1: f64 = x - 20.0;
    const r1: f64 = @sqrt((dx1 * dx1) + (y * y));
    const e0: f64 = (z[0] - r0) / 0.25;
    const e1: f64 = (z[1] - r1) / 0.25;
    return -0.5 * ((e0 * e0) + (e1 * e1));
}

test("1. normalizeLogWeights extreme -1000 underflow immunity & ESS uniform/degenerate bounds", () => {
    const lw: [4]f64 = [-1000.0, -1000.0, -1000.0, -1000.0];
    let w_b: [4]f64 = [0.0; 4];
    let w: []f64 = w_b[0..];

    const log_z: f64 = normalizeLogWeights(lw[0..], *w) ?? unreachable;
    expect_eq(absF64(log_z - (-1000.0 + @log(4.0))) < 1.0e-12, true);
    expect_eq(absF64(w[0] - 0.25) < 1.0e-12, true);
    expect_eq(absF64(w[3] - 0.25) < 1.0e-12, true);
    expect_eq(absF64(effectiveSampleSize(w) - 4.0) < 1.0e-12, true);

    // Degenerate weights: single dominant particle => ESS == 1.0
    const deg_w: [4]f64 = [0.0, 1.0, 0.0, 0.0];
    expect_eq(absF64(effectiveSampleSize(deg_w[0..]) - 1.0) < 1.0e-12, true);

    // Relative log-weights shifted by -1000 preserve exact probabilities
    const lw2: [3]f64 = [-1000.0, -1000.0 + @log(3.0), -1000.0 + @log(6.0)];
    let w2_b: [3]f64 = [0.0; 3];
    let w2: []f64 = w2_b[0..];
    * = normalizeLogWeights(lw2[0..], *w2) ?? unreachable;
    expect_eq(absF64(w2[0] - 0.1) < 1.0e-12, true);
    expect_eq(absF64(w2[1] - 0.3) < 1.0e-12, true);
    expect_eq(absF64(w2[2] - 0.6) < 1.0e-12, true);
});

test("2. Systematic, Stratified & Multinomial resampling frequencies match target distribution", () => {
    // 1000 particles split into 4 blocks with total block weights 0.1, 0.2, 0.3, 0.4
    const n: usize = 1000;
    let w: [1000]f64 = [0.0; 1000];
    let i: usize = 0;
    while (i < n) {
        if (i < 250) {
            w[i] = 0.1 / 250.0;
        } else if (i < 500) {
            w[i] = 0.2 / 250.0;
        } else if (i < 750) {
            w[i] = 0.3 / 250.0;
        } else {
            w[i] = 0.4 / 250.0;
        }
        i += 1;
    }

    let idx_b: [1000]usize = [0; 1000];
    let idx: []usize = idx_b[0..];
    const kinds: [3]ResampleKind = [ResampleKind.Systematic, ResampleKind.Stratified, ResampleKind.Multinomial];
    const tols: [3]f64 = [0.005, 0.01, 0.06];

    let k: usize = 0;
    while (k < 3) {
        expect_eq(resampleIndices(w[0..], kinds[k], 42 + (k as u64), *idx), true);
        let counts: [4]usize = [0; 4];
        let j: usize = 0;
        while (j < n) {
            const p: usize = idx[j];
            expect_eq(p < n, true);
            counts[p / 250] += 1;
            j += 1;
        }
        expect_eq(absF64(((counts[0] as f64) / 1000.0) - 0.1) < tols[k], true);
        expect_eq(absF64(((counts[1] as f64) / 1000.0) - 0.2) < tols[k], true);
        expect_eq(absF64(((counts[2] as f64) / 1000.0) - 0.3) < tols[k], true);
        expect_eq(absF64(((counts[3] as f64) / 1000.0) - 0.4) < tols[k], true);
        k += 1;
    }
});

test("3. Closed-loop 2D nonlinear dual-beacon range tracking & 1D nonlinear SMC benchmark", () => {
    // (a) 2D nonlinear range-only tracking of a moving target with 300 particles
    const n: usize = 300;
    const dim: usize = 4;
    let p_b: [1200]f64 = [0.0; 1200];
    let w_b: [300]f64 = [0.0; 300];
    let tmp_p_b: [1200]f64 = [0.0; 1200];
    let lw_b: [300]f64 = [0.0; 300];
    let idx_b: [300]usize = [0; 300];
    let noise_b: [4]f64 = [0.0; 4];

    let pts: []f64 = p_b[0..];
    let wts: []f64 = w_b[0..];
    let tmp_p: []f64 = tmp_p_b[0..];
    let lw: []f64 = lw_b[0..];
    let idx: []usize = idx_b[0..];
    let noise: []f64 = noise_b[0..];

    // True trajectory starts at (2.0, 5.0) moving with velocity (0.8, 0.4)
    const mean0: [4]f64 = [2.5, 4.5, 0.7, 0.5];
    const std0: [4]f64 = [1.5, 1.5, 0.5, 0.5];
    let pf: ParticleFilter = ParticleFilter.initGaussian(*pts, *wts, n, mean0[0..], std0[0..], 2026) ?? unreachable;

    let true_x: f64 = 2.0;
    let true_y: f64 = 5.0;
    const true_vx: f64 = 0.8;
    const true_vy: f64 = 0.4;
    const pstd: [4]f64 = [0.08, 0.08, 0.04, 0.04];
    const u_empty: [0]f64 = [];
    let resample_count: usize = 0;

    let step: usize = 0;
    while (step < 15) {
        true_x += true_vx;
        true_y += true_vy;

        expect_eq(pf.predict(propCv2D, u_empty[0..], 1.0, pstd[0..], *noise), true);

        const r0: f64 = @sqrt((true_x * true_x) + (true_y * true_y));
        const dx1: f64 = true_x - 20.0;
        const r1: f64 = @sqrt((dx1 * dx1) + (true_y * true_y));
        const z: [2]f64 = [r0, r1];

        const log_ml: f64 = pf.update(llRange2D, z[0..], *lw) ?? unreachable;
        expect_eq(log_ml == log_ml, true);

        if (pf.resampleIfNeeded(0.5, ResampleKind.Systematic, *idx, *tmp_p)) {
            resample_count += 1;
        }
        step += 1;
    }

    expect_eq(resample_count >= 1, true);

    let est_m_b: [4]f64 = [0.0; 4];
    let est_c_b: [16]f64 = [0.0; 16];
    let est_m: []f64 = est_m_b[0..];
    let est_c: []f64 = est_c_b[0..];
    expect_eq(pf.estimateMean(*est_m), true);
    expect_eq(pf.estimateCovariance(est_m, *est_c), true);

    // Position error < 0.35 after 15 steps of nonlinear range-only tracking
    const pos_err: f64 = @sqrt(((est_m[0] - true_x) * (est_m[0] - true_x)) + ((est_m[1] - true_y) * (est_m[1] - true_y)));
    expect_eq(pos_err < 0.35, true);
    // Covariance matrix is symmetric with positive diagonal
    expect_eq(est_c[0] > 0.0 && est_c[5] > 0.0, true);
    expect_eq(absF64(est_c[1] - est_c[4]) < 1.0e-12, true);

    // (b) 1D classic nonlinear benchmark step sanity check
    let p1_b: [100]f64 = [0.0; 100];
    let w1_b: [100]f64 = [0.0; 100];
    let lw1_b: [100]f64 = [0.0; 100];
    let n1_b: [1]f64 = [0.0];
    let p1: []f64 = p1_b[0..];
    let w1: []f64 = w1_b[0..];
    let lw1: []f64 = lw1_b[0..];
    let n1: []f64 = n1_b[0..];
    const lo1: [1]f64 = [-5.0];
    const hi1: [1]f64 = [5.0];
    let pf1: ParticleFilter = ParticleFilter.initUniform(*p1, *w1, 100, lo1[0..], hi1[0..], 7) ?? unreachable;
    const ctrl1: [1]f64 = [1.0];
    const ps1: [1]f64 = [1.0];
    const z1: [1]f64 = [2.0];
    expect_eq(pf1.predict(propNonlinear1D, ctrl1[0..], 1.0, ps1[0..], *n1), true);
    expect_eq(pf1.update(llNonlinear1D, z1[0..], *lw1) != null, true);
});

test("4. Bit-exact deterministic reproducibility with identical seed", () => {
    const n: usize = 64;
    let p1_b: [256]f64 = [0.0; 256];
    let w1_b: [64]f64 = [0.0; 64];
    let p2_b: [256]f64 = [0.0; 256];
    let w2_b: [64]f64 = [0.0; 64];
    let tmp_b: [256]f64 = [0.0; 256];
    let lw_b: [64]f64 = [0.0; 64];
    let idx_b: [64]usize = [0; 64];
    let ns_b: [4]f64 = [0.0; 4];

    let p1: []f64 = p1_b[0..];
    let w1: []f64 = w1_b[0..];
    let p2: []f64 = p2_b[0..];
    let w2: []f64 = w2_b[0..];
    let tmp: []f64 = tmp_b[0..];
    let lw: []f64 = lw_b[0..];
    let idx: []usize = idx_b[0..];
    let ns: []f64 = ns_b[0..];

    const m0: [4]f64 = [1.0, 2.0, 0.5, 0.5];
    const s0: [4]f64 = [0.5, 0.5, 0.2, 0.2];
    const ps: [4]f64 = [0.1, 0.1, 0.05, 0.05];
    const ctrl0: [0]f64 = [];
    const z0: [2]f64 = [3.0, 18.0];

    let pf1: ParticleFilter = ParticleFilter.initGaussian(*p1, *w1, n, m0[0..], s0[0..], 12345) ?? unreachable;
    let pf2: ParticleFilter = ParticleFilter.initGaussian(*p2, *w2, n, m0[0..], s0[0..], 12345) ?? unreachable;

    expect_eq(pf1.predict(propCv2D, ctrl0[0..], 1.0, ps[0..], *ns), true);
    expect_eq(pf2.predict(propCv2D, ctrl0[0..], 1.0, ps[0..], *ns), true);

    const ml1: f64 = pf1.update(llRange2D, z0[0..], *lw) ?? unreachable;
    const ml2: f64 = pf2.update(llRange2D, z0[0..], *lw) ?? unreachable;
    expect_eq(ml1, ml2);

    const r1: bool = pf1.resampleIfNeeded(0.99, ResampleKind.Stratified, *idx, *tmp);
    const r2: bool = pf2.resampleIfNeeded(0.99, ResampleKind.Stratified, *idx, *tmp);
    expect_eq(r1, r2);

    let k: usize = 0;
    while (k < 256) {
        expect_eq(p1[k], p2[k]);
        k += 1;
    }
});

test("5. Boundary defense: zero particles/dim, negative/zero weights, short buffers & NaN/Inf", () => {
    const z: f64 = 0.0;
    const nan_v: f64 = z / z;
    const inf_v: f64 = 1.0 / z;

    let p_b: [8]f64 = [0.0; 8];
    let w_b: [4]f64 = [0.0; 4];
    let idx_b: [4]usize = [0; 4];
    let pts: []f64 = p_b[0..];
    let wts: []f64 = w_b[0..];
    let idx: []usize = idx_b[0..];

    const lo: [2]f64 = [0.0, 0.0];
    const hi: [2]f64 = [1.0, 1.0];
    const hi_bad: [2]f64 = [-1.0, 1.0];
    const sd_bad: [2]f64 = [-0.1, 1.0];
    const empty_f: [0]f64 = [];

    // Zero particles / zero dim / inverted bounds / negative std / short buffers
    expect_eq(ParticleFilter.initUniform(*pts, *wts, 0, lo[0..], hi[0..], 1) == null, true);
    expect_eq(ParticleFilter.initUniform(*pts, *wts, 4, empty_f[0..], empty_f[0..], 1) == null, true);
    expect_eq(ParticleFilter.initUniform(*pts, *wts, 4, lo[0..], hi_bad[0..], 1) == null, true);
    expect_eq(ParticleFilter.initGaussian(*pts, *wts, 4, lo[0..], sd_bad[0..], 1) == null, true);
    // 5 particles * 2 dim = 10 > p_b.len (8)
    expect_eq(ParticleFilter.initUniform(*pts, *wts, 5, lo[0..], hi[0..], 1) == null, true);

    // normalizeLogWeights & resampleIndices invalid inputs
    const lw_nan: [2]f64 = [0.0, nan_v];
    const lw_inf: [2]f64 = [0.0, inf_v];
    const lw_neg_inf: [2]f64 = [-inf_v, -inf_v];
    expect_eq(normalizeLogWeights(empty_f[0..], *wts) == null, true);
    expect_eq(normalizeLogWeights(lw_nan[0..], *wts) == null, true);
    expect_eq(normalizeLogWeights(lw_inf[0..], *wts) == null, true);
    expect_eq(normalizeLogWeights(lw_neg_inf[0..], *wts) == null, true);

    const w_neg: [2]f64 = [-0.2, 1.2];
    const w_zero: [2]f64 = [0.0, 0.0];
    expect_eq(resampleIndices(w_neg[0..], ResampleKind.Systematic, 1, *idx), false);
    expect_eq(resampleIndices(w_zero[0..], ResampleKind.Systematic, 1, *idx), false);
    expect_eq(effectiveSampleSize(empty_f[0..]), 0.0);
    expect_eq(effectiveSampleSize(w_neg[0..]), 0.0);
});
