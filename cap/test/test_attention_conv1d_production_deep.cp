// Origin: Pure-CP Stdlib Test Suite (lib/attention/conv1d.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation 1D Causal/Dilated Conv, Depthwise
// Conv1D, HTK Mel scale conversions, triangular Mel filterbank partition-of-unity, and SIMD
// @dot log-Mel projection.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    conv1dCausalF32,
    depthwiseConv1dF32,
    hzToMel,
    melToHz,
    buildMelFilterbankF32,
    applyMelFilterbankF32,
} from "cap:attention";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn testCausalConvNoFutureLeakage()! {
    // 2 input channels, 2 output channels, seq_len = 6, kernel_size = 3, dilation = 1.
    // Input A and Input B are identical for t in 0..3, but Input B has huge poison at t=4,5.
    const in_a: [12]f32 = [
        1.0, 2.0, 3.0, 4.0,  5.0,  6.0,
       -1.0, 0.5, 1.5, 2.5, -0.5,  1.0,
    ];
    const in_b: [12]f32 = [
        1.0, 2.0, 3.0, 4.0, 99999.0, -88888.0,
       -1.0, 0.5, 1.5, 2.5, 77777.0,  66666.0,
    ];
    const weight: [12]f32 = [
        // oc=0: ic=0 [0.2, 0.3, 0.5], ic=1 [0.1, -0.2, 0.4]
        0.2, 0.3, 0.5,   0.1, -0.2, 0.4,
        // oc=1: ic=0 [-0.5, 0.0, 0.5], ic=1 [0.25, 0.25, 0.5]
       -0.5, 0.0, 0.5,   0.25, 0.25, 0.5,
    ];
    const bias: [2]f32 = [0.5, -1.0];

    let out_a: [12]f32 = undefined;
    let out_b: [12]f32 = undefined;
    let out_a_s: []f32 = out_a[0..];
    let out_b_s: []f32 = out_b[0..];

    expect_eq(cap.attention.conv1dCausalF32(in_a[0..], weight[0..], bias[0..], 2, 2, 6, 3, 1, out_a_s), true);
    expect_eq(conv1dCausalF32(in_b[0..], weight[0..], bias[0..], 2, 2, 6, 3, 1, out_b_s), true);

    // Zero future-frame leakage: t = 0, 1, 2, 3 must be bit-identical across both channels.
    let oc: usize = 0;
    while (oc < 2) : (oc += 1) {
        let t: usize = 0;
        while (t < 4) : (t += 1) {
            expect_eq(out_a[oc * 6 + t], out_b[oc * 6 + t]);
        }
    }

    // Manual verification at oc=0, t=0 (only k=2 sees t=0; k=0,1 are left-zero padded):
    // 0.5 (bias) + 1.0*0.5 + (-1.0)*0.4 = 0.6
    expect_eq(absF64((out_a[0] as f64) - 0.6) < 0.0001, true);
    // Manual verification at oc=0, t=2 (full window [0,1,2]):
    // 0.5 + (1*0.2 + 2*0.3 + 3*0.5) + (-1*0.1 + 0.5*(-0.2) + 1.5*0.4)
    // = 0.5 + 2.3 + 0.4 = 3.2
    expect_eq(absF64((out_a[2] as f64) - 3.2) < 0.0001, true);
}

fn testCausalConvDilatedMatchesManualBaseline()! {
    // in_ch=1, out_ch=1, seq_len=7, kernel_size=3, dilation=2 -> receptive field offsets: 4, 2, 0.
    // y[t] = w[0]*x[t-4] + w[1]*x[t-2] + w[2]*x[t] + bias
    const x: [7]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0];
    const w: [3]f32 = [1.0, 2.0, 3.0];
    const b: [1]f32 = [0.5];
    let y: [7]f32 = undefined;
    let y_s: []f32 = y[0..];

    expect_eq(conv1dCausalF32(x[0..], w[0..], b[0..], 1, 1, 7, 3, 2, y_s), true);

    // Expected:
    // t=0: 3*1 + 0.5 = 3.5
    // t=1: 3*2 + 0.5 = 6.5
    // t=2: 2*1 + 3*3 + 0.5 = 11.5
    // t=3: 2*2 + 3*4 + 0.5 = 16.5
    // t=4: 1*1 + 2*3 + 3*5 + 0.5 = 22.5
    // t=5: 1*2 + 2*4 + 3*6 + 0.5 = 28.5
    // t=6: 1*3 + 2*5 + 3*7 + 0.5 = 34.5
    const expected: [7]f32 = [3.5, 6.5, 11.5, 16.5, 22.5, 28.5, 34.5];
    let t: usize = 0;
    while (t < 7) : (t += 1) {
        expect_eq(absF64((y[t] as f64) - (expected[t] as f64)) < 0.0001, true);
    }
}

