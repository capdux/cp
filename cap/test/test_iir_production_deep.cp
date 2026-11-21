// Source: cp/cap/test/test_iir_production_deep.cp (Production deep tests for cap:iir)

import { test, expect_eq } from "cap:test";
import {
    SosSection,
    SosState,
    designButterworthLowpass,
    designButterworthHighpass,
    designChebyshev1Lowpass,
    sosFilter,
    sosFiltFilt,
    sosMagnitudeDb,
} from "cap:iir";

const TWO_PI: f64 = 6.28318530717958647692;
const MINUS_3DB: f64 = -3.010299956639812;

fn absF64(v: f64): f64 {
    return if (v < 0.0) (0.0 - v) else v;
}

test("1. Butterworth lowpass (orders 2, 3, 4, 6): 0 dB DC, exact -3.0103 dB cutoff & -20*N dB/dec rolloff", () => {
    const fs: f64 = 10000.0;
    const fc: f64 = 100.0; // Low fc/fs = 0.01 so bilinear warping is negligible at 10*fc (1 decade)
    const orders: [4]usize = [2, 3, 4, 6];

    let secs_b: [4]SosSection = [SosSection.identity(); 4];
    let secs: []SosSection = secs_b[0..];

    let prev_stop_db: f64 = 0.0;
    let i: usize = 0;
    while (i < 4) {
        const ord: usize = orders[i];
        const expected_sec: usize = (ord + 1) / 2;
        const n_sec: usize = designButterworthLowpass(ord, fc, fs, *secs) ?? unreachable;
        expect_eq(n_sec, expected_sec);

        const dc_db: f64 = sosMagnitudeDb(secs[0..n_sec], fs, 0.0) ?? unreachable;
        const fc_db: f64 = sosMagnitudeDb(secs[0..n_sec], fs, fc) ?? unreachable;
        const dec_db: f64 = sosMagnitudeDb(secs[0..n_sec], fs, 10.0 * fc) ?? unreachable;

        expect_eq(absF64(dc_db) < 1.0e-9, true);
        expect_eq(absF64(fc_db - MINUS_3DB) < 1.0e-6, true);

        // At 1 decade above fc (1000 Hz << 5000 Hz Nyquist), rolloff is ~ -20*order dB (slightly steeper due to bilinear warping)
        const expected_dec_db: f64 = -20.0 * (ord as f64);
        expect_eq(dec_db <= (expected_dec_db + 0.5), true);
        expect_eq(dec_db >= (expected_dec_db - 6.0), true);
        if (i > 0) {
            expect_eq(dec_db < prev_stop_db, true);
        }
        prev_stop_db = dec_db;
        i += 1;
    }
});

test("2. 4th-order Butterworth highpass: exact -3.0103 dB cutoff, 0 dB Nyquist & deep DC rejection", () => {
    const fs: f64 = 2000.0;
    const fc: f64 = 250.0;
    let secs_b: [2]SosSection = [SosSection.identity(); 2];
    let secs: []SosSection = secs_b[0..];

    const n_sec: usize = designButterworthHighpass(4, fc, fs, *secs) ?? unreachable;
    expect_eq(n_sec, 2);

    const dc_db: f64 = sosMagnitudeDb(secs[0..n_sec], fs, 0.0) ?? unreachable;
    const low_db: f64 = sosMagnitudeDb(secs[0..n_sec], fs, 50.0) ?? unreachable;
    const fc_db: f64 = sosMagnitudeDb(secs[0..n_sec], fs, fc) ?? unreachable;
    const nyq_db: f64 = sosMagnitudeDb(secs[0..n_sec], fs, 0.5 * fs) ?? unreachable;

    expect_eq(dc_db <= -200.0, true);
    expect_eq(low_db < -50.0, true);
    expect_eq(absF64(fc_db - MINUS_3DB) < 1.0e-6, true);
    expect_eq(absF64(nyq_db) < 1.0e-9, true);
});

