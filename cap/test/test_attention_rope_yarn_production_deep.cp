// Origin: Pure-CP Stdlib Test Suite (lib/attention/rope_yarn.cp) | Created: 2026-11-22
// Production Deep Verification Suite for long-context RoPE extension (Linear PI, NTK-Aware, YaRN):
// scale==1.0 equivalence with standard RoPE, frequency monotonicity & YaRN high/mid/low band ramp,
// yarnAttentionMscale formula accuracy, applyScaledRopeInPlaceF32 exact mscale*||v|| norm, and guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RopeScalingKind,
    YarnConfig,
    yarnAttentionMscale,
    computeRopeInvFreqs,
    applyScaledRopeInPlaceF32,
} from "cap:attention";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

// ============================================================================
// Test 1: scale_factor == 1.0 Matches Standard RoPE Across All Scaling Kinds
// ============================================================================

fn testScaleOneMatchesStandardRope()! {
    const cfg = YarnConfig.default(64, 4096, 1.0);
    let f_none: [32]f64 = [0.0; 32];
    let f_lin: [32]f64 = [0.0; 32];
    let f_ntk: [32]f64 = [0.0; 32];
    let f_yarn: [32]f64 = [0.0; 32];

    let s_none: []f64 = f_none[0..];
    let s_lin: []f64 = f_lin[0..];
    let s_ntk: []f64 = f_ntk[0..];
    let s_yarn: []f64 = f_yarn[0..];

    expect_eq(cap.attention.computeRopeInvFreqs(RopeScalingKind.None, cfg, s_none), true);
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Linear, cfg, s_lin), true);
    expect_eq(computeRopeInvFreqs(RopeScalingKind.NtkAware, cfg, s_ntk), true);
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Yarn, cfg, s_yarn), true);

    let i: usize = 0;
    while (i < 32) : (i += 1) {
        const expected: f64 = 1.0 / std.math.pow(f64, 10000.0, ((i * 2) as f64) / 64.0);
        expect_eq(absF64(f_none[i] - expected) < 1.0e-12, true);
        expect_eq(absF64(f_lin[i] - expected) < 1.0e-12, true);
        expect_eq(absF64(f_ntk[i] - expected) < 1.0e-12, true);
        expect_eq(absF64(f_yarn[i] - expected) < 1.0e-12, true);
    }
}

// ============================================================================
// Test 2: Linear / NTK-Aware / YaRN Monotonicity & High/Mid/Low Band Behavior
// ============================================================================

fn testLinearNtkYarnMonotonicityAndBandBehavior()! {
    // 4x context extension: 4096 -> 16384
    const cfg = YarnConfig.default(64, 4096, 4.0);
    let f_base: [32]f64 = [0.0; 32];
    let f_lin: [32]f64 = [0.0; 32];
    let f_ntk: [32]f64 = [0.0; 32];
    let f_yarn: [32]f64 = [0.0; 32];

    let s_base: []f64 = f_base[0..];
    let s_lin: []f64 = f_lin[0..];
    let s_ntk: []f64 = f_ntk[0..];
    let s_yarn: []f64 = f_yarn[0..];

    expect_eq(computeRopeInvFreqs(RopeScalingKind.None, cfg, s_base), true);
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Linear, cfg, s_lin), true);
    expect_eq(computeRopeInvFreqs(RopeScalingKind.NtkAware, cfg, s_ntk), true);
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Yarn, cfg, s_yarn), true);

    // Strictly decreasing monotonicity across all 4 frequency schedules.
    let i: usize = 0;
    while (i < 31) : (i += 1) {
        expect_eq(f_base[i] > f_base[i + 1], true);
        expect_eq(f_lin[i] > f_lin[i + 1], true);
        expect_eq(f_ntk[i] > f_ntk[i + 1], true);
        expect_eq(f_yarn[i] > f_yarn[i + 1], true);
    }

    // Linear PI scales every band uniformly by 1 / 4.0.
    expect_eq(absF64(f_lin[0] - f_base[0] / 4.0) < 1.0e-12, true);
    expect_eq(absF64(f_lin[31] - f_base[31] / 4.0) < 1.0e-12, true);

    // NTK-Aware preserves i=0 (1.0) and stretches lower frequencies.
    expect_eq(absF64(f_ntk[0] - 1.0) < 1.0e-12, true);
    expect_eq(f_ntk[31] < f_base[31], true);

    // YaRN: high-freq band (i=0) untouched, low-freq band (i=31) linearly interpolated,
    // and mid-band smoothly blended strictly between linear and base!
    expect_eq(absF64(f_yarn[0] - f_base[0]) < 1.0e-12, true);
    expect_eq(absF64(f_yarn[31] - f_lin[31]) < 1.0e-12, true);

    let found_mid_blend: bool = false;
    let m: usize = 1;
    while (m < 31) : (m += 1) {
        expect_eq(f_yarn[m] >= f_lin[m] - 1.0e-12, true);
        expect_eq(f_yarn[m] <= f_base[m] + 1.0e-12, true);
        if (f_yarn[m] > f_lin[m] + 1.0e-9 && f_yarn[m] < f_base[m] - 1.0e-9) {
            found_mid_blend = true;
        }
    }
    expect_eq(found_mid_blend, true);
}

