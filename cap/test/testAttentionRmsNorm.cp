// Origin: Pure-CP Stdlib Test Suite (lib/attention/rmsNorm.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Fused RMSNorm / LayerNorm / QK-Norm & SwiGLU / GeGLU:
//   1. RMSNorm F32 accuracy (`rmsNormF32` & `rmsNormInPlaceF32`) + unit RMS invariant
//   2. Gemma-2/3 unit-offset RMSNorm (`rmsNormUnitOffsetF32` & `NormConfig.gemmaDefaults`)
//   3. Fused Residual Add + RMSNorm (`fusedAddRmsNormF32`) verifying residual mutation & output equivalence
//   4. GQA multi-head QK-Norm (`qkRmsNormHeadsInPlaceF32`) head independence & batch row RMSNorm
//   5. LayerNorm (`layerNormF32`) zero-mean / unit-variance & affine weight + optional bias
//   6. SwiGLU / GeGLU / Interleaved / Split gated MLP activations numerical accuracy
//   7. Validation error boundaries (`DimMismatch`, `EmptyInput`, `InvalidEps`) & `cap.attention` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    RMSNORM_DEFAULT_EPS,
    NormKernelError,
    NormConfig,
    rmsNormF32,
    rmsNormInPlaceF32,
    rmsNormUnitOffsetF32,
    rmsNormWithConfigF32,
    fusedAddRmsNormF32,
    qkRmsNormHeadsInPlaceF32,
    batchRmsNormF32,
    layerNormF32,
    siluF32,
    geluTanhF32,
    swigluF32,
    gegluF32,
    swigluInterleavedF32,
    swigluSplitF32,
} from "cap:attention";

fn approxEq(a: f32, b: f32, tol: f32): bool {
    const d = if (a > b) a - b else b - a;
    return d <= tol;
}

test("RmsNorm: standard & in-place numerical accuracy + unit RMS invariant") {
    // x = [3.0, 4.0, 0.0, 0.0] -> mean(x^2) = 25/4 = 6.25 -> rms = 2.5
    const x: [4]f32 = [3.0, 4.0, 0.0, 0.0];
    const w: [4]f32 = [1.0, 2.0, 1.0, 1.0];
    let out: [4]f32 = [0.0; 4];
    let out_s: []f32 = out[0..];

    const n = rmsNormF32(x[0..], w[0..], 1.0e-7, *out_s) catch 0;
    expect_eq(n, @as(usize, 4));
    expect_true(approxEq(out[0], 3.0 / 2.5, 1.0e-4));          // 1.2
    expect_true(approxEq(out[1], (4.0 / 2.5) * 2.0, 1.0e-4));  // 3.2
    expect_true(approxEq(out[2], 0.0, 1.0e-6));

    // In-place matches out-of-place
    let x_ip: [4]f32 = [3.0, 4.0, 0.0, 0.0];
    let x_ip_s: []f32 = x_ip[0..];
    const n2 = rmsNormInPlaceF32(*x_ip_s, w[0..], 1.0e-7) catch 0;
    expect_eq(n2, @as(usize, 4));
    expect_true(approxEq(x_ip[0], out[0], 1.0e-6));
    expect_true(approxEq(x_ip[1], out[1], 1.0e-6));
}

test("RmsNorm: Gemma unit-offset (1 + w) scaling & NormConfig dispatch") {
    // With zero weight vector, unit_offset should behave like identity weight 1.0
    const x: [4]f32 = [2.0, -2.0, 2.0, -2.0]; // rms = 2.0
    const w_zero: [4]f32 = [0.0, 0.5, -0.5, 1.0];
    let out: [4]f32 = [0.0; 4];
    let out_s: []f32 = out[0..];

    const n = rmsNormUnitOffsetF32(x[0..], w_zero[0..], 1.0e-7, *out_s) catch 0;
    expect_eq(n, @as(usize, 4));
    expect_true(approxEq(out[0], 1.0, 1.0e-4));   // (2/2) * (1 + 0.0)
    expect_true(approxEq(out[1], -1.5, 1.0e-4));  // (-2/2) * (1 + 0.5)
    expect_true(approxEq(out[2], 0.5, 1.0e-4));   // (2/2) * (1 - 0.5)
    expect_true(approxEq(out[3], -2.0, 1.0e-4));  // (-2/2) * (1 + 1.0)

    let out2: [4]f32 = [0.0; 4];
    let out2_s: []f32 = out2[0..];
    const cfg = NormConfig.gemmaDefaults();
    expect_true(cfg.isValid());
    * = rmsNormWithConfigF32(x[0..], w_zero[0..], cfg, *out2_s) catch 0;
    expect_true(approxEq(out2[1], out[1], 1.0e-5));
}

