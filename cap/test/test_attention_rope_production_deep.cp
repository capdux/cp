// Origin: Pure-CP Stdlib Test Suite (Transformer Kernels: RoPE, RMSNorm, LayerNorm & SwiGLU/GeLU) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/rope.cp
// Covers:
//   1. RoPE exact L2 norm preservation & relative position invariance dot(R(m)q, R(n)k) == dot(R(m-n)q, k) (< 1e-12)
//   2. NTK-Aware long-context frequency scaling & norm/rel-pos preservation under ntk_scale = 4.0
//   3. rmsNorm (unit RMS, empty/custom weight, in-place) & layerNorm (zero mean, unit variance, affine gamma/beta)
//   4. silu, geluTanh (exact symmetry identity) & swigluForward analytic precision
//   5. Odd head_dim, out-of-range pos, negative eps, buffer underflow & NaN/Inf boundary defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    RopeConfig,
    ropePrecomputeTables,
    ropeApplyInPlace,
    rmsNorm,
    layerNorm,
    silu,
    geluTanh,
    swigluForward,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn dotF64(a: []const f64, b: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        s += a[i] * b[i];
    }
    return s;
}

fn normF64(a: []const f64): f64 {
    return std.math.sqrt(dotF64(a, a));
}

fn testRopeNormAndRelativePositionInvariance()! {
    const cfg = RopeConfig.default(8);
    let cos_buf: [256]f64 = [0.0; 256]; // 64 * 4
    let sin_buf: [256]f64 = [0.0; 256];
    let cos_s: []f64 = cos_buf[0..];
    let sin_s: []f64 = sin_buf[0..];
    expect_eq(ropePrecomputeTables(cfg, 64, cos_s, sin_s), true);

    const q_orig: [8]f64 = [1.2, -0.7, 0.5, 2.1, -1.3, 0.8, 0.4, -0.9];
    const k_orig: [8]f64 = [-0.6, 1.5, -1.1, 0.3, 0.9, -1.4, 2.0, 0.7];

    // 1. Position 0 is exact identity
    let q0_buf: [8]f64 = q_orig;
    let q0: []f64 = q0_buf[0..];
    expect_eq(ropeApplyInPlace(q0, 0, 8, cos_buf[0..], sin_buf[0..]), true);
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        expect_eq(approxF64(q0_buf[i], q_orig[i], EPS_F64), true);
    }

    // 2. Apply R(m=27) to q and R(n=11) to k
    let qm_buf: [8]f64 = q_orig;
    let kn_buf: [8]f64 = k_orig;
    let qm: []f64 = qm_buf[0..];
    let kn: []f64 = kn_buf[0..];
    expect_eq(ropeApplyInPlace(qm, 27, 8, cos_buf[0..], sin_buf[0..]), true);
    expect_eq(ropeApplyInPlace(kn, 11, 8, cos_buf[0..], sin_buf[0..]), true);

    // Exact L2 norm preservation
    expect_eq(approxF64(normF64(qm_buf[0..]), normF64(q_orig[0..]), EPS_F64), true);
    expect_eq(approxF64(normF64(kn_buf[0..]), normF64(k_orig[0..]), EPS_F64), true);

    // Core relative position invariance: dot(R(27)q, R(11)k) == dot(R(16)q, k)
    let q_rel_buf: [8]f64 = q_orig;
    let q_rel: []f64 = q_rel_buf[0..];
    expect_eq(ropeApplyInPlace(q_rel, 16, 8, cos_buf[0..], sin_buf[0..]), true);

    const dot_mn = dotF64(qm_buf[0..], kn_buf[0..]);
    const dot_rel = dotF64(q_rel_buf[0..], k_orig[0..]);
    expect_eq(approxF64(dot_mn, dot_rel, EPS_F64), true);
}