// ============================================================================
// Test 3: yarnAttentionMscale Formula Precision & Safe Clamping
// ============================================================================

fn testYarnAttentionMscaleAccuracy()! {
    expect_eq(absF64(cap.attention.yarnAttentionMscale(1.0, 1.0) - 1.0) < 1.0e-12, true);
    const expected_4: f64 = 0.1 * std.math.log(f64, std.math.e, 4.0) + 1.0;
    const expected_8: f64 = 0.1 * std.math.log(f64, std.math.e, 8.0) + 1.0;
    const expected_4_c2: f64 = 0.2 * std.math.log(f64, std.math.e, 4.0) + 1.0;

    expect_eq(absF64(yarnAttentionMscale(4.0, 1.0) - expected_4) < 1.0e-12, true);
    expect_eq(absF64(yarnAttentionMscale(8.0, 1.0) - expected_8) < 1.0e-12, true);
    expect_eq(absF64(yarnAttentionMscale(4.0, 2.0) - expected_4_c2) < 1.0e-12, true);

    // Defensive clamping on invalid / non-positive inputs -> 1.0.
    expect_eq(yarnAttentionMscale(0.5, 1.0), 1.0 as f64);
    expect_eq(yarnAttentionMscale(4.0, 0.0), 1.0 as f64);
    expect_eq(yarnAttentionMscale(std.math.nan(f64), 1.0), 1.0 as f64);
}

// ============================================================================
// Test 4: applyScaledRopeInPlaceF32 Exact Norm Scaling (||v'|| == mscale * ||v||)
// ============================================================================

fn testApplyScaledRopeExactNormScaling()! {
    const cfg = YarnConfig.default(64, 4096, 4.0);
    let inv_freqs: [32]f64 = [0.0; 32];
    let inv_s: []f64 = inv_freqs[0..];
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Yarn, cfg, inv_s), true);

    let v: [64]f32 = [0.0; 64];
    let v_orig: [64]f32 = [0.0; 64];
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        const val: f32 = (((i % 13) as f32) - 6.0) * 0.4;
        v[i] = val;
        v_orig[i] = val;
    }

    // Position 0 with mscale = 1.0 is an exact identity transform.
    let v_s: []f32 = v[0..];
    expect_eq(cap.attention.applyScaledRopeInPlaceF32(v_s, 0, inv_freqs[0..], 1.0), true);
    let k: usize = 0;
    while (k < 64) : (k += 1) {
        expect_eq(absF32(v[k] - v_orig[k]) < 0.0001, true);
    }

    // Position 12000 with YaRN mscale: ||v_out||_2 must equal mscale * ||v_in||_2.
    const orig_norm: f32 = @sqrt(@dot(v[0..], v[0..]));
    const mscale: f32 = yarnAttentionMscale(cfg.scale_factor, 1.0) as f32;
    expect_eq(applyScaledRopeInPlaceF32(v_s, 12000, inv_freqs[0..], mscale), true);
    const scaled_norm: f32 = @sqrt(@dot(v[0..], v[0..]));
    expect_eq(absF32(scaled_norm - (mscale * orig_norm)) < 0.001, true);
}