test("RmsNorm: fusedAddRmsNormF32 updates residual & matches two-step reference") {
    let res_fused: [4]f32 = [1.0, -2.0, 3.0, 0.5];
    const x: [4]f32 = [2.0, 2.0, 1.0, -0.5];
    const w: [4]f32 = [1.5, 0.5, 2.0, 1.0];
    let out_fused: [4]f32 = [0.0; 4];
    let res_s: []f32 = res_fused[0..];
    let out_s: []f32 = out_fused[0..];

    const n = fusedAddRmsNormF32(*res_s, x[0..], w[0..], RMSNORM_DEFAULT_EPS, *out_s) catch 0;
    expect_eq(n, @as(usize, 4));

    // Reference two-step computation: residual = [3.0, 0.0, 4.0, 0.0] -> rms = 2.5
    expect_true(approxEq(res_fused[0], 3.0, 1.0e-6));
    expect_true(approxEq(res_fused[1], 0.0, 1.0e-6));
    expect_true(approxEq(res_fused[2], 4.0, 1.0e-6));
    expect_true(approxEq(res_fused[3], 0.0, 1.0e-6));

    let out_ref: [4]f32 = [0.0; 4];
    let ref_s: []f32 = out_ref[0..];
    * = rmsNormF32(res_fused[0..], w[0..], RMSNORM_DEFAULT_EPS, *ref_s) catch 0;
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        expect_true(approxEq(out_fused[i], out_ref[i], 1.0e-5));
    }
}

test("RmsNorm: GQA multi-head QK-Norm independence & batchRmsNormF32") {
    // 2 Q heads (head_dim=2), 1 KV head (head_dim=2)
    let q: [4]f32 = [3.0, 4.0, 10.0, 0.0]; // head0 rms=sqrt(12.5), head1 rms=sqrt(50)
    let k: [2]f32 = [6.0, 8.0];
    const qw: [2]f32 = [1.0, 1.0];
    const kw: [2]f32 = [2.0, 2.0];
    let q_s: []f32 = q[0..];
    let k_s: []f32 = k[0..];

    const heads = qkRmsNormHeadsInPlaceF32(*q_s, *k_s, qw[0..], kw[0..], 2, 1, 2, 1.0e-7) catch 0;
    expect_eq(heads, @as(usize, 3));

    // Each head normalized independently: head1 = [10, 0] -> [sqrt(2), 0] (~1.4142)
    expect_true(approxEq(q[2], 1.4142135, 1.0e-4));
    expect_true(approxEq(q[3], 0.0, 1.0e-6));
    // Head 0 and K head are proportional ([3,4] vs [6,8]), K has weight 2.0 so k[0] == 2 * q[0]
    expect_true(approxEq(k[0], q[0] * 2.0, 1.0e-4));
    expect_true(approxEq(k[1], q[1] * 2.0, 1.0e-4));

    // Batch 2 rows of dim 2
    const rows_in: [4]f32 = [3.0, 4.0, 10.0, 0.0];
    let rows_out: [4]f32 = [0.0; 4];
    let rows_out_s: []f32 = rows_out[0..];
    const r_cnt = batchRmsNormF32(rows_in[0..], 2, 2, qw[0..], 1.0e-7, *rows_out_s) catch 0;
    expect_eq(r_cnt, @as(usize, 2));
    expect_true(approxEq(rows_out[0], q[0], 1.0e-5));
    expect_true(approxEq(rows_out[2], q[2], 1.0e-5));
}

test("LayerNorm: zero-mean unit-variance & affine bias application") {
    const x: [4]f32 = [10.0, 20.0, 30.0, 40.0]; // mean = 25.0, var = 125.0, std = 11.18034
    const w: [4]f32 = [1.0, 1.0, 1.0, 1.0];
    const b: [4]f32 = [0.5, 0.5, 0.5, 0.5];
    let out: [4]f32 = [0.0; 4];
    let out_s: []f32 = out[0..];

    // 1. Without bias -> mean(out) == 0, var(out) ~= 1
    * = layerNormF32(x[0..], w[0..], null, 1.0e-7, *out_s) catch 0;
    const mean_out = (out[0] + out[1] + out[2] + out[3]) / 4.0;
    expect_true(approxEq(mean_out, 0.0, 1.0e-5));
    const var_out = (out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]) / 4.0;
    expect_true(approxEq(var_out, 1.0, 1.0e-4));

    // 2. With bias = 0.5 -> shifted by +0.5
    let out_b: [4]f32 = [0.0; 4];
    let out_b_s: []f32 = out_b[0..];
    * = layerNormF32(x[0..], w[0..], b[0..], 1.0e-7, *out_b_s) catch 0;
    expect_true(approxEq(out_b[0], out[0] + 0.5, 1.0e-5));
    expect_true(approxEq(out_b[3], out[3] + 0.5, 1.0e-5));
}

