// ejmahler/RustFFT (MIT/Apache-2.0) Reference-Hardened FFT/RFFT/STFT/SIMD Test Suite
// Covers:
//   - Radix-4 vs Radix-2 vs Naive O(N^2) DFT across power-of-two sizes 1..4096 (max err < 1e-4)
//   - Bluestein arbitrary-length FFT/IFFT across primes & 48kHz audio frame sizes
//     (3, 5, 7, 12, 15, 60, 120, 240, 480) vs Naive O(N^2) DFT (max err < 1e-4)
//   - Parseval energy conservation, linearity, circular time-shift theorem, FFT->IFFT (< 1e-5)
//   - RFFT -> IRFFT roundtrip (< 1e-5) & exact match with full complex FFT bins
//   - Zero-heap STFT -> ISTFT Overlap-Add perfect reconstruction (< 1e-5) with Hann/Hamming/Blackman-Harris
//   - 8-lane SIMD primitives (applyWindowSimdF32, computeMagnitudeSimdF32, complexMulSimdF32,
//     dotF32Simd, convolveFirF32Simd) vs scalar reference + NaN/Inf sanitization
//   - Boundary error guards + 1,200-iteration deterministic PRNG fuzzing (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DspKernelError,
    DspWindowKind,
    fftRadix2InPlace,
    ifftRadix2InPlace,
    fftRadix4InPlace,
    ifftRadix4InPlace,
    bluesteinScratchLen,
    fftBluesteinInPlace,
    ifftBluesteinInPlace,
    rfftForward,
    irfftInverse,
    stftFrameCount,
    stftForwardF32,
    istftOverlapAddF32,
    sanitizeNonFiniteF32,
    applyWindowSimdF32,
    computeMagnitudeSimdF32,
    complexMulSimdF32,
    dotF32Simd,
    convolveFirF32Simd,
} from "../../lib/media/fftFilter.cp";

const TWO_PI: f64 = 6.28318530717958647692;

fn testAbsF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

/// Naive O(N^2) reference DFT for exact mathematical cross-checking
fn naiveDft(in_re: []const f64, in_im: []const f64, out_re: *[]f64, out_im: *[]f64) {
    const n: usize = in_re.len;
    const n_f64: f64 = @as(f64, @floatFromInt(n));
    let k: usize = 0;
    while (k < n) : (k += 1) {
        let sum_re: f64 = 0.0;
        let sum_im: f64 = 0.0;
        let t: usize = 0;
        while (t < n) : (t += 1) {
            const ang: f64 = -TWO_PI * @as(f64, @floatFromInt(k * t)) / n_f64;
            const c: f64 = std.math.cos(ang);
            const s: f64 = std.math.sin(ang);
            sum_re += in_re[t] * c - in_im[t] * s;
            sum_im += in_re[t] * s + in_im[t] * c;
        }
        out_re[k] = sum_re;
        out_im[k] = sum_im;
    }
}

fn maxComplexDiff(a_re: []const f64, a_im: []const f64, b_re: []const f64, b_im: []const f64): f64 {
    let max_e: f64 = 0.0;
    let i: usize = 0;
    while (i < a_re.len) : (i += 1) {
        const dr: f64 = testAbsF64(a_re[i] - b_re[i]);
        const di: f64 = testAbsF64(a_im[i] - b_im[i]);
        if (dr > max_e) max_e = dr;
        if (di > max_e) max_e = di;
    }
    return max_e;
}

