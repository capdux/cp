// Source: cp/cap/test/test_fir_production_deep.cp (Production deep tests for cap:fir)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    windowHann,
    windowHamming,
    windowBlackman,
    designLowpassFir,
    FirFilter,
    Biquad,
} from "cap:math";

const TWO_PI: f64 = 6.28318530717958647692;

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

test("1. Hann, Hamming & Blackman window symmetry & exact endpoint/center values", () => {
    let hann: [17]f64 = std.mem.zeroes([17]f64);
    let hamm: [17]f64 = std.mem.zeroes([17]f64);
    let black: [17]f64 = std.mem.zeroes([17]f64);

    expect_eq(windowHann(hann[0..]), true);
    expect_eq(windowHamming(hamm[0..]), true);
    expect_eq(windowBlackman(black[0..]), true);

    // Endpoint & center values
    expect_eq(absF64(hann[0]) < 1e-12, true);
    expect_eq(absF64(hann[8] - 1.0) < 1e-12, true);
    expect_eq(absF64(hamm[0] - 0.08) < 1e-12, true);
    expect_eq(absF64(hamm[8] - 1.0) < 1e-12, true);
    expect_eq(absF64(black[0]) < 1e-12, true);
    expect_eq(absF64(black[8] - 1.0) < 1e-12, true);

    // Perfect mirror symmetry w[i] == w[N-1-i]
    let i: usize = 0;
    while (i < 17) {
        expect_eq(absF64(hann[i] - hann[16 - i]) < 1e-12, true);
        expect_eq(absF64(hamm[i] - hamm[16 - i]) < 1e-12, true);
        expect_eq(absF64(black[i] - black[16 - i]) < 1e-12, true);
        i += 1;
    }
});

test("2. designLowpassFir DC gain == 1.0 & passband vs stopband sine attenuation (> 20dB)", () => {
    // 31-tap (odd) & 32-tap (even) lowpass FIR at cutoff_norm = 0.15 (fs=1000Hz -> fc=150Hz)
    let coeffs_odd: [31]f64 = std.mem.zeroes([31]f64);
    let coeffs_even: [32]f64 = std.mem.zeroes([32]f64);
    expect_eq(designLowpassFir(0.15, coeffs_odd[0..]), true);
    expect_eq(designLowpassFir(0.15, coeffs_even[0..]), true);

    let sum_odd: f64 = 0.0;
    let i: usize = 0;
    while (i < 31) {
        sum_odd = sum_odd + coeffs_odd[i];
        expect_eq(absF64(coeffs_odd[i] - coeffs_odd[30 - i]) < 1e-12, true);
        i += 1;
    }
    expect_eq(absF64(sum_odd - 1.0) < 1e-12, true);

    let sum_even: f64 = 0.0;
    let j: usize = 0;
    while (j < 32) {
        sum_even = sum_even + coeffs_even[j];
        j += 1;
    }
    expect_eq(absF64(sum_even - 1.0) < 1e-12, true);

    // Stream low-frequency passband sine (f_norm = 0.04) and high-frequency stopband sine (f_norm = 0.38)
    let delay: [31]f64 = std.mem.zeroes([31]f64);
    let fir = FirFilter.init(coeffs_odd[0..], delay[0..]) ?? unreachable;

    let pass_in: [120]f64 = std.mem.zeroes([120]f64);
    let pass_out: [120]f64 = std.mem.zeroes([120]f64);
    let k: usize = 0;
    while (k < 120) {
        pass_in[k] = std.math.sin(TWO_PI * 0.04 * (k as f64));
        k += 1;
    }
    expect_eq(fir.processSlice(pass_in[0..], pass_out[0..]), true);

    let peak_pass: f64 = 0.0;
    let m: usize = 40;
    while (m < 120) {
        const a = absF64(pass_out[m]);
        if (a > peak_pass) { peak_pass = a; }
        m += 1;
    }
    // Passband attenuation < 0.1 (amplitude > 0.9)
    expect_eq(peak_pass > 0.9, true);

    fir.reset();
    let peak_stop: f64 = 0.0;
    let s: usize = 0;
    while (s < 120) {
        const y = fir.process(std.math.sin(TWO_PI * 0.38 * (s as f64)));
        if (s >= 40) {
            const a = absF64(y);
            if (a > peak_stop) { peak_stop = a; }
        }
        s += 1;
    }
    // Stopband attenuation > 20 dB (amplitude < 0.1)
    expect_eq(peak_stop < 0.1, true);
});

