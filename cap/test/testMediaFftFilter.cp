// Origin: Pure-CP Stdlib Test Suite (lib/media/fftFilter.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Unbounded-Slice Audio DSP Kernel:
//   1. Radix-2 FFT/IFFT round-trip accuracy (< 1e-9) & Parseval energy conservation (N=1024 & N=4096)
//   2. Single-tone sub-bin parabolic frequency estimation & dual-tone STFT magnitude/power-dB spectrum
//   3. RBJ Biquad LP / HP / Notch / Peaking EQ / All-Pass measured gain vs analytical `magnitudeResponseAt`
//   4. Multi-stage `BiquadCascade` (4th-order Butterworth LP) passband, -3dB cutoff & stopband roll-off
//   5. Windowed Sinc resampling (`48kHz -> 16kHz` downsampling & `16kHz -> 48kHz` upsampling) frequency fidelity
//   6. ITU-R BS.1770 K-Weighted loudness (`LUFS`), crest factor, DC offset & clipping detection
//   7. Invalid parameter / error-set guards (`DspKernelError`) & `cap.media` facade parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    DspKernelError,
    DspWindowKind,
    windowCoherentGain,
    applyDspWindowF64,
    fftRadix2InPlace,
    ifftRadix2InPlace,
    computeMagnitudeSpectrum,
    computePowerDbSpectrum,
    findPeakFrequencyHz,
    computeStftFrameMagnitude,
    BiquadFilterKind,
    designBiquad,
    BiquadFilter,
    BiquadCascade,
    resampleSincF32,
    measureAudioLoudnessF32,
} from "cap:media";

const TWO_PI: f64 = 6.28318530717958647692;

fn absDiff(a: f64, b: f64): f64 {
    const d: f64 = a - b;
    return if (d < 0.0) -d else d;
}

fn fillSineF32(buf: *[]f32, freq_hz: f64, sample_rate_hz: f64, amp: f64) {
    const inc: f64 = TWO_PI * freq_hz / sample_rate_hz;
    let i: usize = 0;
    while (i < buf.len) : (i += 1) {
        buf[i] = (amp * std.math.sin(inc * @as(f64, @floatFromInt(i)))) as f32;
    }
}

fn steadyStateRms(buf: []const f32, skip_samples: usize): f64 {
    if (buf.len <= skip_samples) return 0.0;
    let sum_sq: f64 = 0.0;
    let i: usize = skip_samples;
    while (i < buf.len) : (i += 1) {
        const v: f64 = @as(f64, buf[i]);
        sum_sq += v * v;
    }
    return std.math.sqrt(sum_sq / @as(f64, @floatFromInt(buf.len - skip_samples)));
}

test("FFT/IFFT: round-trip accuracy < 1e-9 & Parseval energy conservation") {
    let re: [1024]f64 = [0.0; 1024];
    let im: [1024]f64 = [0.0; 1024];
    let orig_re: [1024]f64 = [0.0; 1024];
    let orig_im: [1024]f64 = [0.0; 1024];

    let time_energy: f64 = 0.0;
    let i: usize = 0;
    while (i < 1024) : (i += 1) {
        const t: f64 = @as(f64, @floatFromInt(i)) / 1024.0;
        const r: f64 = 0.7 * std.math.sin(TWO_PI * 7.0 * t) + 0.35 * std.math.cos(TWO_PI * 53.0 * t) - 0.15;
        const m: f64 = 0.25 * std.math.sin(TWO_PI * 19.0 * t);
        re[i] = r;
        im[i] = m;
        orig_re[i] = r;
        orig_im[i] = m;
        time_energy += r * r + m * m;
    }

    let re_s: []f64 = re[0..];
    let im_s: []f64 = im[0..];
    fftRadix2InPlace(*re_s, *im_s) catch {
        expect_true(false);
        return;
    };

    // Parseval's theorem: sum(|x[n]|^2) == (1/N) * sum(|X[k]|^2)
    let freq_energy: f64 = 0.0;
    let k: usize = 0;
    while (k < 1024) : (k += 1) {
        freq_energy += re[k] * re[k] + im[k] * im[k];
    }
    const freq_energy_norm: f64 = freq_energy / 1024.0;
    expect_true(absDiff(time_energy, freq_energy_norm) < 1.0e-9);

    ifftRadix2InPlace(*re_s, *im_s) catch {
        expect_true(false);
        return;
    };

    let max_err: f64 = 0.0;
    let j: usize = 0;
    while (j < 1024) : (j += 1) {
        const er: f64 = absDiff(re[j], orig_re[j]);
        const ei: f64 = absDiff(im[j], orig_im[j]);
        if (er > max_err) max_err = er;
        if (ei > max_err) max_err = ei;
    }
    expect_true(max_err < 1.0e-9);

    // Also verify larger N=4096 unbounded slice works with sub-1e-9 precision.
    let re4k: [4096]f64 = [0.0; 4096];
    let im4k: [4096]f64 = [0.0; 4096];
    re4k[0] = 1.0;
    re4k[17] = -0.5;
    re4k[2048] = 0.75;
    let re4k_s: []f64 = re4k[0..];
    let im4k_s: []f64 = im4k[0..];
    fftRadix2InPlace(*re4k_s, *im4k_s) catch {
        expect_true(false);
        return;
    };
    ifftRadix2InPlace(*re4k_s, *im4k_s) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(re4k[0], 1.0) < 1.0e-9);
    expect_true(absDiff(re4k[17], -0.5) < 1.0e-9);
    expect_true(absDiff(re4k[2048], 0.75) < 1.0e-9);
}

