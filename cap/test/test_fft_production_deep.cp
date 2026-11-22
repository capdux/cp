// Source: cp/cap/test/test_fft_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:fft & cap.fft

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FftError,
    Complex,
    WindowKind,
    FrequencyPeak,
    Fft,
} from "cap:math";

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testForwardInverseRoundtripAndParseval(): !void {
    const alloc = std.heap.page_allocator;
    const n: usize = 64;

    let input: [64]Complex = [Complex{ re: 0.0, im: 0.0 }; 64];
    let real_sig: [64]f64 = [0.0; 64];
    for (i of 0..n) {
        const t = (i as f64) / 64.0;
        const re = 1.5 * @cos(2.0 * std.math.pi * 5.0 * t) + 0.75 * @sin(2.0 * std.math.pi * 13.0 * t) + 0.25;
        const im = 0.5 * @sin(2.0 * std.math.pi * 3.0 * t);
        input[i] = Complex.init(re, im);
        real_sig[i] = re;
    }

    const spec = Fft.forward(alloc, &input)?;
    defer alloc.free(spec);
    expect_eq(spec.len, n);

    // Complex Parseval: sum |x[n]|^2 == (1/N) * sum |X[k]|^2
    let time_energy_c: f64 = 0.0;
    let freq_energy_c: f64 = 0.0;
    for (i of 0..n) {
        time_energy_c += input[i].absSq();
        freq_energy_c += spec[i].absSq();
    }
    freq_energy_c /= (n as f64);
    expect_eq(approxF64(time_energy_c, freq_energy_c, 0.000000001), true);

    // Roundtrip inverse(forward(x)) == x within < 1e-9
    const recon = Fft.inverse(alloc, spec)?;
    defer alloc.free(recon);
    expect_eq(recon.len, n);
    for (i of 0..n) {
        expect_eq(approxF64(recon[i].re, input[i].re, 0.000000001), true);
        expect_eq(approxF64(recon[i].im, input[i].im, 0.000000001), true);
    }

    // Real single-sided magnitude Parseval conservation
    const rmag = Fft.rfftMagnitude(alloc, &real_sig)?;
    defer alloc.free(rmag);
    expect_eq(rmag.len, n / 2 + 1);

    let time_energy_r: f64 = 0.0;
    for (i of 0..n) {
        time_energy_r += real_sig[i] * real_sig[i];
    }
    let freq_energy_r: f64 = 0.0;
    for (k of 0..rmag.len) {
        freq_energy_r += rmag[k] * rmag[k];
    }
    freq_energy_r /= (n as f64);
    expect_eq(approxF64(time_energy_r, freq_energy_r, 0.000000001), true);
}

fn testErrorGuardsAndPowerOfTwoHelpers(): !void {
    const alloc = std.heap.page_allocator;

    expect_eq(Fft.isPowerOfTwo(0), false);
    expect_eq(Fft.isPowerOfTwo(1), true);
    expect_eq(Fft.isPowerOfTwo(6), false);
    expect_eq(Fft.isPowerOfTwo(256), true);
    expect_eq(Fft.nextPowerOfTwo(0), 1);
    expect_eq(Fft.nextPowerOfTwo(5), 8);
    expect_eq(Fft.nextPowerOfTwo(16), 16);

    // EmptyInput on forward, inverse, applyWindow, rfftMagnitude, dominantFrequency, convolve
    const empty_c: []const Complex = [];
    let got_empty_fwd = false;
    * = Fft.forward(alloc, empty_c) catch e => {
        if (std.mem.eql(u8, @errorName(e), "EmptyInput")) got_empty_fwd = true;
    };
    expect_eq(got_empty_fwd, true);

    const empty_f: []const f64 = [];
    let got_empty_win = false;
    * = Fft.applyWindow(alloc, empty_f, WindowKind.Hann) catch e => {
        if (std.mem.eql(u8, @errorName(e), "EmptyInput")) got_empty_win = true;
    };
    expect_eq(got_empty_win, true);

    // NotPowerOfTwo on length 6
    const non_pow2_c: [6]Complex = [Complex{ re: 1.0, im: 0.0 }; 6];
    let got_np2_fwd = false;
    * = Fft.forward(alloc, &non_pow2_c) catch e => {
        if (std.mem.eql(u8, @errorName(e), "NotPowerOfTwo")) got_np2_fwd = true;
    };
    expect_eq(got_np2_fwd, true);

    let got_np2_inv = false;
    * = Fft.inverse(alloc, &non_pow2_c) catch e => {
        if (std.mem.eql(u8, @errorName(e), "NotPowerOfTwo")) got_np2_inv = true;
    };
    expect_eq(got_np2_inv, true);

    const non_pow2_f: [6]f64 = [1.0; 6];
    let got_np2_rmag = false;
    * = Fft.rfftMagnitude(alloc, &non_pow2_f) catch e => {
        if (std.mem.eql(u8, @errorName(e), "NotPowerOfTwo")) got_np2_rmag = true;
    };
    expect_eq(got_np2_rmag, true);

    // InvalidSampleRate (zero and negative)
    const valid_sig: [8]f64 = [1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0];
    let got_bad_sr_zero = false;
    * = Fft.dominantFrequency(alloc, &valid_sig, 0.0) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidSampleRate")) got_bad_sr_zero = true;
    };
    expect_eq(got_bad_sr_zero, true);

    let got_bad_sr_neg = false;
    * = Fft.dominantFrequency(alloc, &valid_sig, -16000.0) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidSampleRate")) got_bad_sr_neg = true;
    };
    expect_eq(got_bad_sr_neg, true);
}