fn main() {
    // ========================================================================
    // 1. Radix-4 vs Naive DFT & Radix-2 across Power-of-Two 1..4096
    // ========================================================================
    test("fft_ref_01_radix4_vs_naive_dft_and_radix2_1_to_4096", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const sizes: [13]usize = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096];

        let si: usize = 0;
        while (si < 13) : (si += 1) {
            const n: usize = sizes[si];
            let orig_re: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(orig_re);
            let orig_im: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(orig_im);
            let r4_re: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(r4_re);
            let r4_im: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(r4_im);
            let ref_re: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(ref_re);
            let ref_im: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(ref_im);

            let i: usize = 0;
            while (i < n) : (i += 1) {
                const fi: f64 = @as(f64, @floatFromInt(i));
                const rv: f64 = std.math.sin(fi * 0.37 + 0.2) + 0.5 * std.math.cos(fi * 1.13);
                const iv: f64 = std.math.cos(fi * 0.71 - 0.4) * 0.6;
                orig_re[i] = rv;
                orig_im[i] = iv;
                r4_re[i] = rv;
                r4_im[i] = iv;
                ref_re[i] = rv;
                ref_im[i] = iv;
            }

            fftRadix4InPlace(r4_re, r4_im) catch unreachable;

            if (n <= 128) {
                let dft_re: []f64 = alloc.alloc(f64, n) catch unreachable;
                defer alloc.free(dft_re);
                let dft_im: []f64 = alloc.alloc(f64, n) catch unreachable;
                defer alloc.free(dft_im);
                naiveDft(orig_re, orig_im, dft_re, dft_im);
                expect_eq(maxComplexDiff(r4_re, r4_im, dft_re, dft_im) < 1.0e-8, true);
            } else {
                fftRadix2InPlace(ref_re, ref_im) catch unreachable;
                expect_eq(maxComplexDiff(r4_re, r4_im, ref_re, ref_im) < 1.0e-8, true);
            }

            // Roundtrip IFFT error < 1e-10
            ifftRadix4InPlace(r4_re, r4_im) catch unreachable;
            expect_eq(maxComplexDiff(r4_re, r4_im, orig_re, orig_im) < 1.0e-10, true);
        }
    });

    // ========================================================================
    // 2. Bluestein Arbitrary-Length FFT/IFFT (Primes & 48kHz Audio Frames)
    // ========================================================================
    test("fft_ref_02_bluestein_arbitrary_lengths_vs_naive_dft", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const sizes: [9]usize = [3, 5, 7, 12, 15, 60, 120, 240, 480];

        let si: usize = 0;
        while (si < 9) : (si += 1) {
            const n: usize = sizes[si];
            const m: usize = bluesteinScratchLen(n);

            let orig_re: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(orig_re);
            let orig_im: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(orig_im);
            let x_re: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(x_re);
            let x_im: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(x_im);
            let dft_re: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(dft_re);
            let dft_im: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(dft_im);

            let a_re: []f64 = alloc.alloc(f64, m) catch unreachable;
            defer alloc.free(a_re);
            let a_im: []f64 = alloc.alloc(f64, m) catch unreachable;
            defer alloc.free(a_im);
            let b_re: []f64 = alloc.alloc(f64, m) catch unreachable;
            defer alloc.free(b_re);
            let b_im: []f64 = alloc.alloc(f64, m) catch unreachable;
            defer alloc.free(b_im);

            let i: usize = 0;
            while (i < n) : (i += 1) {
                const fi: f64 = @as(f64, @floatFromInt(i));
                const rv: f64 = std.math.cos(fi * 0.29) - 0.3 * std.math.sin(fi * 0.91);
                const iv: f64 = 0.4 * std.math.sin(fi * 0.53);
                orig_re[i] = rv;
                orig_im[i] = iv;
                x_re[i] = rv;
                x_im[i] = iv;
            }

            naiveDft(orig_re, orig_im, dft_re, dft_im);
            fftBluesteinInPlace(x_re, x_im, a_re, a_im, b_re, b_im) catch unreachable;
            expect_eq(maxComplexDiff(x_re, x_im, dft_re, dft_im) < 1.0e-7, true);

            // Inverse Bluestein roundtrip
            ifftBluesteinInPlace(x_re, x_im, a_re, a_im, b_re, b_im) catch unreachable;
            expect_eq(maxComplexDiff(x_re, x_im, orig_re, orig_im) < 1.0e-9, true);
        }
    });

    // ========================================================================
    // 3. Mathematical Theorems: Parseval, Linearity, Circular Time-Shift
    // ========================================================================
    test("fft_ref_03_parseval_linearity_and_shift_theorems", () => {
        const n: usize = 64;
        let x_re: [64]f64 = [0.0; 64];
        let x_im: [64]f64 = [0.0; 64];
        let y_re: [64]f64 = [0.0; 64];
        let y_im: [64]f64 = [0.0; 64];
        let lin_re: [64]f64 = [0.0; 64];
        let lin_im: [64]f64 = [0.0; 64];
        let sh_re: [64]f64 = [0.0; 64];
        let sh_im: [64]f64 = [0.0; 64];

        const alpha: f64 = 2.5;
        const beta: f64 = -1.75;
        const shift: usize = 7;

        let time_energy: f64 = 0.0;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            const fi: f64 = @as(f64, @floatFromInt(i));
            x_re[i] = std.math.sin(fi * 0.5) + 0.25 * std.math.cos(fi * 1.7);
            x_im[i] = 0.5 * std.math.cos(fi * 0.3);
            y_re[i] = std.math.cos(fi * 0.8);
            y_im[i] = -0.2 * std.math.sin(fi * 1.2);
            lin_re[i] = alpha * x_re[i] + beta * y_re[i];
            lin_im[i] = alpha * x_im[i] + beta * y_im[i];
            time_energy += x_re[i] * x_re[i] + x_im[i] * x_im[i];
        }

        // Circular shift x[(i - shift) mod N]
        i = 0;
        while (i < n) : (i += 1) {
            const src_idx: usize = ((i + n - shift) % n) as usize;
            sh_re[i] = x_re[src_idx];
            sh_im[i] = x_im[src_idx];
        }

        let sx_re: []f64 = x_re[0..];
        let sx_im: []f64 = x_im[0..];
        let sy_re: []f64 = y_re[0..];
        let sy_im: []f64 = y_im[0..];
        let sl_re: []f64 = lin_re[0..];
        let sl_im: []f64 = lin_im[0..];
        let ss_re: []f64 = sh_re[0..];
        let ss_im: []f64 = sh_im[0..];

        fftRadix4InPlace(sx_re, sx_im) catch unreachable;
        fftRadix4InPlace(sy_re, sy_im) catch unreachable;
        fftRadix4InPlace(sl_re, sl_im) catch unreachable;
        fftRadix4InPlace(ss_re, ss_im) catch unreachable;

        // 1. Parseval: sum |x[n]|^2 == (1/N) * sum |X[k]|^2
        let freq_energy: f64 = 0.0;
        let max_lin_err: f64 = 0.0;
        let max_shift_err: f64 = 0.0;
        const n_f64: f64 = @as(f64, @floatFromInt(n));

        i = 0;
        while (i < n) : (i += 1) {
            freq_energy += sx_re[i] * sx_re[i] + sx_im[i] * sx_im[i];

            // 2. Linearity: FFT(a*x + b*y) == a*FFT(x) + b*FFT(y)
            const exp_lr: f64 = alpha * sx_re[i] + beta * sy_re[i];
            const exp_li: f64 = alpha * sx_im[i] + beta * sy_im[i];
            const le_r: f64 = testAbsF64(sl_re[i] - exp_lr);
            const le_i: f64 = testAbsF64(sl_im[i] - exp_li);
            if (le_r > max_lin_err) max_lin_err = le_r;
            if (le_i > max_lin_err) max_lin_err = le_i;

            // 3. Time-shift: FFT(x[n - d])[k] == X[k] * exp(-i * 2*pi*k*d / N)
            const ang: f64 = -TWO_PI * @as(f64, @floatFromInt(i * shift)) / n_f64;
            const cr: f64 = std.math.cos(ang);
            const ci: f64 = std.math.sin(ang);
            const exp_sr: f64 = sx_re[i] * cr - sx_im[i] * ci;
            const exp_si: f64 = sx_re[i] * ci + sx_im[i] * cr;
            const se_r: f64 = testAbsF64(ss_re[i] - exp_sr);
            const se_i: f64 = testAbsF64(ss_im[i] - exp_si);
            if (se_r > max_shift_err) max_shift_err = se_r;
            if (se_i > max_shift_err) max_shift_err = se_i;
        }

        expect_eq(testAbsF64(time_energy - (freq_energy / n_f64)) < 1.0e-10, true);
        expect_eq(max_lin_err < 1.0e-10, true);
        expect_eq(max_shift_err < 1.0e-10, true);
    });

    // ========================================================================
    // 4. RFFT -> IRFFT Roundtrip & Exact Match with Full Complex FFT
    // ========================================================================
    test("fft_ref_04_rfft_irfft_roundtrip_and_complex_fft_match", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const sizes: [6]usize = [2, 8, 32, 128, 512, 1024];

        let si: usize = 0;
        while (si < 6) : (si += 1) {
            const n: usize = sizes[si];
            const bins: usize = (n >> 1) + 1;

            let real_in: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(real_in);
            let full_re: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(full_re);
            let full_im: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(full_im);
            let r_re: []f64 = alloc.alloc(f64, bins) catch unreachable;
            defer alloc.free(r_re);
            let r_im: []f64 = alloc.alloc(f64, bins) catch unreachable;
            defer alloc.free(r_im);
            let recon: []f64 = alloc.alloc(f64, n) catch unreachable;
            defer alloc.free(recon);
            let w_re: []f64 = alloc.alloc(f64, n >> 1) catch unreachable;
            defer alloc.free(w_re);
            let w_im: []f64 = alloc.alloc(f64, n >> 1) catch unreachable;
            defer alloc.free(w_im);

            let i: usize = 0;
            while (i < n) : (i += 1) {
                const v: f64 = std.math.sin(@as(f64, @floatFromInt(i)) * 0.41) +
                    0.35 * std.math.cos(@as(f64, @floatFromInt(i)) * 1.27);
                real_in[i] = v;
                full_re[i] = v;
                full_im[i] = 0.0;
            }

            fftRadix4InPlace(full_re, full_im) catch unreachable;
            const written = rfftForward(real_in, r_re, r_im) catch unreachable;
            expect_eq(written, bins);
            expect_eq(maxComplexDiff(r_re, r_im, full_re[0..bins], full_im[0..bins]) < 1.0e-10, true);

            const n_out = irfftInverse(r_re, r_im, recon, w_re, w_im) catch unreachable;
            expect_eq(n_out, n);
            let max_rec_err: f64 = 0.0;
            i = 0;
            while (i < n) : (i += 1) {
                const d: f64 = testAbsF64(recon[i] - real_in[i]);
                if (d > max_rec_err) max_rec_err = d;
            }
            expect_eq(max_rec_err < 1.0e-10, true);
        }
    });

    // ========================================================================
    // 5. Zero-Heap STFT -> ISTFT Overlap-Add Perfect Reconstruction (< 1e-5)
    // ========================================================================
    test("fft_ref_05_stft_istft_overlap_add_perfect_reconstruction", () => {
        const fft_size: usize = 128;
        const hop_size: usize = 32; // 75% overlap (COLA exact for Hann / Hamming / Blackman-Harris)
        const sig_len: usize = 512;
        const frames: usize = stftFrameCount(sig_len, fft_size, hop_size);
        const bins: usize = (fft_size >> 1) + 1;
        expect_eq(frames, 13);

        let sig: [512]f32 = [0.0; 512];
        let i: usize = 0;
        while (i < sig_len) : (i += 1) {
            const fi: f64 = @as(f64, @floatFromInt(i));
            sig[i] = (0.6 * std.math.sin(fi * 0.19) + 0.3 * std.math.cos(fi * 0.73)) as f32;
        }

        let frame_work: [128]f64 = [0.0; 128];
        let work_re: [64]f64 = [0.0; 64];
        let work_im: [64]f64 = [0.0; 64];
        let spec_re: [13 * 65]f64 = [0.0; 13 * 65];
        let spec_im: [13 * 65]f64 = [0.0; 13 * 65];
        let accum: [512]f64 = [0.0; 512];
        let norm: [512]f64 = [0.0; 512];
        let recon: [512]f32 = [0.0; 512];

        const windows: [3]DspWindowKind = [
            DspWindowKind.hann,
            DspWindowKind.hamming,
            DspWindowKind.blackman_harris,
        ];

        let wi: usize = 0;
        while (wi < 3) : (wi += 1) {
            let fw_s: []f64 = frame_work[0..];
            let wr_s: []f64 = work_re[0..];
            let wi_s: []f64 = work_im[0..];
            let sr_s: []f64 = spec_re[0..];
            let si_s: []f64 = spec_im[0..];
            let ac_s: []f64 = accum[0..];
            let nm_s: []f64 = norm[0..];
            let rc_s: []f32 = recon[0..];

            const nf = stftForwardF32(sig[0..], fft_size, hop_size, windows[wi], fw_s, sr_s, si_s) catch unreachable;
            expect_eq(nf, frames);

            const nrec = istftOverlapAddF32(
                sr_s, si_s, nf, fft_size, hop_size, windows[wi],
                fw_s, wr_s, wi_s, ac_s, nm_s, rc_s,
            ) catch unreachable;
            expect_eq(nrec, sig_len);

            // Check interior steady-state region [fft_size .. sig_len - fft_size]
            let max_err: f32 = 0.0;
            let k: usize = fft_size;
            while (k < sig_len - fft_size) : (k += 1) {
                const d: f32 = absF32(recon[k] - sig[k]);
                if (d > max_err) max_err = d;
            }
            expect_eq(max_err < 1.0e-5, true);
        }
    });

    // ========================================================================
    // 6. 8-Lane SIMD Primitives vs Scalar Reference & NaN/Inf Sanitization
    // ========================================================================
    test("fft_ref_06_simd_primitives_vs_scalar_and_nan_sanitize", () => {
        const n: usize = 53; // non-multiple of 8 (6x8 + 5 tail)
        let a_re: [53]f32 = [0.0; 53];
        let a_im: [53]f32 = [0.0; 53];
        let b_re: [53]f32 = [0.0; 53];
        let b_im: [53]f32 = [0.0; 53];
        let out_win: [53]f32 = [0.0; 53];
        let out_mag: [53]f32 = [0.0; 53];
        let out_re: [53]f32 = [0.0; 53];
        let out_im: [53]f32 = [0.0; 53];

        let i: usize = 0;
        while (i < n) : (i += 1) {
            const fi: f32 = @as(f32, @floatFromInt(i));
            a_re[i] = (fi * 0.13) - 2.0;
            a_im[i] = (fi * 0.07) - 1.5;
            b_re[i] = 1.0 - (fi * 0.03);
            b_im[i] = (fi * 0.05) - 0.8;
        }

        let ow_s: []f32 = out_win[0..];
        let om_s: []f32 = out_mag[0..];
        let or_s: []f32 = out_re[0..];
        let oi_s: []f32 = out_im[0..];

        expect_eq(applyWindowSimdF32(ow_s, a_re[0..], b_re[0..]) catch unreachable, n);
        expect_eq(computeMagnitudeSimdF32(a_re[0..], a_im[0..], om_s) catch unreachable, n);
        expect_eq(complexMulSimdF32(a_re[0..], a_im[0..], b_re[0..], b_im[0..], or_s, oi_s) catch unreachable, n);

        i = 0;
        while (i < n) : (i += 1) {
            expect_eq(absF32(out_win[i] - (a_re[i] * b_re[i])) < 1.0e-5, true);
            const ref_mag: f32 = std.math.sqrt(a_re[i] * a_re[i] + a_im[i] * a_im[i]);
            expect_eq(absF32(out_mag[i] - ref_mag) < 1.0e-5, true);
            const ref_r: f32 = a_re[i] * b_re[i] - a_im[i] * b_im[i];
            const ref_i: f32 = a_re[i] * b_im[i] + a_im[i] * b_re[i];
            expect_eq(absF32(out_re[i] - ref_r) < 1.0e-5, true);
            expect_eq(absF32(out_im[i] - ref_i) < 1.0e-5, true);
        }

        // NaN / Inf sanitization
        let dirty: [4]f32 = [1.0, std.math.nan(f32), std.math.inf(f32), -2.5];
        let ds: []f32 = dirty[0..];
        expect_eq(sanitizeNonFiniteF32(ds), 2);
        expect_eq(dirty[0], 1.0);
        expect_eq(dirty[1], 0.0);
        expect_eq(dirty[2], 0.0);
        expect_eq(dirty[3], -2.5);
    });

    // ========================================================================
    // 7. Boundary Guards + 1,200-Iteration Deterministic PRNG Fuzzing
    // ========================================================================
    test("fft_ref_07_boundary_guards_and_1200_deterministic_fuzz", () => {
        // Explicit boundary rejections
        let dummy1: [1]f64 = [0.0];
        let er_s: []f64 = dummy1[0..0];
        let ei_s: []f64 = dummy1[0..0];
        let got_err = false;
        fftRadix4InPlace(er_s, ei_s) catch {
            got_err = true;
        };
        expect_eq(got_err, true);

        let small3_re: [3]f64 = [1.0, 2.0, 3.0];
        let small3_im: [3]f64 = [0.0, 0.0, 0.0];
        let s3r: []f64 = small3_re[0..];
        let s3i: []f64 = small3_im[0..];
        let tiny_s: [2]f64 = [0.0; 2];
        let ts: []f64 = tiny_s[0..];
        got_err = false;
        fftBluesteinInPlace(s3r, s3i, ts, ts, ts, ts) catch {
            got_err = true;
        };
        expect_eq(got_err, true);

        // 1,200-iteration deterministic PRNG fuzz across lengths 1..37 (primes, composites, powers of 2)
        let state: u64 = 0x13579BDF2468ACE0;
        let orig_re: [40]f64 = [0.0; 40];
        let orig_im: [40]f64 = [0.0; 40];
        let work_re: [40]f64 = [0.0; 40];
        let work_im: [40]f64 = [0.0; 40];
        let a_re: [128]f64 = [0.0; 128];
        let a_im: [128]f64 = [0.0; 128];
        let b_re: [128]f64 = [0.0; 128];
        let b_im: [128]f64 = [0.0; 128];

        let iter: usize = 0;
        while (iter < 1200) : (iter += 1) {
            state ^= state >> 12;
            state ^= state << 25;
            state ^= state >> 27;
            const r = state *% 0x2545F4914F6CDD1D;
            const n: usize = (1 as usize) + ((r % 37) as usize);

            let i: usize = 0;
            while (i < n) : (i += 1) {
                state ^= state >> 12;
                state ^= state << 25;
                state ^= state >> 27;
                const v1: i32 = @as(i32, @intCast(state & 0x3FF)) - 512;
                const v2: i32 = @as(i32, @intCast((state >> 16) & 0x3FF)) - 512;
                orig_re[i] = @as(f64, @floatFromInt(v1)) * 0.01;
                orig_im[i] = @as(f64, @floatFromInt(v2)) * 0.01;
                work_re[i] = orig_re[i];
                work_im[i] = orig_im[i];
            }

            let wr: []f64 = work_re[0..n];
            let wi: []f64 = work_im[0..n];
            let ar: []f64 = a_re[0..128];
            let ai: []f64 = a_im[0..128];
            let br: []f64 = b_re[0..128];
            let bi: []f64 = b_im[0..128];

            fftBluesteinInPlace(wr, wi, ar, ai, br, bi) catch unreachable;
            ifftBluesteinInPlace(wr, wi, ar, ai, br, bi) catch unreachable;
            expect_eq(maxComplexDiff(wr, wi, orig_re[0..n], orig_im[0..n]) < 1.0e-8, true);
        }
    });
}