test("FFT/STFT: parabolic sub-bin peak frequency estimation & dual-tone power spectrum") {
    const fs: f64 = 48000.0;
    const target_hz: f64 = 1037.5; // Non-integer bin (bin width = 46.875 Hz -> bin 22.133)

    let re: [1024]f64 = [0.0; 1024];
    let im: [1024]f64 = [0.0; 1024];
    let i: usize = 0;
    while (i < 1024) : (i += 1) {
        re[i] = std.math.sin(TWO_PI * target_hz * @as(f64, @floatFromInt(i)) / fs);
    }

    expect_true(absDiff(windowCoherentGain(DspWindowKind.hann), 0.5) < 1.0e-9);
    let re_s: []f64 = re[0..];
    let im_s: []f64 = im[0..];
    applyDspWindowF64(*re_s, DspWindowKind.hann);
    fftRadix2InPlace(*re_s, *im_s) catch {
        expect_true(false);
        return;
    };

    let mag: [513]f64 = [0.0; 513];
    let mag_s: []f64 = mag[0..];
    const bins = computeMagnitudeSpectrum(re[0..], im[0..], *mag_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(bins, @as(usize, 513));

    const est_hz = findPeakFrequencyHz(mag[0..bins], fs) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(est_hz, target_hz) < 1.5);

    // Dual-tone STFT frame: 440 Hz (amp 0.35) + 2000 Hz (amp 1.0)
    let pcm: [512]f32 = [0.0; 512];
    let n: usize = 0;
    while (n < 512) : (n += 1) {
        const t: f64 = @as(f64, @floatFromInt(n)) / 16000.0;
        const v: f64 = 0.35 * std.math.sin(TWO_PI * 440.0 * t) + 1.0 * std.math.sin(TWO_PI * 2000.0 * t);
        pcm[n] = v as f32;
    }
    let work_re: [512]f64 = [0.0; 512];
    let work_im: [512]f64 = [0.0; 512];
    let stft_mag: [257]f64 = [0.0; 257];
    let stft_db: [257]f64 = [0.0; 257];
    let wre_s: []f64 = work_re[0..];
    let wim_s: []f64 = work_im[0..];
    let smag_s: []f64 = stft_mag[0..];
    let sdb_s: []f64 = stft_db[0..];

    const stft_bins = computeStftFrameMagnitude(
        pcm[0..],
        DspWindowKind.blackman_harris,
        *wre_s,
        *wim_s,
        *smag_s,
    ) catch {
        expect_true(false);
        return;
    };
    expect_eq(stft_bins, @as(usize, 257));
    const dom_hz = findPeakFrequencyHz(stft_mag[0..stft_bins], 16000.0) catch 0.0;
    expect_true(absDiff(dom_hz, 2000.0) < 2.0);

    const db_bins = computePowerDbSpectrum(work_re[0..], work_im[0..], -120.0, *sdb_s) catch 0;
    expect_eq(db_bins, @as(usize, 257));
    // Bin 64 = 2000 Hz, Bin 14 = 437.5 Hz, Bin 120 = 3750 Hz (noise floor)
    expect_true(stft_db[64] > stft_db[14]);
    expect_true(stft_db[14] > stft_db[120] + 25.0);
}