test("3. Chebyshev-I lowpass (1 dB ripple): exact -1.0 dB at cutoff & steeper transition than Butterworth", () => {
    const fs: f64 = 4000.0;
    const fc: f64 = 400.0;
    const ripple_db: f64 = 1.0;

    let cheb4_b: [2]SosSection = [SosSection.identity(); 2];
    let cheb4: []SosSection = cheb4_b[0..];
    const n_c4: usize = designChebyshev1Lowpass(4, ripple_db, fc, fs, *cheb4) ?? unreachable;
    expect_eq(n_c4, 2);

    let butt4_b: [2]SosSection = [SosSection.identity(); 2];
    let butt4: []SosSection = butt4_b[0..];
    const n_b4: usize = designButterworthLowpass(4, fc, fs, *butt4) ?? unreachable;
    expect_eq(n_b4, 2);

    // Even-order Chebyshev-I: DC = -ripple_db, cutoff = -ripple_db
    const c4_dc: f64 = sosMagnitudeDb(cheb4[0..n_c4], fs, 0.0) ?? unreachable;
    const c4_fc: f64 = sosMagnitudeDb(cheb4[0..n_c4], fs, fc) ?? unreachable;
    expect_eq(absF64(c4_dc - (-ripple_db)) < 1.0e-6, true);
    expect_eq(absF64(c4_fc - (-ripple_db)) < 1.0e-6, true);

    // Transition band at 1.5 * fc: Chebyshev-I is significantly steeper than same-order Butterworth
    const c4_trans: f64 = sosMagnitudeDb(cheb4[0..n_c4], fs, 1.5 * fc) ?? unreachable;
    const b4_trans: f64 = sosMagnitudeDb(butt4[0..n_b4], fs, 1.5 * fc) ?? unreachable;
    expect_eq(c4_trans < (b4_trans - 5.0), true);

    // Odd-order Chebyshev-I (order 3): DC = 0.0 dB, cutoff = -ripple_db
    let cheb3_b: [2]SosSection = [SosSection.identity(); 2];
    let cheb3: []SosSection = cheb3_b[0..];
    const n_c3: usize = designChebyshev1Lowpass(3, ripple_db, fc, fs, *cheb3) ?? unreachable;
    expect_eq(n_c3, 2);
    const c3_dc: f64 = sosMagnitudeDb(cheb3[0..n_c3], fs, 0.0) ?? unreachable;
    const c3_fc: f64 = sosMagnitudeDb(cheb3[0..n_c3], fs, fc) ?? unreachable;
    expect_eq(absF64(c3_dc) < 1.0e-6, true);
    expect_eq(absF64(c3_fc - (-ripple_db)) < 1.0e-6, true);
});

test("4. sosFiltFilt achieves zero phase delay and strong high-frequency noise suppression", () => {
    // 1000 Hz sample rate, 500 samples (0.5 s):
    //   Clean signal: 10 Hz sine (period = 100 samples, peak at idx 225 in [200..300])
    //   Interfering noise: 200 Hz sine of amplitude 0.6
    const fs: f64 = 1000.0;
    const n: usize = 500;
    let clean: [500]f64 = [0.0; 500];
    let noisy: [500]f64 = [0.0; 500];

    let i: usize = 0;
    while (i < n) {
        const t: f64 = (i as f64) / fs;
        const s_low: f64 = @sin(TWO_PI * 10.0 * t);
        const s_hi: f64 = 0.6 * @sin(TWO_PI * 200.0 * t);
        clean[i] = s_low;
        noisy[i] = s_low + s_hi;
        i += 1;
    }

    let secs_b: [2]SosSection = [SosSection.identity(); 2];
    let secs: []SosSection = secs_b[0..];
    const n_sec: usize = designButterworthLowpass(4, 35.0, fs, *secs) ?? unreachable;

    let out_ff_b: [500]f64 = [0.0; 500];
    let tmp_b: [500]f64 = [0.0; 500];
    let st_b: [2]SosState = [SosState.zero(); 2];
    let out_ff: []f64 = out_ff_b[0..];
    let tmp: []f64 = tmp_b[0..];
    let st: []SosState = st_b[0..];

    expect_eq(sosFiltFilt(secs[0..n_sec], noisy[0..], *out_ff, *tmp, *st), true);

    // Find peak index in steady-state middle cycle [200..300]
    let clean_peak_idx: usize = 200;
    let ff_peak_idx: usize = 200;
    let k: usize = 201;
    while (k < 300) {
        if (clean[k] > clean[clean_peak_idx]) {
            clean_peak_idx = k;
        }
        if (out_ff[k] > out_ff[ff_peak_idx]) {
            ff_peak_idx = k;
        }
        k += 1;
    }
    // Zero phase distortion: peak index aligns exactly with clean 10 Hz sine!
    expect_eq(ff_peak_idx, clean_peak_idx);

    // RMS error vs clean 10 Hz sine in steady-state window [100..400] should be tiny
    let sse: f64 = 0.0;
    let m: usize = 100;
    while (m < 400) {
        const diff: f64 = out_ff[m] - clean[m];
        sse += diff * diff;
        m += 1;
    }
    const rms_err: f64 = @sqrt(sse / 300.0);
    expect_eq(rms_err < 0.02, true);

    // Contrast: single-pass causal sosFilter exhibits positive phase lag on the same peak
    let out_causal_b: [500]f64 = [0.0; 500];
    let out_causal: []f64 = out_causal_b[0..];
    st[0] = SosState.zero();
    st[1] = SosState.zero();
    expect_eq(sosFilter(secs[0..n_sec], *st, noisy[0..], *out_causal), true);
    let causal_peak_idx: usize = 200;
    let c: usize = 201;
    while (c < 300) {
        if (out_causal[c] > out_causal[causal_peak_idx]) {
            causal_peak_idx = c;
        }
        c += 1;
    }
    expect_eq(causal_peak_idx > clean_peak_idx, true);
});