fn testRopeNtkScaling()! {
    const cfg_std = RopeConfig.default(8);
    const cfg_ntk = RopeConfig{ head_dim: 8, base_theta: 10000.0, ntk_scale: 4.0 };

    let cos_std_buf: [64]f64 = [0.0; 64]; // 16 * 4
    let sin_std_buf: [64]f64 = [0.0; 64];
    let cos_ntk_buf: [64]f64 = [0.0; 64];
    let sin_ntk_buf: [64]f64 = [0.0; 64];
    let cos_std: []f64 = cos_std_buf[0..];
    let sin_std: []f64 = sin_std_buf[0..];
    let cos_ntk: []f64 = cos_ntk_buf[0..];
    let sin_ntk: []f64 = sin_ntk_buf[0..];

    expect_eq(ropePrecomputeTables(cfg_std, 16, cos_std, sin_std), true);
    expect_eq(ropePrecomputeTables(cfg_ntk, 16, cos_ntk, sin_ntk), true);

    // At pos = 10: j = 0 has exp = 0 (unchanged); j = 1 has slower rotation under NTK scaling
    expect_eq(approxF64(cos_std_buf[10 * 4 + 0], cos_ntk_buf[10 * 4 + 0], EPS_F64), true);
    expect_eq(@abs(cos_std_buf[10 * 4 + 1] - cos_ntk_buf[10 * 4 + 1]) > 0.0001, true);

    // NTK tables still strictly preserve norm and relative position invariance
    const q: [8]f64 = [0.5, -1.2, 0.8, 0.3, -0.4, 1.1, -0.7, 0.9];
    const k: [8]f64 = [1.0, 0.4, -0.6, 1.3, 0.2, -0.8, 0.5, -1.1];
    let q9_buf: [8]f64 = q;
    let k4_buf: [8]f64 = k;
    let q5_buf: [8]f64 = q;
    let q9: []f64 = q9_buf[0..];
    let k4: []f64 = k4_buf[0..];
    let q5: []f64 = q5_buf[0..];
    expect_eq(ropeApplyInPlace(q9, 9, 8, cos_ntk_buf[0..], sin_ntk_buf[0..]), true);
    expect_eq(ropeApplyInPlace(k4, 4, 8, cos_ntk_buf[0..], sin_ntk_buf[0..]), true);
    expect_eq(ropeApplyInPlace(q5, 5, 8, cos_ntk_buf[0..], sin_ntk_buf[0..]), true);
    expect_eq(approxF64(dotF64(q9_buf[0..], k4_buf[0..]), dotF64(q5_buf[0..], k[0..]), EPS_F64), true);
}

fn testRmsNormAndLayerNorm()! {
    const x: [4]f64 = [1.0, -3.0, 5.0, -7.0];
    const empty_w: []const f64 = x[0..0];

    // 1. rmsNorm with empty weight -> exact unit RMS
    let rms_buf: [4]f64 = [0.0; 4];
    let rms_out: []f64 = rms_buf[0..];
    expect_eq(rmsNorm(x[0..], empty_w, 0.0, rms_out), true);
    const mean_sq = dotF64(rms_buf[0..], rms_buf[0..]) / 4.0;
    expect_eq(approxF64(std.math.sqrt(mean_sq), 1.0, EPS_F64), true);

    // rmsNorm with custom weight & in-place safety
    const w: [4]f64 = [2.0, 2.0, 2.0, 2.0];
    let inplace_buf: [4]f64 = x;
    let inplace_s: []f64 = inplace_buf[0..];
    expect_eq(rmsNorm(inplace_buf[0..], w[0..], 0.0, inplace_s), true);
    expect_eq(approxF64(inplace_buf[0], rms_buf[0] * 2.0, EPS_F64), true);
    expect_eq(approxF64(inplace_buf[3], rms_buf[3] * 2.0, EPS_F64), true);

    // 2. layerNorm with empty gamma/beta -> zero mean, unit variance
    let ln_buf: [4]f64 = [0.0; 4];
    let ln_out: []f64 = ln_buf[0..];
    expect_eq(layerNorm(x[0..], empty_w, empty_w, 0.0, ln_out), true);
    const mean = (ln_buf[0] + ln_buf[1] + ln_buf[2] + ln_buf[3]) / 4.0;
    const var_val = dotF64(ln_buf[0..], ln_buf[0..]) / 4.0;
    expect_eq(approxF64(mean, 0.0, EPS_F64), true);
    expect_eq(approxF64(var_val, 1.0, EPS_F64), true);

    // layerNorm with gamma=2, beta=3 -> mean=3, variance=4
    const gamma: [4]f64 = [2.0, 2.0, 2.0, 2.0];
    const beta: [4]f64 = [3.0, 3.0, 3.0, 3.0];
    expect_eq(layerNorm(x[0..], gamma[0..], beta[0..], 0.0, ln_out), true);
    const mean_aff = (ln_buf[0] + ln_buf[1] + ln_buf[2] + ln_buf[3]) / 4.0;
    expect_eq(approxF64(mean_aff, 3.0, EPS_F64), true);
}