test("Biquad: LP / HP / Notch / Peaking EQ / All-Pass measured gain matches analytical response") {
    const fs: f64 = 48000.0;
    let buf: [2048]f32 = [0.0; 2048];
    let buf_s: []f32 = buf[0..];

    // 1. Low-pass @ 1000 Hz: pass 200 Hz, attenuate 6000 Hz
    const lp_c = designBiquad(BiquadFilterKind.low_pass, fs, 1000.0, 0.70710678, 0.0) catch {
        expect_true(false);
        return;
    };
    let lp = BiquadFilter.init(lp_c);
    fillSineF32(*buf_s, 200.0, fs, 1.0);
    const in_rms_lo: f64 = steadyStateRms(buf[0..], 512);
    lp.processSliceInPlaceF32(*buf_s);
    const gain_lo: f64 = steadyStateRms(buf[0..], 512) / in_rms_lo;
    expect_true(absDiff(gain_lo, lp_c.magnitudeResponseAt(200.0, fs)) < 0.015);
    expect_true(gain_lo > 0.95);

    lp.reset();
    fillSineF32(*buf_s, 6000.0, fs, 1.0);
    const in_rms_hi: f64 = steadyStateRms(buf[0..], 512);
    lp.processSliceInPlaceF32(*buf_s);
    const gain_hi: f64 = steadyStateRms(buf[0..], 512) / in_rms_hi;
    expect_true(absDiff(gain_hi, lp_c.magnitudeResponseAt(6000.0, fs)) < 0.015);
    expect_true(gain_hi < 0.05);

    // 2. High-pass @ 2000 Hz: attenuate 200 Hz, pass 8000 Hz
    const hp_c = designBiquad(BiquadFilterKind.high_pass, fs, 2000.0, 0.70710678, 0.0) catch {
        expect_true(false);
        return;
    };
    expect_true(hp_c.magnitudeResponseAt(200.0, fs) < 0.02);
    expect_true(hp_c.magnitudeResponseAt(8000.0, fs) > 0.95);

    // 3. Notch @ 1000 Hz (Q = 5.0): deep null at 1000 Hz, unity at 4000 Hz
    const notch_c = designBiquad(BiquadFilterKind.notch, fs, 1000.0, 5.0, 0.0) catch {
        expect_true(false);
        return;
    };
    expect_true(notch_c.magnitudeDbAt(1000.0, fs, -120.0) < -40.0);
    expect_true(absDiff(notch_c.magnitudeResponseAt(4000.0, fs), 1.0) < 0.02);

    // 4. Peaking EQ @ 1000 Hz (+6 dB => ~1.995x linear)
    const pk_c = designBiquad(BiquadFilterKind.peaking_eq, fs, 1000.0, 2.0, 6.0) catch {
        expect_true(false);
        return;
    };
    let pk = BiquadFilter.init(pk_c);
    fillSineF32(*buf_s, 1000.0, fs, 0.25);
    const pk_in_rms: f64 = steadyStateRms(buf[0..], 512);
    pk.processSliceInPlaceF32(*buf_s);
    const pk_gain: f64 = steadyStateRms(buf[0..], 512) / pk_in_rms;
    expect_true(absDiff(pk_gain, pk_c.magnitudeResponseAt(1000.0, fs)) < 0.02);
    expect_true(absDiff(pk_gain, 1.9952623) < 0.03);

    // 5. All-pass maintains unity magnitude across frequencies
    const ap_c = designBiquad(BiquadFilterKind.all_pass, fs, 1500.0, 0.7071, 0.0) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(ap_c.magnitudeResponseAt(300.0, fs), 1.0) < 1.0e-6);
    expect_true(absDiff(ap_c.magnitudeResponseAt(1500.0, fs), 1.0) < 1.0e-6);
    expect_true(absDiff(ap_c.magnitudeResponseAt(9000.0, fs), 1.0) < 1.0e-6);
}