test("5. Boundary defense: order 0, above-Nyquist cutoff, invalid ripple, short buffers & NaN/Inf", () => {
    let secs_b: [2]SosSection = [SosSection.identity(); 2];
    let secs: []SosSection = secs_b[0..];
    let secs_short: []SosSection = secs_b[0..1];

    const z: f64 = 0.0;
    const nan_v: f64 = z / z;
    const inf_v: f64 = 1.0 / z;

    // Invalid design parameters
    expect_eq(designButterworthLowpass(0, 100.0, 1000.0, *secs) == null, true);
    expect_eq(designButterworthLowpass(4, 0.0, 1000.0, *secs) == null, true);
    expect_eq(designButterworthLowpass(4, 500.0, 1000.0, *secs) == null, true);
    expect_eq(designButterworthLowpass(4, 600.0, 1000.0, *secs) == null, true);
    expect_eq(designButterworthLowpass(4, 100.0, 0.0, *secs) == null, true);
    expect_eq(designButterworthLowpass(4, nan_v, 1000.0, *secs) == null, true);
    expect_eq(designButterworthLowpass(4, 100.0, 1000.0, *secs_short) == null, true);

    expect_eq(designButterworthHighpass(0, 100.0, 1000.0, *secs) == null, true);
    expect_eq(designButterworthHighpass(4, 500.0, 1000.0, *secs) == null, true);

    expect_eq(designChebyshev1Lowpass(4, 0.0, 100.0, 1000.0, *secs) == null, true);
    expect_eq(designChebyshev1Lowpass(4, -1.0, 100.0, 1000.0, *secs) == null, true);
    expect_eq(designChebyshev1Lowpass(4, inf_v, 100.0, 1000.0, *secs) == null, true);

    // Filter & magnitude boundary checks
    const n_sec: usize = designButterworthLowpass(4, 100.0, 1000.0, *secs) ?? unreachable;
    expect_eq(sosMagnitudeDb(secs[0..0], 1000.0, 100.0) == null, true);
    expect_eq(sosMagnitudeDb(secs[0..n_sec], 1000.0, -1.0) == null, true);
    expect_eq(sosMagnitudeDb(secs[0..n_sec], 1000.0, 501.0) == null, true);
    expect_eq(sosMagnitudeDb(secs[0..n_sec], 1000.0, nan_v) == null, true);

    const inp_nan: [4]f64 = [1.0, nan_v, 2.0, 3.0];
    const inp_ok: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    let out_b: [4]f64 = [0.0; 4];
    let tmp_b: [4]f64 = [0.0; 4];
    let st_b: [2]SosState = [SosState.zero(); 2];
    let out: []f64 = out_b[0..];
    let out_short: []f64 = out_b[0..2];
    let tmp: []f64 = tmp_b[0..];
    let st: []SosState = st_b[0..];
    let st_short: []SosState = st_b[0..1];

    expect_eq(sosFilter(secs[0..n_sec], *st, inp_nan[0..], *out), false);
    expect_eq(sosFilter(secs[0..n_sec], *st_short, inp_ok[0..], *out), false);
    expect_eq(sosFilter(secs[0..n_sec], *st, inp_ok[0..], *out_short), false);
    expect_eq(sosFiltFilt(secs[0..n_sec], inp_nan[0..], *out, *tmp, *st), false);
    expect_eq(sosFiltFilt(secs[0..n_sec], inp_ok[0..], *out_short, *tmp, *st), false);
});