// ============================================================================
// Test 5: Invalid Parameter & Non-Finite Defensive Guards
// ============================================================================

fn testInvalidParameterDefensiveGuards()! {
    let freqs: [4]f64 = [0.0; 4];
    let freqs_s: []f64 = freqs[0..];
    let freqs_short: []f64 = freqs[0..3];

    // computeRopeInvFreqs guards
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Yarn, YarnConfig.default(0, 4096, 2.0), freqs_s), false);   // head_dim == 0
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Yarn, YarnConfig.default(7, 4096, 2.0), freqs_s), false);   // odd head_dim
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Yarn, YarnConfig.default(8, 4096, 2.0), freqs_short), false); // undersized out
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Yarn, YarnConfig.default(8, 0, 2.0), freqs_s), false);      // orig_max_pos == 0
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Linear, YarnConfig.default(8, 4096, 0.5), freqs_s), false); // scale < 1.0

    let bad_theta = YarnConfig.default(8, 4096, 2.0);
    bad_theta.rope_theta = 1.0;
    expect_eq(computeRopeInvFreqs(RopeScalingKind.None, bad_theta, freqs_s), false);

    let bad_beta = YarnConfig.default(8, 4096, 2.0);
    bad_beta.beta_fast = 1.0;
    bad_beta.beta_slow = 2.0;
    expect_eq(computeRopeInvFreqs(RopeScalingKind.Yarn, bad_beta, freqs_s), false);

    // applyScaledRopeInPlaceF32 guards
    let v8: [8]f32 = [1.0; 8];
    let v7: [7]f32 = [1.0; 7];
    let v_nan: [8]f32 = [1.0, std.math.nan(f32), 1.0, 1.0, 1.0, 1.0, 1.0, 1.0];
    let v8_s: []f32 = v8[0..];
    let v7_s: []f32 = v7[0..];
    let v_empty: []f32 = v8[0..0];
    let v_nan_s: []f32 = v_nan[0..];

    expect_eq(computeRopeInvFreqs(RopeScalingKind.None, YarnConfig.default(8, 4096, 1.0), freqs_s), true);
    expect_eq(applyScaledRopeInPlaceF32(v_empty, 1, freqs[0..], 1.0), false);
    expect_eq(applyScaledRopeInPlaceF32(v7_s, 1, freqs[0..], 1.0), false);
    expect_eq(applyScaledRopeInPlaceF32(v8_s, 1, freqs[0..3], 1.0), false);
    expect_eq(applyScaledRopeInPlaceF32(v8_s, 1, freqs[0..], 0.0), false);
    expect_eq(applyScaledRopeInPlaceF32(v8_s, 1, freqs[0..], -1.0), false);
    expect_eq(applyScaledRopeInPlaceF32(v_nan_s, 1, freqs[0..], 1.0), false);
}

fn main() {
    test("1. attention.rope_yarn: scale_factor == 1.0 matches standard RoPE across all kinds", () => {
        testScaleOneMatchesStandardRope()?;
    });
    test("2. attention.rope_yarn: Linear/NTK/YaRN monotonicity & high/mid/low band behavior", () => {
        testLinearNtkYarnMonotonicityAndBandBehavior()?;
    });
    test("3. attention.rope_yarn: yarnAttentionMscale formula accuracy & clamping", () => {
        testYarnAttentionMscaleAccuracy()?;
    });
    test("4. attention.rope_yarn: applyScaledRopeInPlaceF32 exact mscale*||v|| norm", () => {
        testApplyScaledRopeExactNormScaling()?;
    });
    test("5. attention.rope_yarn: invalid parameter and non-finite defensive guards", () => {
        testInvalidParameterDefensiveGuards()?;
    });
}