test("BiquadCascade: 4th-order Butterworth SOS low-pass steep roll-off & slice processing") {
    const fs: f64 = 48000.0;
    let bw4 = BiquadCascade.designButterworthLowPass(2, fs, 1000.0) catch {
        expect_true(false);
        return;
    };
    expect_eq(bw4.stageCount(), @as(usize, 2));

    // Passband (200 Hz) near 1.0, cutoff (1000 Hz) near -3.01 dB (0.7071), stopband (4000 Hz) < -22 dB
    expect_true(absDiff(bw4.magnitudeResponseAt(200.0, fs), 1.0) < 0.01);
    expect_true(absDiff(bw4.magnitudeResponseAt(1000.0, fs), 0.70710678) < 0.01);
    expect_true(bw4.magnitudeResponseAt(4000.0, fs) < 0.08);

    let src: [2048]f32 = [0.0; 2048];
    let dst: [2048]f32 = [0.0; 2048];
    let src_s: []f32 = src[0..];
    let dst_s: []f32 = dst[0..];
    fillSineF32(*src_s, 4000.0, fs, 1.0);
    const in_rms: f64 = steadyStateRms(src[0..], 512);

    const written = bw4.processSliceF32(src[0..], *dst_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(written, @as(usize, 2048));
    const measured_gain: f64 = steadyStateRms(dst[0..], 512) / in_rms;
    expect_true(absDiff(measured_gain, bw4.magnitudeResponseAt(4000.0, fs)) < 0.015);
}

test("SincResampler: 48kHz -> 16kHz downsampling & 16kHz -> 48kHz upsampling frequency fidelity") {
    // 1. Downsample 48kHz -> 16kHz (1000 Hz tone)
    let src48: [1536]f32 = [0.0; 1536];
    let src48_s: []f32 = src48[0..];
    fillSineF32(*src48_s, 1000.0, 48000.0, 0.8);

    let dst16: [512]f32 = [0.0; 512];
    let dst16_s: []f32 = dst16[0..];
    const n16 = resampleSincF32(src48[0..], 48000, *dst16_s, 16000, 12) catch {
        expect_true(false);
        return;
    };
    expect_eq(n16, @as(usize, 512));

    let re16: [512]f64 = [0.0; 512];
    let im16: [512]f64 = [0.0; 512];
    let mag16: [257]f64 = [0.0; 257];
    let re16_s: []f64 = re16[0..];
    let im16_s: []f64 = im16[0..];
    let mag16_s: []f64 = mag16[0..];
    * = computeStftFrameMagnitude(dst16[0..512], DspWindowKind.hann, *re16_s, *im16_s, *mag16_s) catch 0;
    const freq_down = findPeakFrequencyHz(mag16[0..], 16000.0) catch 0.0;
    expect_true(absDiff(freq_down, 1000.0) < 2.5);

    // 2. Upsample 16kHz -> 48kHz (1000 Hz tone)
    let src16: [512]f32 = [0.0; 512];
    let src16_s: []f32 = src16[0..];
    fillSineF32(*src16_s, 1000.0, 16000.0, 0.8);

    let dst48: [1536]f32 = [0.0; 1536];
    let dst48_s: []f32 = dst48[0..];
    const n48 = resampleSincF32(src16[0..], 16000, *dst48_s, 48000, 12) catch {
        expect_true(false);
        return;
    };
    expect_eq(n48, @as(usize, 1536));

    let re48: [1024]f64 = [0.0; 1024];
    let im48: [1024]f64 = [0.0; 1024];
    let mag48: [513]f64 = [0.0; 513];
    let re48_s: []f64 = re48[0..];
    let im48_s: []f64 = im48[0..];
    let mag48_s: []f64 = mag48[0..];
    // Use interior window [256..1280] to avoid edge transients
    * = computeStftFrameMagnitude(dst48[256..1280], DspWindowKind.hann, *re48_s, *im48_s, *mag48_s) catch 0;
    const freq_up = findPeakFrequencyHz(mag48[0..], 48000.0) catch 0.0;
    expect_true(absDiff(freq_up, 1000.0) < 2.5);

    // 3. DC unity-gain normalization check
    let dc_in: [64]f32 = [0.5; 64];
    let dc_out: [192]f32 = [0.0; 192];
    let dc_out_s: []f32 = dc_out[0..];
    const ndc = resampleSincF32(dc_in[0..], 16000, *dc_out_s, 48000, 8) catch 0;
    expect_eq(ndc, @as(usize, 192));
    expect_true(absDiff(@as(f64, dc_out[0]), 0.5) < 0.001);
    expect_true(absDiff(@as(f64, dc_out[96]), 0.5) < 0.001);
    expect_true(absDiff(@as(f64, dc_out[191]), 0.5) < 0.001);
}

test("Loudness: ITU-R BS.1770 K-Weighted LUFS, crest factor, DC offset & clipping") {
    const fs: f64 = 48000.0;
    let buf: [4800]f32 = [0.0; 4800]; // 100 ms @ 48 kHz
    let buf_s: []f32 = buf[0..];

    // 1. 1 kHz 0 dBFS sine: RMS ~ -3.01 dBFS, Peak ~ 0.0 dBFS, Crest ~ 3.01 dB
    fillSineF32(*buf_s, 1000.0, fs, 0.5); // -6.02 dBFS peak => -9.03 dBFS RMS
    const m1k = measureAudioLoudnessF32(buf[0..], fs) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(m1k.peak_dbfs, -6.0206) < 0.05);
    expect_true(absDiff(m1k.rms_dbfs, -9.0309) < 0.05);
    expect_true(absDiff(m1k.crest_factor_db, 3.0103) < 0.05);
    expect_true(absDiff(m1k.dc_offset, 0.0) < 0.001);
    expect_eq(m1k.clipped_samples, @as(usize, 0));

    // 2. K-Weighting boosts 4 kHz relative to 100 Hz by ~3.5-4.2 LU
    fillSineF32(*buf_s, 100.0, fs, 0.5);
    const m100 = measureAudioLoudnessF32(buf[0..], fs) catch {
        expect_true(false);
        return;
    };
    fillSineF32(*buf_s, 4000.0, fs, 0.5);
    const m4k = measureAudioLoudnessF32(buf[0..], fs) catch {
        expect_true(false);
        return;
    };
    const k_diff: f64 = m4k.k_weighted_lufs - m100.k_weighted_lufs;
    expect_true(k_diff > 2.5 && k_diff < 5.0);

    // 3. Clipping & DC offset detection
    let clip_buf: [16]f32 = [0.25; 16];
    clip_buf[0] = 1.0;
    clip_buf[1] = -1.0;
    clip_buf[2] = 0.99995;
    const mc = measureAudioLoudnessF32(clip_buf[0..], fs) catch {
        expect_true(false);
        return;
    };
    expect_eq(mc.clipped_samples, @as(usize, 3));
    expect_true(mc.dc_offset > 0.2);
}