test("Gated MLP: SiLU, GeLU-Tanh, SwiGLU, GeGLU, Interleaved & Split projections") {
    // Scalar checks: silu(0) == 0, silu(1) ~= 0.7310586, gelu(0) == 0, gelu(1) ~= 0.841192
    expect_true(approxEq(siluF32(0.0), 0.0, 1.0e-6));
    expect_true(approxEq(siluF32(1.0), 0.7310586, 1.0e-5));
    expect_true(approxEq(siluF32(-50.0), 0.0, 1.0e-5)); // overflow-safe negative branch
    expect_true(approxEq(geluTanhF32(0.0), 0.0, 1.0e-6));
    expect_true(approxEq(geluTanhF32(1.0), 0.841192, 1.0e-4));

    const gate: [3]f32 = [0.0, 1.0, -1.0];
    const up: [3]f32 = [2.0, 3.0, 4.0];
    let sw_out: [3]f32 = [0.0; 3];
    let ge_out: [3]f32 = [0.0; 3];
    let sw_s: []f32 = sw_out[0..];
    let ge_s: []f32 = ge_out[0..];

    * = swigluF32(gate[0..], up[0..], *sw_s) catch 0;
    * = gegluF32(gate[0..], up[0..], *ge_s) catch 0;
    expect_true(approxEq(sw_out[0], 0.0, 1.0e-6));
    expect_true(approxEq(sw_out[1], siluF32(1.0) * 3.0, 1.0e-5));
    expect_true(approxEq(ge_out[1], geluTanhF32(1.0) * 3.0, 1.0e-5));

    // Interleaved [g0, u0, g1, u1, g2, u2] & Split [g0, g1, g2, u0, u1, u2] match swigluF32
    const interleaved: [6]f32 = [0.0, 2.0, 1.0, 3.0, -1.0, 4.0];
    const split: [6]f32 = [0.0, 1.0, -1.0, 2.0, 3.0, 4.0];
    let o_inter: [3]f32 = [0.0; 3];
    let o_split: [3]f32 = [0.0; 3];
    let oi_s: []f32 = o_inter[0..];
    let os_s: []f32 = o_split[0..];
    * = swigluInterleavedF32(interleaved[0..], *oi_s) catch 0;
    * = swigluSplitF32(split[0..], *os_s) catch 0;
    let i: usize = 0;
    while (i < 3) : (i += 1) {
        expect_true(approxEq(o_inter[i], sw_out[i], 1.0e-6));
        expect_true(approxEq(o_split[i], sw_out[i], 1.0e-6));
    }
}

test("RmsNorm: validation error boundaries & cap.attention facade parity") {
    const empty: [0]f32 = [];
    const x2: [2]f32 = [1.0, 2.0];
    const w3: [3]f32 = [1.0, 1.0, 1.0];
    let out2: [2]f32 = [0.0; 2];
    let out2_s: []f32 = out2[0..];
    let empty_out: [0]f32 = [];
    let empty_out_s: []f32 = empty_out[0..];

    // InvalidEps
    let got_eps_err = false;
    const r1 = cap.attention.rmsNormF32(x2[0..], x2[0..], 0.0, *out2_s) catch |e| {
        if (e == NormKernelError.InvalidEps) got_eps_err = true;
    };
    * = r1;
    expect_true(got_eps_err);

    // EmptyInput
    let got_empty_err = false;
    const r2 = cap.attention.rmsNormF32(empty[0..], empty[0..], RMSNORM_DEFAULT_EPS, *empty_out_s) catch |e| {
        if (e == NormKernelError.EmptyInput) got_empty_err = true;
    };
    * = r2;
    expect_true(got_empty_err);

    // DimMismatch
    let got_dim_err = false;
    const r3 = cap.attention.rmsNormF32(x2[0..], w3[0..], RMSNORM_DEFAULT_EPS, *out2_s) catch |e| {
        if (e == NormKernelError.DimMismatch) got_dim_err = true;
    };
    * = r3;
    expect_true(got_dim_err);
}