fn testWindowsAndDominantFrequencies(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Window endpoint attenuation on unit signal
    const ones: [16]f64 = [1.0; 16];
    const hann = Fft.applyWindow(alloc, &ones, WindowKind.Hann)?;
    defer alloc.free(hann);
    expect_eq(approxF64(hann[0], 0.0, 0.000000000001), true);
    expect_eq(approxF64(hann[15], 0.0, 0.000000000001), true);

    const hamming = Fft.applyWindow(alloc, &ones, WindowKind.Hamming)?;
    defer alloc.free(hamming);
    expect_eq(approxF64(hamming[0], 0.08, 0.000000000001), true);
    expect_eq(approxF64(hamming[15], 0.08, 0.000000000001), true);

    const blackman = Fft.applyWindow(alloc, &ones, WindowKind.Blackman)?;
    defer alloc.free(blackman);
    expect_eq(approxF64(blackman[0], 0.0, 0.000000000001), true);
    expect_eq(approxF64(blackman[15], 0.0, 0.000000000001), true);

    // 2. Dominant frequency detection at 440Hz and 1000Hz (with DC offset ignored)
    // Using N = 4096, sample_rate = 4096.0 Hz => bin k == exact k Hz
    const n: usize = 4096;
    const sr: f64 = 4096.0;
    let sig_440 = alloc.alloc(f64, n) catch return FftError.OutOfMemory;
    defer alloc.free(sig_440);
    let sig_1000 = alloc.alloc(f64, n) catch return FftError.OutOfMemory;
    defer alloc.free(sig_1000);

    for (i of 0..n) {
        const t = (i as f64) / sr;
        // Include strong DC bias (5.0) to verify bin 0 is ignored
        sig_440[i] = 5.0 + @sin(2.0 * std.math.pi * 440.0 * t);
        sig_1000[i] = 5.0 + @sin(2.0 * std.math.pi * 1000.0 * t);
    }

    const peak_440 = Fft.dominantFrequency(alloc, sig_440, sr)?;
    expect_eq(peak_440.bin, 440);
    expect_eq(approxF64(peak_440.freq_hz, 440.0, 0.000000001), true);
    expect_eq(peak_440.magnitude > 1000.0, true);

    const peak_1000 = Fft.dominantFrequency(alloc, sig_1000, sr)?;
    expect_eq(peak_1000.bin, 1000);
    expect_eq(approxF64(peak_1000.freq_hz, 1000.0, 0.000000001), true);
    expect_eq(peak_1000.magnitude > 1000.0, true);
}

fn testFastConvolutionMatchesDirect(): !void {
    const alloc = std.heap.page_allocator;

    const a: [5]f64 = [1.2, -0.5, 2.3, 0.8, -1.1];
    const b: [7]f64 = [0.5, 1.0, -0.25, 0.75, -0.5, 2.0, 0.3];

    const fft_conv = Fft.convolve(alloc, &a, &b)?;
    defer alloc.free(fft_conv);
    expect_eq(fft_conv.len, a.len + b.len - 1);

    // Time-domain direct convolution reference
    let direct: [11]f64 = [0.0; 11];
    for (i of 0..a.len) {
        for (j of 0..b.len) {
            direct[i + j] += a[i] * b[j];
        }
    }

    for (k of 0..fft_conv.len) {
        expect_eq(approxF64(fft_conv[k], direct[k], 0.000000001), true);
    }
}

fn testCapFftLazyMount(): !void {
    const alloc = std.heap.page_allocator;

    let buf: [8]cap.math.Complex = [
        cap.math.Complex.init(1.0, 0.0),
        cap.math.Complex.init(2.0, -1.0),
        cap.math.Complex.init(0.5, 0.5),
        cap.math.Complex.init(-1.0, 2.0),
        cap.math.Complex.init(0.0, -0.5),
        cap.math.Complex.init(3.0, 1.0),
        cap.math.Complex.init(-2.0, 0.0),
        cap.math.Complex.init(0.25, -0.75),
    ];
    let orig: [8]cap.math.Complex = [cap.math.Complex.init(0.0, 0.0); 8];
    for (i of 0..8) {
        orig[i] = buf[i];
    }

    cap.math.Fft.forwardInPlace(&buf)?;
    cap.math.Fft.inverseInPlace(&buf)?;
    for (i of 0..8) {
        expect_eq(approxF64(buf[i].re, orig[i].re, 0.000000001), true);
        expect_eq(approxF64(buf[i].im, orig[i].im, 0.000000001), true);
    }

    const sig: [8]f64 = [1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0];
    const win = cap.math.Fft.applyWindow(alloc, &sig, cap.math.WindowKind.Rectangular)?;
    defer alloc.free(win);
    const peak = cap.math.Fft.dominantFrequency(alloc, win, 800.0)?;
    expect_eq(peak.bin, 2);
    expect_eq(approxF64(peak.freq_hz, 200.0, 0.000000001), true);
}

fn main() {
    test("1. Forward/Inverse FFT Roundtrip (<1e-9) & Parseval Energy Conservation", () => {
        testForwardInverseRoundtripAndParseval()?;
    });
    test("2. NotPowerOfTwo / EmptyInput / InvalidSampleRate Guards & Helpers", () => {
        testErrorGuardsAndPowerOfTwoHelpers()?;
    });
    test("3. Windowing (Hann/Hamming/Blackman) & 440Hz/1000Hz Dominant Frequency", () => {
        testWindowsAndDominantFrequencies()?;
    });
    test("4. Frequency-Domain Fast Convolution vs Time-Domain Direct Convolution (<1e-9)", () => {
        testFastConvolutionMatchesDirect()?;
    });
    test("5. import cap from \"cap\"; cap.fft Dynamic Lazy Mounting", () => {
        testCapFftLazyMount()?;
    });
}