test("DspKernelError: invalid sample rate, non-power-of-two FFT, out-of-range cutoff & cap.media parity") {
    // Non-power-of-two FFT rejected
    let bad_re: [12]f64 = [0.0; 12];
    let bad_im: [12]f64 = [0.0; 12];
    let bad_re_s: []f64 = bad_re[0..];
    let bad_im_s: []f64 = bad_im[0..];
    let got_non_pow2: bool = false;
    cap.media.fftRadix2InPlace(*bad_re_s, *bad_im_s) catch |e| {
        got_non_pow2 = (e == DspKernelError.NotPowerOfTwo);
    };
    expect_true(got_non_pow2);

    // Length mismatch rejected
    let ok_re: [8]f64 = [0.0; 8];
    let ok_re_s: []f64 = ok_re[0..];
    let got_mismatch: bool = false;
    fftRadix2InPlace(*ok_re_s, *bad_im_s) catch |e| {
        got_mismatch = (e == DspKernelError.LengthMismatch);
    };
    expect_true(got_mismatch);

    // Invalid sample rate
    let got_bad_sr: bool = false;
    * = cap.media.designBiquad(BiquadFilterKind.low_pass, 0.0, 1000.0, 0.707, 0.0) catch |e| {
        got_bad_sr = (e == DspKernelError.InvalidSampleRate);
    };
    expect_true(got_bad_sr);

    // Cutoff >= Nyquist rejected
    let got_bad_fc: bool = false;
    * = designBiquad(BiquadFilterKind.low_pass, 16000.0, 8000.0, 0.707, 0.0) catch |e| {
        got_bad_fc = (e == DspKernelError.InvalidCutoff);
    };
    expect_true(got_bad_fc);

    // Invalid Q <= 0 rejected
    let got_bad_q: bool = false;
    * = designBiquad(BiquadFilterKind.peaking_eq, 48000.0, 1000.0, 0.0, 3.0) catch |e| {
        got_bad_q = (e == DspKernelError.InvalidQ);
    };
    expect_true(got_bad_q);

    // Undersized resampler output buffer rejected
    let tiny_src: [32]f32 = [0.0; 32];
    let tiny_dst: [8]f32 = [0.0; 8];
    let tiny_dst_s: []f32 = tiny_dst[0..];
    let got_small: bool = false;
    * = resampleSincF32(tiny_src[0..], 16000, *tiny_dst_s, 48000, 8) catch |e| {
        got_small = (e == DspKernelError.OutputTooSmall);
    };
    expect_true(got_small);
}