fn testSiluGeluAndSwiglu()! {
    // SiLU analytic checks + large negative stability
    expect_eq(approxF64(silu(0.0), 0.0, EPS_F64), true);
    const silu_1 = 1.0 / (1.0 + std.math.exp(-1.0));
    expect_eq(approxF64(silu(1.0), silu_1, EPS_F64), true);
    const silu_m2 = -2.0 / (1.0 + std.math.exp(2.0));
    expect_eq(approxF64(silu(-2.0), silu_m2, EPS_F64), true);
    expect_eq(approxF64(silu(-100.0), 0.0, EPS_F64), true);

    // GeLU(tanh) exact checks & odd-difference identity gelu(x) - gelu(-x) == x
    expect_eq(approxF64(geluTanh(0.0), 0.0, EPS_F64), true);
    expect_eq(approxF64(geluTanh(1.0) - geluTanh(-1.0), 1.0, EPS_F64), true);
    expect_eq(approxF64(geluTanh(2.5) - geluTanh(-2.5), 2.5, EPS_F64), true);
    expect_eq(@abs(geluTanh(1.0) - 0.84119199) < 0.00001, true);

    // SwiGLU forward
    const gate: [4]f64 = [-2.0, 0.0, 1.0, 3.0];
    const up: [4]f64 = [4.0, 5.0, -2.0, 0.5];
    let out_buf: [4]f64 = [0.0; 4];
    let out: []f64 = out_buf[0..];
    expect_eq(swigluForward(gate[0..], up[0..], out), true);
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        expect_eq(approxF64(out_buf[i], silu(gate[i]) * up[i], EPS_F64), true);
    }
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_v = std.math.nan(f64);
    const inf_v = std.math.inf(f64);

    let c_buf: [8]f64 = [0.0; 8];
    let s_buf: [8]f64 = [0.0; 8];
    let c_s: []f64 = c_buf[0..];
    let s_s: []f64 = s_buf[0..];
    let c_small: []f64 = c_buf[0..2];

    // 1. RoPE defenses
    expect_eq(ropePrecomputeTables(RopeConfig.default(0), 2, c_s, s_s), false);
    expect_eq(ropePrecomputeTables(RopeConfig.default(7), 2, c_s, s_s), false); // odd head_dim
    expect_eq(ropePrecomputeTables(RopeConfig{ head_dim: 4, base_theta: 1.0, ntk_scale: 1.0 }, 2, c_s, s_s), false);
    expect_eq(ropePrecomputeTables(RopeConfig{ head_dim: 4, base_theta: 10000.0, ntk_scale: 0.0 }, 2, c_s, s_s), false);
    expect_eq(ropePrecomputeTables(RopeConfig{ head_dim: 4, base_theta: nan_v, ntk_scale: 1.0 }, 2, c_s, s_s), false);
    expect_eq(ropePrecomputeTables(RopeConfig.default(4), 2, c_small, s_s), false); // buffer too small

    expect_eq(ropePrecomputeTables(RopeConfig.default(4), 2, c_s, s_s), true);
    let v4_buf: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    let v4_nan_buf: [4]f64 = [1.0, nan_v, 3.0, 4.0];
    let v4: []f64 = v4_buf[0..];
    let v4_nan: []f64 = v4_nan_buf[0..];
    expect_eq(ropeApplyInPlace(v4, 2, 4, c_buf[0..4], s_buf[0..4]), false); // pos=2 out of range (max=2 -> 0..1)
    expect_eq(ropeApplyInPlace(v4, 0, 3, c_buf[0..4], s_buf[0..4]), false); // odd head_dim
    expect_eq(ropeApplyInPlace(v4_nan, 0, 4, c_buf[0..4], s_buf[0..4]), false);

    // 2. RMSNorm / LayerNorm / SwiGLU defenses
    const ok2: [2]f64 = [1.0, 2.0];
    const zero2: [2]f64 = [0.0, 0.0];
    const nan2: [2]f64 = [1.0, nan_v];
    const inf2: [2]f64 = [1.0, inf_v];
    const empty_f: []const f64 = ok2[0..0];
    let o2_buf: [2]f64 = [0.0; 2];
    let o2: []f64 = o2_buf[0..];
    let o1: []f64 = o2_buf[0..1];

    expect_eq(rmsNorm(empty_f, empty_f, 0.00001, o2), false);
    expect_eq(rmsNorm(ok2[0..], empty_f, -0.00001, o2), false);
    expect_eq(rmsNorm(ok2[0..], empty_f, 0.00001, o1), false);
    expect_eq(rmsNorm(nan2[0..], empty_f, 0.00001, o2), false);
    expect_eq(rmsNorm(zero2[0..], empty_f, 0.0, o2), false); // zero denom

    expect_eq(layerNorm(empty_f, empty_f, empty_f, 0.00001, o2), false);
    expect_eq(layerNorm(ok2[0..], empty_f, empty_f, -1.0, o2), false);
    expect_eq(layerNorm(nan2[0..], empty_f, empty_f, 0.00001, o2), false);

    expect_eq(swigluForward(ok2[0..], ok2[0..1], o2), false); // length mismatch
    expect_eq(swigluForward(ok2[0..], ok2[0..], o1), false);  // output too small
    expect_eq(swigluForward(nan2[0..], ok2[0..], o2), false);
    expect_eq(swigluForward(ok2[0..], inf2[0..], o2), false);
}

fn main() {
    test("1. rope: L2 norm preservation & relative position invariance (< 1e-12)", () => {
        testRopeNormAndRelativePositionInvariance()?;
    });
    test("2. rope: NTK-Aware scaling alters rotational frequency & preserves invariance", () => {
        testRopeNtkScaling()?;
    });
    test("3. norm: rmsNorm (unit RMS, in-place) & layerNorm (zero mean, unit var, affine)", () => {
        testRmsNormAndLayerNorm()?;
    });
    test("4. ffn: silu, geluTanh symmetry & swigluForward analytic precision", () => {
        testSiluGeluAndSwiglu()?;
    });
    test("5. kernels: odd head_dim, OOB pos, negative eps, buffer & NaN/Inf defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