test("3. Biquad lowpass, highpass, bandpass & notch analytic magnitudeAt verification", () => {
    const fs: f64 = 48000.0;
    const fc: f64 = 1000.0;
    const q: f64 = 0.7071067811865476; // Butterworth 1/sqrt(2)

    const lp = Biquad.lowpass(fs, fc, q) ?? unreachable;
    expect_eq(absF64(lp.magnitudeAt(fs, 0.0) - 1.0) < 1e-9, true);           // DC == 1.0
    expect_eq(absF64(lp.magnitudeAt(fs, fc) - 0.70710678) < 1e-4, true);     // -3dB at fc
    expect_eq(lp.magnitudeAt(fs, fs * 0.5) < 1e-9, true);                    // Nyquist ~ 0

    const hp = Biquad.highpass(fs, fc, q) ?? unreachable;
    expect_eq(hp.magnitudeAt(fs, 0.0) < 1e-9, true);                         // DC ~ 0
    expect_eq(absF64(hp.magnitudeAt(fs, fc) - 0.70710678) < 1e-4, true);     // -3dB at fc
    expect_eq(absF64(hp.magnitudeAt(fs, fs * 0.5) - 1.0) < 1e-9, true);      // Nyquist == 1.0

    const bp = Biquad.bandpass(fs, fc, 4.0) ?? unreachable;
    expect_eq(bp.magnitudeAt(fs, 0.0) < 1e-9, true);
    expect_eq(absF64(bp.magnitudeAt(fs, fc) - 1.0) < 1e-9, true);            // 0dB peak at fc
    expect_eq(bp.magnitudeAt(fs, fs * 0.5) < 1e-9, true);

    const nt = Biquad.notch(fs, fc, 5.0) ?? unreachable;
    expect_eq(absF64(nt.magnitudeAt(fs, 0.0) - 1.0) < 1e-9, true);           // DC == 1.0
    expect_eq(nt.magnitudeAt(fs, fc) < 1e-6, true);                          // deep null at fc
    expect_eq(absF64(nt.magnitudeAt(fs, fs * 0.5) - 1.0) < 1e-9, true);      // Nyquist == 1.0
});

test("4. Biquad time-domain sine filtering matches analytic magnitudeAt & processInPlace/reset", () => {
    const fs: f64 = 8000.0;
    const fc: f64 = 1000.0;
    let nt = Biquad.notch(fs, fc, 4.0) ?? unreachable;

    // Feed 1000 Hz sine wave into 1000 Hz notch filter -> steady-state amplitude must vanish
    let buf: [400]f64 = std.mem.zeroes([400]f64);
    let i: usize = 0;
    while (i < 400) {
        buf[i] = std.math.sin(TWO_PI * fc * (i as f64) / fs);
        i += 1;
    }
    nt.processInPlace(buf[0..]);

    let peak_notch: f64 = 0.0;
    let j: usize = 300;
    while (j < 400) {
        const a = absF64(buf[j]);
        if (a > peak_notch) { peak_notch = a; }
        j += 1;
    }
    expect_eq(peak_notch < 0.02, true);

    // Reset and feed 200 Hz passband sine -> should match magnitudeAt(8000, 200)
    nt.reset();
    const expected_gain = nt.magnitudeAt(fs, 200.0);
    let peak_pass: f64 = 0.0;
    let k: usize = 0;
    while (k < 400) {
        const y = nt.process(std.math.sin(TWO_PI * 200.0 * (k as f64) / fs));
        if (k >= 300) {
            const a = absF64(y);
            if (a > peak_pass) { peak_pass = a; }
        }
        k += 1;
    }
    expect_eq(absF64(peak_pass - expected_gain) < 0.03, true);
});

test("5. Invalid parameters rejection (empty slices, bad cutoff, fs/fc/q, short delay line)", () => {
    let empty: [0]f64 = std.mem.zeroes([0]f64);
    expect_eq(windowHann(empty[0..]), false);
    expect_eq(windowHamming(empty[0..]), false);
    expect_eq(windowBlackman(empty[0..]), false);
    expect_eq(designLowpassFir(0.2, empty[0..]), false);

    let taps: [5]f64 = std.mem.zeroes([5]f64);
    expect_eq(designLowpassFir(0.0, taps[0..]), false);
    expect_eq(designLowpassFir(-0.1, taps[0..]), false);
    expect_eq(designLowpassFir(0.5, taps[0..]), false);
    expect_eq(designLowpassFir(0.6, taps[0..]), false);

    // Delay line shorter than coeffs rejected
    let short_delay: [4]f64 = std.mem.zeroes([4]f64);
    expect_eq(FirFilter.init(empty[0..], taps[0..]) == null, true);
    expect_eq(FirFilter.init(taps[0..], short_delay[0..]) == null, true);

    // processSlice output buffer too small
    let ok_delay: [5]f64 = std.mem.zeroes([5]f64);
    let fir = FirFilter.init(taps[0..], ok_delay[0..]) ?? unreachable;
    expect_eq(fir.processSlice(taps[0..], short_delay[0..]), false);

    // Biquad invalid fs, fc >= fs/2, q <= 0
    expect_eq(Biquad.lowpass(0.0, 100.0, 0.7) == null, true);
    expect_eq(Biquad.lowpass(48000.0, 0.0, 0.7) == null, true);
    expect_eq(Biquad.lowpass(48000.0, 24000.0, 0.7) == null, true);
    expect_eq(Biquad.lowpass(48000.0, 25000.0, 0.7) == null, true);
    expect_eq(Biquad.highpass(48000.0, 1000.0, 0.0) == null, true);
    expect_eq(Biquad.bandpass(48000.0, 1000.0, -1.0) == null, true);
    expect_eq(Biquad.notch(-48000.0, 1000.0, 1.0) == null, true);
});