fn testDepthwiseConvStridePaddingAndChannelIsolation()! {
    // 2 independent channels, seq_len = 6, kernel_size = 3, stride = 2, padding = 1 -> out_len = 3.
    // Windows in padded coordinates: [0,1,2] -> [-1,0,1], [2,3,4] -> [1,2,3], [4,5,6] -> [3,4,5].
    const input: [12]f32 = [
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, // ch 0
       10.0, 0.0, -5.0, 2.0, 1.0, 4.0, // ch 1
    ];
    const weight: [6]f32 = [
        1.0, 1.0, 1.0, // ch 0 sum kernel
        2.0, 0.0, -1.0, // ch 1 diff kernel
    ];
    const bias: [2]f32 = [0.0, 1.0];

    let out: [6]f32 = undefined;
    let out_s: []f32 = out[0..];

    expect_eq(cap.attention.depthwiseConv1dF32(input[0..], weight[0..], bias[0..], 2, 6, 3, 2, 1, out_s), true);

    // ch 0: (0+1+2)=3, (2+3+4)=9, (4+5+6)=15
    expect_eq(absF64((out[0] as f64) - 3.0) < 0.0001, true);
    expect_eq(absF64((out[1] as f64) - 9.0) < 0.0001, true);
    expect_eq(absF64((out[2] as f64) - 15.0) < 0.0001, true);

    // ch 1 (+1.0 bias): (2*0 + 0*10 - 1*0)+1=1, (2*0 + 0*(-5) - 1*2)+1=-1, (2*2 + 0*1 - 1*4)+1=1
    expect_eq(absF64((out[3] as f64) - 1.0) < 0.0001, true);
    expect_eq(absF64((out[4] as f64) - (-1.0)) < 0.0001, true);
    expect_eq(absF64((out[5] as f64) - 1.0) < 0.0001, true);
}

fn testMelScaleRoundtripFilterbankUnityAndLogMelProjection()! {
    // 1. HTK Hz <-> Mel roundtrip accuracy.
    const freqs: [5]f64 = [0.0, 100.0, 700.0, 2500.0, 8000.0];
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        const m: f64 = cap.attention.hzToMel(freqs[i]);
        const back: f64 = cap.attention.melToHz(m);
        expect_eq(absF64(back - freqs[i]) < 0.000001, true);
    }
    expect_eq(hzToMel(-50.0), 0.0);
    expect_eq(melToHz(-10.0), 0.0);

    // 2. Build 8-Mel, n_fft=64 (n_bins=33), sr=16000Hz filterbank.
    const n_mels: usize = 8;
    const n_fft: usize = 64;
    const n_bins: usize = 33;
    let fb: [264]f32 = undefined; // 8 * 33
    let fb_s: []f32 = fb[0..];

    expect_eq(buildMelFilterbankF32(n_mels, n_fft, 16000.0, 0.0, 8000.0, fb_s), true);

    // Verify all weights in [0, 1] and partition-of-unity sum == 1.0 across interior frequency bins
    // (between first filter center and last filter center).
    const mel_max: f64 = hzToMel(8000.0);
    const mel_step: f64 = mel_max / 9.0;
    const f_first_center: f64 = melToHz(mel_step);
    const f_last_center: f64 = melToHz(8.0 * mel_step);

    let b: usize = 0;
    while (b < n_bins) : (b += 1) {
        let col_sum: f64 = 0.0;
        let m: usize = 0;
        while (m < n_mels) : (m += 1) {
            const w: f32 = fb[m * n_bins + b];
            expect_eq(w >= 0.0 && w <= 1.0001, true);
            col_sum += w as f64;
        }
        const hz: f64 = (b as f64) * 16000.0 / 64.0;
        if (hz >= f_first_center && hz <= f_last_center) {
            expect_eq(absF64(col_sum - 1.0) < 0.001, true);
        }
    }

    // 3. Apply filterbank via @dot SIMD: flat unit power spectrum vs all-zero spectrum (log_floor clamp).
    let unit_spec: [33]f32 = [1.0; 33];
    let zero_spec: [33]f32 = [0.0; 33];
    let log_mel: [8]f32 = undefined;
    let log_mel_s: []f32 = log_mel[0..];

    expect_eq(applyMelFilterbankF32(unit_spec[0..], fb[0..], n_mels, n_bins, 0.00001, log_mel_s), true);
    let m: usize = 0;
    while (m < n_mels) : (m += 1) {
        expect_eq(!std.math.isNan(log_mel[m]) && !std.math.isInf(log_mel[m]), true);
    }

    const floor_val: f32 = 0.0001;
    expect_eq(applyMelFilterbankF32(zero_spec[0..], fb[0..], n_mels, n_bins, floor_val, log_mel_s), true);
    const expected_floor_log: f64 = @log(floor_val as f64);
    m = 0;
    while (m < n_mels) : (m += 1) {
        expect_eq(absF64((log_mel[m] as f64) - expected_floor_log) < 0.001, true);
    }
}

fn testInvalidDimensionsAndDefensiveGuards()! {
    const x: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    const w: [3]f32 = [1.0, 1.0, 1.0];
    const empty_b: []const f32 = x[0..0];
    let out: [4]f32 = undefined;
    let out_s: []f32 = out[0..];

    // conv1dCausalF32 guards
    expect_eq(conv1dCausalF32(x[0..], w[0..], empty_b, 0, 1, 4, 3, 1, out_s), false);
    expect_eq(conv1dCausalF32(x[0..], w[0..], empty_b, 1, 1, 4, 3, 0, out_s), false);
    expect_eq(conv1dCausalF32(x[0..], w[0..], empty_b, 1, 1, 4, 3, 1, out[0..2]), false);

    // depthwiseConv1dF32 guards: stride == 0, kernel > padded_len
    expect_eq(depthwiseConv1dF32(x[0..], w[0..], empty_b, 1, 4, 3, 0, 0, out_s), false);
    expect_eq(depthwiseConv1dF32(x[0..], w[0..], empty_b, 1, 2, 5, 1, 0, out_s), false);

    // buildMelFilterbankF32 guards: odd n_fft, f_max <= f_min, f_max > nyquist, short out_fb
    let fb: [10]f32 = undefined;
    let fb_s: []f32 = fb[0..];
    expect_eq(buildMelFilterbankF32(2, 15, 16000.0, 0.0, 8000.0, fb_s), false);
    expect_eq(buildMelFilterbankF32(2, 16, 16000.0, 4000.0, 2000.0, fb_s), false);
    expect_eq(buildMelFilterbankF32(2, 16, 16000.0, 0.0, 9000.0, fb_s), false);
    expect_eq(buildMelFilterbankF32(2, 16, 16000.0, 0.0, 8000.0, fb_s), false); // needs 2*9=18 > 10

    // applyMelFilterbankF32 guards: non-positive log_floor, NaN spectrum
    const nan_spec: [4]f32 = [1.0, std.math.nan(f32), 1.0, 1.0];
    expect_eq(applyMelFilterbankF32(x[0..], x[0..], 1, 4, 0.0, out_s), false);
    expect_eq(applyMelFilterbankF32(nan_spec[0..], x[0..], 1, 4, 0.001, out_s), false);
}

fn main() {
    test("1. attention/conv1d: causal conv1d zero future-frame leakage & manual verification", () => {
        testCausalConvNoFutureLeakage()?;
    });

    test("2. attention/conv1d: dilated causal conv1d (dilation=2) matches manual baseline", () => {
        testCausalConvDilatedMatchesManualBaseline()?;
    });

    test("3. attention/conv1d: depthwise conv1d stride, padding & channel isolation", () => {
        testDepthwiseConvStridePaddingAndChannelIsolation()?;
    });

    test("4. attention/conv1d: HTK Mel roundtrip, filterbank partition-of-unity & @dot log-Mel", () => {
        testMelScaleRoundtripFilterbankUnityAndLogMelProjection()?;
    });

    test("5. attention/conv1d: invalid dimensions, Nyquist & log_floor/NaN defensive guards", () => {
        testInvalidDimensionsAndDefensiveGuards()?;
    });
}
