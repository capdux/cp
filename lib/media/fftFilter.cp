// Origin: Pure-CP Stdlib (Unbounded-Slice Radix-2 FFT/IFFT, STFT, RBJ Biquad IIR/SOS, Windowed Sinc Resampler & ITU-R BS.1770 K-Weighted Loudness Engine) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice driven audio DSP kernel:
//   - Spectral windows (`rectangular | hann | hamming | blackman_harris`) + coherent gain helpers
//   - Iterative in-place Radix-2 Cooley-Tukey FFT & IFFT supporting any power-of-two slice (64..65536+)
//   - Magnitude/Power-dB spectrum extraction, parabolic sub-bin peak frequency estimation & STFT frame helper
//   - Complete RBJ Audio EQ Cookbook 8-mode Biquad IIR filter (`Direct Form II Transposed`), analytical
//     complex frequency response `|H(e^{jw})|`, and up to 8-stage SOS cascade (Butterworth LP + ITU-R BS.1770 K-Weighting)
//   - Windowed Sinc anti-aliased arbitrary-ratio resampler (`resampleSincF32`) with DC-gain normalization
//   - Professional audio loudness & dynamics analyzer (`measureAudioLoudnessF32`: RMS/Peak dBFS, crest factor,
//     DC offset, clipping counter & K-Weighted LUFS with -70 LUFS absolute gating)

const std = @import("std");

// ============================================================================
// 1. Constants, Error Set & Numerical Helpers
// ============================================================================

const PI_F64: f64 = 3.14159265358979323846;
const TWO_PI_F64: f64 = 6.28318530717958647692;
const DEFAULT_DB_FLOOR: f64 = -120.0;
const CLIP_THRESHOLD_F64: f64 = 0.9999;

/// Maximum number of cascaded second-order sections (SOS) in `BiquadCascade` (up to 16th-order IIR).
pub const BIQUAD_CASCADE_MAX_STAGES: usize = 8;

/// Error set returned by unbounded-slice DSP kernel operations.
pub err DspKernelError {
    NotPowerOfTwo,
    LengthMismatch,
    InvalidSampleRate,
    InvalidCutoff,
    InvalidQ,
    InvalidRatio,
    OutputTooSmall,
}

fn isPowerOfTwo(n: usize): bool {
    return n > 0 && (n & (n - 1)) == 0;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn clampF64(x: f64, lo: f64, hi: f64): f64 {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

fn ampToDb(amp: f64, floor_db: f64): f64 {
    if (amp <= 0.0 || !isFiniteF64(amp)) {
        return floor_db;
    }
    const db: f64 = 20.0 * std.math.log10(amp);
    if (!isFiniteF64(db) || db < floor_db) {
        return floor_db;
    }
    return db;
}

fn powerToDb(power: f64, floor_db: f64): f64 {
    if (power <= 0.0 || !isFiniteF64(power)) {
        return floor_db;
    }
    const db: f64 = 10.0 * std.math.log10(power);
    if (!isFiniteF64(db) || db < floor_db) {
        return floor_db;
    }
    return db;
}

/// Normalized cardinal sine `sin(pi * x) / (pi * x)` with smooth small-argument Taylor fallback.
fn sincNormalized(x: f64): f64 {
    const ax: f64 = absF64(x);
    if (ax < 1.0e-8) {
        const px: f64 = PI_F64 * x;
        return 1.0 - (px * px) / 6.0;
    }
    const px: f64 = PI_F64 * x;
    return std.math.sin(px) / px;
}

// ============================================================================
// 2. Spectral Window Functions
// ============================================================================

/// Analysis window family for FFT / STFT spectral leakage suppression.
pub enum DspWindowKind {
    rectangular,
    hann,
    hamming,
    blackman_harris,
}

/// Evaluates a single window coefficient at sample `index` of a `len`-point window.
pub fn windowSampleAt(kind: DspWindowKind, index: usize, len: usize): f64 {
    if (len <= 1 || kind == DspWindowKind.rectangular) {
        return 1.0;
    }
    const denom: f64 = @as(f64, @floatFromInt(len - 1));
    const phase: f64 = TWO_PI_F64 * (@as(f64, @floatFromInt(index)) / denom);

    if (kind == DspWindowKind.hann) {
        return 0.5 * (1.0 - std.math.cos(phase));
    }
    if (kind == DspWindowKind.hamming) {
        return 0.54 - 0.46 * std.math.cos(phase);
    }
    // 4-term minimum Blackman-Harris window (-92 dB sidelobe suppression).
    return 0.35875 -
        0.48829 * std.math.cos(phase) +
        0.14128 * std.math.cos(2.0 * phase) -
        0.01168 * std.math.cos(3.0 * phase);
}

/// Returns the coherent amplitude gain of `kind` for amplitude-calibrated FFT analysis.
pub fn windowCoherentGain(kind: DspWindowKind): f64 {
    if (kind == DspWindowKind.rectangular) return 1.0;
    if (kind == DspWindowKind.hann) return 0.5;
    if (kind == DspWindowKind.hamming) return 0.54;
    return 0.35875;
}

/// Applies the selected analysis window in-place to an arbitrary-length `f64` buffer.
pub fn applyDspWindowF64(buf: *[]f64, kind: DspWindowKind) {
    if (buf.len <= 1 || kind == DspWindowKind.rectangular) {
        return;
    }
    let i: usize = 0;
    while (i < buf.len) : (i += 1) {
        buf[i] *= windowSampleAt(kind, i, buf.len);
    }
}

/// Applies the selected analysis window in-place to an arbitrary-length `f32` PCM buffer.
pub fn applyDspWindowF32(buf: *[]f32, kind: DspWindowKind) {
    if (buf.len <= 1 || kind == DspWindowKind.rectangular) {
        return;
    }
    let i: usize = 0;
    while (i < buf.len) : (i += 1) {
        const w: f64 = windowSampleAt(kind, i, buf.len);
        buf[i] = ((buf[i] as f64) * w) as f32;
    }
}

// ============================================================================
// 3. Unbounded-Slice Iterative Radix-2 Cooley-Tukey FFT / IFFT & Spectrum
// ============================================================================

/// Performs an in-place iterative Radix-2 Cooley-Tukey forward FFT on arbitrary power-of-two slices.
/// Zero recursion, zero dynamic heap allocation; supports any `N = 2^k` (e.g. 64..65536+).
pub fn fftRadix2InPlace(re: *[]f64, im: *[]f64): void ! DspKernelError {
    if (re.len != im.len) {
        return DspKernelError.LengthMismatch;
    }
    const n: usize = re.len;
    if (!isPowerOfTwo(n)) {
        return DspKernelError.NotPowerOfTwo;
    }
    if (n <= 1) {
        return;
    }

    // 1. In-place bit-reversal permutation.
    let i: usize = 1;
    let j: usize = 0;
    while (i < n) : (i += 1) {
        let bit: usize = n >> 1;
        while ((j & bit) != 0) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;

        if (i < j) {
            const tr: f64 = re[i];
            re[i] = re[j];
            re[j] = tr;

            const ti: f64 = im[i];
            im[i] = im[j];
            im[j] = ti;
        }
    }

    // 2. Iterative Decimation-In-Time butterfly stages with periodic twiddle re-anchoring
    //    to prevent floating-point drift on large transforms (e.g. N >= 4096).
    let stage_len: usize = 2;
    while (stage_len <= n) : (stage_len <<= 1) {
        const half: usize = stage_len >> 1;
        const ang: f64 = -TWO_PI_F64 / @as(f64, @floatFromInt(stage_len));
        const wlen_re: f64 = std.math.cos(ang);
        const wlen_im: f64 = std.math.sin(ang);

        let start: usize = 0;
        while (start < n) : (start += stage_len) {
            let w_re: f64 = 1.0;
            let w_im: f64 = 0.0;

            let k: usize = 0;
            while (k < half) : (k += 1) {
                if (k > 0 && (k & 63) == 0) {
                    const exact_ang: f64 = ang * @as(f64, @floatFromInt(k));
                    w_re = std.math.cos(exact_ang);
                    w_im = std.math.sin(exact_ang);
                }

                const idx_even: usize = start + k;
                const idx_odd: usize = idx_even + half;

                const odd_re: f64 = re[idx_odd];
                const odd_im: f64 = im[idx_odd];
                const t_re: f64 = odd_re * w_re - odd_im * w_im;
                const t_im: f64 = odd_re * w_im + odd_im * w_re;

                const u_re: f64 = re[idx_even];
                const u_im: f64 = im[idx_even];

                re[idx_even] = u_re + t_re;
                im[idx_even] = u_im + t_im;
                re[idx_odd] = u_re - t_re;
                im[idx_odd] = u_im - t_im;

                const next_w_re: f64 = w_re * wlen_re - w_im * wlen_im;
                const next_w_im: f64 = w_re * wlen_im + w_im * wlen_re;
                w_re = next_w_re;
                w_im = next_w_im;
            }
        }
    }
}

/// Performs an in-place iterative Radix-2 inverse FFT (`1/N` normalized) via conjugate symmetry.
pub fn ifftRadix2InPlace(re: *[]f64, im: *[]f64): void ! DspKernelError {
    if (re.len != im.len) {
        return DspKernelError.LengthMismatch;
    }
    const n: usize = re.len;
    if (!isPowerOfTwo(n)) {
        return DspKernelError.NotPowerOfTwo;
    }
    if (n <= 1) {
        return;
    }

    let i: usize = 0;
    while (i < n) : (i += 1) {
        im[i] = -im[i];
    }

    fftRadix2InPlace(re, im)?;

    const inv_n: f64 = 1.0 / @as(f64, @floatFromInt(n));
    let k: usize = 0;
    while (k < n) : (k += 1) {
        re[k] *= inv_n;
        im[k] = -im[k] * inv_n;
    }
}

/// Computes linear magnitude spectrum `sqrt(re[k]^2 + im[k]^2)` into `out_mag`.
/// Accepts either a one-sided (`N/2 + 1`) or full two-sided (`N`) destination slice.
/// Returns the number of bins written.
pub fn computeMagnitudeSpectrum(
    re: []const f64,
    im: []const f64,
    out_mag: *[]f64,
): usize ! DspKernelError {
    if (re.len != im.len) {
        return DspKernelError.LengthMismatch;
    }
    if (re.len == 0) {
        return 0;
    }
    const one_sided: usize = (re.len / 2) + 1;
    if (out_mag.len < one_sided) {
        return DspKernelError.OutputTooSmall;
    }
    const bins: usize = if (out_mag.len >= re.len) re.len else one_sided;
    let k: usize = 0;
    while (k < bins) : (k += 1) {
        const r: f64 = re[k];
        const m: f64 = im[k];
        out_mag[k] = std.math.sqrt(r * r + m * m);
    }
    return bins;
}

/// Computes power spectrum in decibels `10 * log10(re[k]^2 + im[k]^2)` clamped at `floor_db`.
/// Accepts either a one-sided (`N/2 + 1`) or full two-sided (`N`) destination slice.
pub fn computePowerDbSpectrum(
    re: []const f64,
    im: []const f64,
    floor_db: f64,
    out_db: *[]f64,
): usize ! DspKernelError {
    if (re.len != im.len) {
        return DspKernelError.LengthMismatch;
    }
    if (re.len == 0) {
        return 0;
    }
    const one_sided: usize = (re.len / 2) + 1;
    if (out_db.len < one_sided) {
        return DspKernelError.OutputTooSmall;
    }
    const bins: usize = if (out_db.len >= re.len) re.len else one_sided;
    let k: usize = 0;
    while (k < bins) : (k += 1) {
        const power: f64 = re[k] * re[k] + im[k] * im[k];
        out_db[k] = powerToDb(power, floor_db);
    }
    return bins;
}

/// Estimates the dominant frequency in Hz from a magnitude spectrum (`N` two-sided or `N/2 + 1` one-sided)
/// using 3-point parabolic sub-bin interpolation for sub-Hz precision.
pub fn findPeakFrequencyHz(mag: []const f64, sample_rate_hz: f64): f64 ! DspKernelError {
    if (sample_rate_hz <= 0.0 || !isFiniteF64(sample_rate_hz)) {
        return DspKernelError.InvalidSampleRate;
    }
    if (mag.len < 2) {
        return DspKernelError.OutputTooSmall;
    }

    // Infer underlying FFT length `N` and positive-frequency search bound `[1 .. search_end]`.
    let fft_len: usize = 0;
    let search_end: usize = 0;
    if (isPowerOfTwo(mag.len)) {
        fft_len = mag.len;
        search_end = mag.len / 2;
    } else {
        fft_len = (mag.len - 1) * 2;
        search_end = mag.len - 1;
    }
    if (fft_len == 0) {
        return 0.0;
    }

    let peak_idx: usize = 0;
    let peak_val: f64 = 0.0;
    let k: usize = 1;
    while (k <= search_end) : (k += 1) {
        const v: f64 = mag[k];
        if (isFiniteF64(v) && v > peak_val) {
            peak_val = v;
            peak_idx = k;
        }
    }

    if (peak_val <= 0.0) {
        return 0.0;
    }

    let refined_bin: f64 = @as(f64, @floatFromInt(peak_idx));
    if (peak_idx > 0 && peak_idx < search_end) {
        const alpha: f64 = mag[peak_idx - 1];
        const beta: f64 = mag[peak_idx];
        const gamma: f64 = mag[peak_idx + 1];
        const denom: f64 = alpha - 2.0 * beta + gamma;
        if (absF64(denom) > 1.0e-18) {
            const delta: f64 = clampF64(0.5 * (alpha - gamma) / denom, -0.5, 0.5);
            refined_bin += delta;
        }
    }

    const bin_hz: f64 = sample_rate_hz / @as(f64, @floatFromInt(fft_len));
    return refined_bin * bin_hz;
}

/// Computes one STFT analysis frame: copies `frame_f32` (zero-padded or truncated to `work_re.len`),
/// applies `window`, executes in-place Radix-2 FFT, and writes the magnitude spectrum to `out_mag`.
pub fn computeStftFrameMagnitude(
    frame_f32: []const f32,
    window: DspWindowKind,
    work_re: *[]f64,
    work_im: *[]f64,
    out_mag: *[]f64,
): usize ! DspKernelError {
    if (work_re.len != work_im.len) {
        return DspKernelError.LengthMismatch;
    }
    if (!isPowerOfTwo(work_re.len)) {
        return DspKernelError.NotPowerOfTwo;
    }
    const n: usize = work_re.len;
    const copy_len: usize = if (frame_f32.len < n) frame_f32.len else n;

    let i: usize = 0;
    while (i < copy_len) : (i += 1) {
        const s: f32 = frame_f32[i];
        work_re[i] = if (isFiniteF32(s)) @as(f64, s) else 0.0;
        work_im[i] = 0.0;
    }
    while (i < n) : (i += 1) {
        work_re[i] = 0.0;
        work_im[i] = 0.0;
    }

    applyDspWindowF64(work_re, window);
    fftRadix2InPlace(work_re, work_im)?;
    return computeMagnitudeSpectrum(work_re.*, work_im.*, out_mag);
}

// ============================================================================
// 4. RBJ Audio EQ Cookbook Biquad IIR Filters & Multi-Stage SOS Cascade
// ============================================================================

/// Complete 8-mode RBJ Audio EQ Cookbook biquad filter topology.
pub enum BiquadFilterKind {
    low_pass,
    high_pass,
    band_pass,
    notch,
    peaking_eq,
    low_shelf,
    high_shelf,
    all_pass,
}

/// Normalized second-order IIR transfer function coefficients (`a0 = 1.0`):
/// `H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)`.
pub struct BiquadCoeffs {
    b0: f64 = 1.0;
    b1: f64 = 0.0;
    b2: f64 = 0.0;
    a1: f64 = 0.0;
    a2: f64 = 0.0;

    /// Returns a transparent unity-gain pass-through coefficient set.
    static fn identity(): BiquadCoeffs {
        return BiquadCoeffs{};
    }

    /// Evaluates the exact analytical linear magnitude response `|H(e^{j*w})|` at `freq_hz`.
    fn magnitudeResponseAt(freq_hz: f64, sample_rate_hz: f64): f64 {
        if (sample_rate_hz <= 0.0 || freq_hz < 0.0) {
            return 0.0;
        }
        const w: f64 = TWO_PI_F64 * (freq_hz / sample_rate_hz);
        const c1: f64 = std.math.cos(w);
        const s1: f64 = std.math.sin(w);
        const c2: f64 = std.math.cos(2.0 * w);
        const s2: f64 = std.math.sin(2.0 * w);

        const num_re: f64 = b0 + b1 * c1 + b2 * c2;
        const num_im: f64 = -b1 * s1 - b2 * s2;
        const den_re: f64 = 1.0 + a1 * c1 + a2 * c2;
        const den_im: f64 = -a1 * s1 - a2 * s2;

        const den_sq: f64 = den_re * den_re + den_im * den_im;
        if (den_sq <= 1.0e-30) {
            return 0.0;
        }
        const num_sq: f64 = num_re * num_re + num_im * num_im;
        return std.math.sqrt(num_sq / den_sq);
    }

    /// Evaluates the analytical magnitude response in decibels clamped at `floor_db`.
    fn magnitudeDbAt(freq_hz: f64, sample_rate_hz: f64, floor_db: f64): f64 {
        return ampToDb(magnitudeResponseAt(freq_hz, sample_rate_hz), floor_db);
    }
}

/// Designs normalized `BiquadCoeffs` using the RBJ Audio EQ Cookbook bilinear transform equations.
pub fn designBiquad(
    kind: BiquadFilterKind,
    sample_rate_hz: f64,
    freq_hz: f64,
    q: f64,
    gain_db: f64,
): BiquadCoeffs ! DspKernelError {
    if (sample_rate_hz <= 0.0 || !isFiniteF64(sample_rate_hz)) {
        return DspKernelError.InvalidSampleRate;
    }
    if (freq_hz <= 0.0 || freq_hz >= 0.5 * sample_rate_hz || !isFiniteF64(freq_hz)) {
        return DspKernelError.InvalidCutoff;
    }
    if (q <= 0.0 || !isFiniteF64(q)) {
        return DspKernelError.InvalidQ;
    }
    if (!isFiniteF64(gain_db)) {
        return DspKernelError.InvalidRatio;
    }

    const w0: f64 = TWO_PI_F64 * (freq_hz / sample_rate_hz);
    const cos_w0: f64 = std.math.cos(w0);
    const sin_w0: f64 = std.math.sin(w0);
    const alpha: f64 = sin_w0 / (2.0 * q);
    const a_lin: f64 = std.math.pow(f64, 10.0, gain_db / 40.0);

    let b0: f64 = 1.0;
    let b1: f64 = 0.0;
    let b2: f64 = 0.0;
    let a0: f64 = 1.0;
    let a1: f64 = 0.0;
    let a2: f64 = 0.0;

    if (kind == BiquadFilterKind.low_pass) {
        b0 = (1.0 - cos_w0) * 0.5;
        b1 = 1.0 - cos_w0;
        b2 = (1.0 - cos_w0) * 0.5;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
    } else if (kind == BiquadFilterKind.high_pass) {
        b0 = (1.0 + cos_w0) * 0.5;
        b1 = -(1.0 + cos_w0);
        b2 = (1.0 + cos_w0) * 0.5;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
    } else if (kind == BiquadFilterKind.band_pass) {
        // Constant 0 dB peak gain band-pass.
        b0 = alpha;
        b1 = 0.0;
        b2 = -alpha;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
    } else if (kind == BiquadFilterKind.notch) {
        b0 = 1.0;
        b1 = -2.0 * cos_w0;
        b2 = 1.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
    } else if (kind == BiquadFilterKind.peaking_eq) {
        b0 = 1.0 + alpha * a_lin;
        b1 = -2.0 * cos_w0;
        b2 = 1.0 - alpha * a_lin;
        a0 = 1.0 + alpha / a_lin;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha / a_lin;
    } else if (kind == BiquadFilterKind.low_shelf) {
        const two_sqrt_a_alpha: f64 = 2.0 * std.math.sqrt(a_lin) * alpha;
        b0 = a_lin * ((a_lin + 1.0) - (a_lin - 1.0) * cos_w0 + two_sqrt_a_alpha);
        b1 = 2.0 * a_lin * ((a_lin - 1.0) - (a_lin + 1.0) * cos_w0);
        b2 = a_lin * ((a_lin + 1.0) - (a_lin - 1.0) * cos_w0 - two_sqrt_a_alpha);
        a0 = (a_lin + 1.0) + (a_lin - 1.0) * cos_w0 + two_sqrt_a_alpha;
        a1 = -2.0 * ((a_lin - 1.0) + (a_lin + 1.0) * cos_w0);
        a2 = (a_lin + 1.0) + (a_lin - 1.0) * cos_w0 - two_sqrt_a_alpha;
    } else if (kind == BiquadFilterKind.high_shelf) {
        const two_sqrt_a_alpha: f64 = 2.0 * std.math.sqrt(a_lin) * alpha;
        b0 = a_lin * ((a_lin + 1.0) + (a_lin - 1.0) * cos_w0 + two_sqrt_a_alpha);
        b1 = -2.0 * a_lin * ((a_lin - 1.0) + (a_lin + 1.0) * cos_w0);
        b2 = a_lin * ((a_lin + 1.0) + (a_lin - 1.0) * cos_w0 - two_sqrt_a_alpha);
        a0 = (a_lin + 1.0) - (a_lin - 1.0) * cos_w0 + two_sqrt_a_alpha;
        a1 = 2.0 * ((a_lin - 1.0) - (a_lin + 1.0) * cos_w0);
        a2 = (a_lin + 1.0) - (a_lin - 1.0) * cos_w0 - two_sqrt_a_alpha;
    } else {
        // all_pass
        b0 = 1.0 - alpha;
        b1 = -2.0 * cos_w0;
        b2 = 1.0 + alpha;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
    }

    const inv_a0: f64 = 1.0 / a0;
    return BiquadCoeffs{
        b0: b0 * inv_a0,
        b1: b1 * inv_a0,
        b2: b2 * inv_a0,
        a1: a1 * inv_a0,
        a2: a2 * inv_a0,
    };
}

/// Stateful Direct Form II Transposed Biquad IIR filter (superior floating-point numerical stability).
pub struct BiquadFilter {
    coeffs: BiquadCoeffs = BiquadCoeffs{};
    z1: f64 = 0.0;
    z2: f64 = 0.0;

    /// Initializes a `BiquadFilter` with zeroed delay state.
    static fn init(coeffs: BiquadCoeffs): BiquadFilter {
        return BiquadFilter{
            coeffs: coeffs,
            z1: 0.0,
            z2: 0.0,
        };
    }

    /// Resets internal delay registers to zero.
    fn reset(*_) {
        z1 = 0.0;
        z2 = 0.0;
    }

    /// Updates filter coefficients while preserving delay state for click-free automation.
    fn setCoeffs(*_, new_coeffs: BiquadCoeffs) {
        coeffs = new_coeffs;
    }

    /// Filters a single `f64` sample using Direct Form II Transposed.
    fn processSample(*_, x: f64): f64 {
        const y: f64 = coeffs.b0 * x + z1;
        z1 = coeffs.b1 * x - coeffs.a1 * y + z2;
        z2 = coeffs.b2 * x - coeffs.a2 * y;
        return y;
    }

    /// Filters an arbitrary-length `f32` PCM slice in-place.
    fn processSliceInPlaceF32(*_, samples: *[]f32) {
        let i: usize = 0;
        while (i < samples.len) : (i += 1) {
            const x: f64 = samples[i] as f64;
            samples[i] = processSample(x) as f32;
        }
    }

    /// Filters `src` into `dst` (returning `OutputTooSmall` if `dst.len < src.len`).
    fn processSliceF32(*_, src: []const f32, dst: *[]f32): usize ! DspKernelError {
        if (dst.len < src.len) {
            return DspKernelError.OutputTooSmall;
        }
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            const x: f64 = src[i] as f64;
            dst[i] = processSample(x) as f32;
        }
        return src.len;
    }
}

/// Cascaded Second-Order Sections (SOS) IIR filter bank supporting up to 8 biquad stages (16th-order).
pub struct BiquadCascade {
    stages: [BIQUAD_CASCADE_MAX_STAGES]BiquadFilter = [BiquadFilter{}; BIQUAD_CASCADE_MAX_STAGES];
    stage_count: usize = 0;

    /// Creates an empty `BiquadCascade` (0 stages = unity pass-through).
    static fn init(): BiquadCascade {
        return BiquadCascade{};
    }

    /// Appends one biquad stage to the cascade.
    fn addStage(*_, coeffs: BiquadCoeffs): void ! DspKernelError {
        if (stage_count >= BIQUAD_CASCADE_MAX_STAGES) {
            return DspKernelError.OutputTooSmall;
        }
        stages[stage_count] = BiquadFilter.init(coeffs);
        stage_count += 1;
    }

    /// Returns the number of active biquad stages.
    fn stageCount(): usize {
        return stage_count;
    }

    /// Resets delay registers across all active stages.
    fn reset(*_) {
        let s: usize = 0;
        while (s < stage_count) : (s += 1) {
            stages[s].reset();
        }
    }

    /// Processes a single `f64` sample sequentially through all active stages.
    fn processSample(*_, x: f64): f64 {
        let v: f64 = x;
        let s: usize = 0;
        while (s < stage_count) : (s += 1) {
            v = stages[s].processSample(v);
        }
        return v;
    }

    /// Processes an arbitrary-length `f32` PCM buffer in-place through all stages.
    fn processSliceInPlaceF32(*_, samples: *[]f32) {
        let i: usize = 0;
        while (i < samples.len) : (i += 1) {
            const y: f64 = processSample(samples[i] as f64);
            samples[i] = y as f32;
        }
    }

    /// Processes `src` into `dst` through all stages.
    fn processSliceF32(*_, src: []const f32, dst: *[]f32): usize ! DspKernelError {
        if (dst.len < src.len) {
            return DspKernelError.OutputTooSmall;
        }
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            const y: f64 = processSample(src[i] as f64);
            dst[i] = y as f32;
        }
        return src.len;
    }

    /// Computes the combined analytical magnitude response `prod_k |H_k(e^{j*w})|` at `freq_hz`.
    fn magnitudeResponseAt(freq_hz: f64, sample_rate_hz: f64): f64 {
        let mag: f64 = 1.0;
        let s: usize = 0;
        while (s < stage_count) : (s += 1) {
            mag *= stages[s].coeffs.magnitudeResponseAt(freq_hz, sample_rate_hz);
        }
        return mag;
    }

    /// Designs an even-order (`2 * num_stages`) maximally-flat Butterworth low-pass SOS cascade.
    static fn designButterworthLowPass(
        num_stages: usize,
        sample_rate_hz: f64,
        cutoff_hz: f64,
    ): BiquadCascade ! DspKernelError {
        if (num_stages == 0 || num_stages > BIQUAD_CASCADE_MAX_STAGES) {
            return DspKernelError.OutputTooSmall;
        }
        let cascade = BiquadCascade.init();
        const total_order: f64 = @as(f64, @floatFromInt(num_stages * 2));
        let k: usize = 0;
        while (k < num_stages) : (k += 1) {
            const angle: f64 = PI_F64 * (@as(f64, @floatFromInt(2 * k + 1))) / (2.0 * total_order);
            const q: f64 = 1.0 / (2.0 * std.math.cos(angle));
            const c = designBiquad(BiquadFilterKind.low_pass, sample_rate_hz, cutoff_hz, q, 0.0)?;
            cascade.addStage(c)?;
        }
        return cascade;
    }

    /// Designs the ITU-R BS.1770-4 two-stage K-Weighting pre-filter cascade:
    ///   - Stage 1: +4.0 dB high-shelf head acoustics pre-filter (~1681.97 Hz, Q = 0.707175)
    ///   - Stage 2: RLB high-pass weighting filter (~38.135 Hz, Q = 0.500327)
    static fn designKWeighting(sample_rate_hz: f64): BiquadCascade ! DspKernelError {
        if (sample_rate_hz <= 0.0 || !isFiniteF64(sample_rate_hz)) {
            return DspKernelError.InvalidSampleRate;
        }
        let cascade = BiquadCascade.init();
        const nyquist: f64 = 0.5 * sample_rate_hz;

        // Stage 1: acoustic head shelf (only when Nyquist comfortably exceeds 1682 Hz).
        if (nyquist > 1800.0) {
            const shelf = designBiquad(
                BiquadFilterKind.high_shelf,
                sample_rate_hz,
                1681.974450955533,
                0.7071752369554196,
                3.999843853973347,
            )?;
            cascade.addStage(shelf)?;
        }

        // Stage 2: Revised Low-frequency B-weighting (RLB) high-pass filter.
        if (nyquist > 60.0) {
            const rlb = designBiquad(
                BiquadFilterKind.high_pass,
                sample_rate_hz,
                38.13547087602444,
                0.5003270373238773,
                0.0,
            )?;
            cascade.addStage(rlb)?;
        }

        return cascade;
    }
}

// ============================================================================
// 5. Windowed Sinc High-Quality Arbitrary-Ratio Resampler
// ============================================================================

/// Resamples an arbitrary-length `f32` PCM buffer from `src_rate_hz` to `dst_rate_hz` using a
/// Hann-windowed Sinc interpolation kernel with automatic anti-aliasing cutoff scaling for downsampling
/// and per-sample DC gain normalization. Zero dynamic heap allocation.
/// Returns the number of output samples written to `dst`.
pub fn resampleSincF32(
    src: []const f32,
    src_rate_hz: u32,
    dst: *[]f32,
    dst_rate_hz: u32,
    half_taps: usize,
): usize ! DspKernelError {
    if (src_rate_hz == 0 || dst_rate_hz == 0) {
        return DspKernelError.InvalidSampleRate;
    }
    if (half_taps == 0 || half_taps > 1024) {
        return DspKernelError.InvalidRatio;
    }
    if (src.len == 0) {
        return 0;
    }

    const src_len_u64: u64 = @as(u64, @intCast(src.len));
    const dst_rate_u64: u64 = @as(u64, dst_rate_hz);
    if (src_len_u64 > (0x7FFF_FFFF_FFFF_FFFF / dst_rate_u64)) {
        return DspKernelError.InvalidRatio;
    }
    const num: u64 = src_len_u64 * dst_rate_u64 + (@as(u64, src_rate_hz) / 2);
    let out_len_u64: u64 = num / @as(u64, src_rate_hz);
    if (out_len_u64 == 0) {
        out_len_u64 = 1;
    }
    const out_len: usize = @as(usize, @intCast(out_len_u64));
    if (dst.len < out_len) {
        return DspKernelError.OutputTooSmall;
    }

    // Fast exact pass-through when sample rates match.
    if (src_rate_hz == dst_rate_hz) {
        let i: usize = 0;
        while (i < out_len) : (i += 1) {
            dst[i] = src[i];
        }
        return out_len;
    }

    const ratio: f64 = @as(f64, @floatFromInt(dst_rate_hz)) / @as(f64, @floatFromInt(src_rate_hz));
    const inv_ratio: f64 = 1.0 / ratio;
    const cutoff: f64 = if (ratio < 1.0) ratio else 1.0;
    const win_half: f64 = @as(f64, @floatFromInt(half_taps)) / cutoff;
    if (!isFiniteF64(win_half) || win_half > 65536.0) {
        return DspKernelError.InvalidRatio;
    }
    const max_src_idx: i64 = @as(i64, @intCast(src.len - 1));

    // Chunked SIMD @dot buffers (64-lane chunks support arbitrary tap windows with zero heap)
    let w_chunk: [64]f32 = [0.0; 64];
    let s_chunk: [64]f32 = [0.0; 64];

    let out_i: usize = 0;
    while (out_i < out_len) : (out_i += 1) {
        const src_pos: f64 = @as(f64, @floatFromInt(out_i)) * inv_ratio;
        let k_start: i64 = @as(i64, @intFromFloat(std.math.floor(src_pos - win_half)));
        let k_end: i64 = @as(i64, @intFromFloat(std.math.ceil(src_pos + win_half)));

        if (k_start < 0) k_start = 0;
        if (k_end > max_src_idx) k_end = max_src_idx;

        let acc: f64 = 0.0;
        let w_sum: f64 = 0.0;
        let chunk_n: usize = 0;
        let k: i64 = k_start;
        while (k <= k_end) : (k += 1) {
            const d: f64 = @as(f64, @floatFromInt(k)) - src_pos;
            const ad: f64 = absF64(d);
            if (ad > win_half) {
                continue;
            }
            const win: f64 = 0.5 * (1.0 + std.math.cos(PI_F64 * d / win_half));
            const h: f64 = sincNormalized(d * cutoff) * win;
            const raw_s: f32 = src[@as(usize, @intCast(k))];
            s_chunk[chunk_n] = if (isFiniteF32(raw_s)) raw_s else 0.0;
            w_chunk[chunk_n] = h as f32;
            w_sum += h;
            chunk_n += 1;

            if (chunk_n == 64) {
                acc += @as(f64, @dot(s_chunk[0..64], w_chunk[0..64]));
                chunk_n = 0;
            }
        }
        if (chunk_n > 0) {
            acc += @as(f64, @dot(s_chunk[0..chunk_n], w_chunk[0..chunk_n]));
        }

        const out_val: f64 = if (absF64(w_sum) > 1.0e-12) acc / w_sum else 0.0;
        dst[out_i] = out_val as f32;
    }

    return out_len;
}

// ============================================================================
// 6. Professional Loudness & Dynamics Metering (ITU-R BS.1770 K-Weighted LUFS)
// ============================================================================

/// Broadcast-grade loudness and dynamics measurements for an audio buffer.
pub struct LoudnessMetrics {
    rms_dbfs: f64 = DEFAULT_DB_FLOOR;
    peak_dbfs: f64 = DEFAULT_DB_FLOOR;
    crest_factor_db: f64 = 0.0;
    k_weighted_lufs: f64 = DEFAULT_DB_FLOOR;
    dc_offset: f64 = 0.0;
    clipped_samples: usize = 0;
}

/// Measures RMS dBFS, sample peak dBFS, crest factor, DC offset, clipping count, and
/// ITU-R BS.1770 K-Weighted integrated loudness (LUFS) with -70 LUFS absolute block gating
/// across an arbitrary-length `f32` PCM slice with zero dynamic heap allocation.
pub fn measureAudioLoudnessF32(
    samples: []const f32,
    sample_rate_hz: f64,
): LoudnessMetrics ! DspKernelError {
    if (sample_rate_hz <= 0.0 || !isFiniteF64(sample_rate_hz)) {
        return DspKernelError.InvalidSampleRate;
    }
    if (samples.len == 0) {
        return LoudnessMetrics{};
    }

    let k_filter = BiquadCascade.designKWeighting(sample_rate_hz)?;

    // 400 ms gating block size per ITU-R BS.1770-4 (clamped to >= 1 sample).
    let block_len: usize = @as(usize, @intFromFloat(sample_rate_hz * 0.4));
    if (block_len == 0) {
        block_len = 1;
    }

    // Absolute gate threshold: -70.0 LUFS => mean_sq_k >= 10^((-70.0 + 0.691) / 10.0).
    const abs_gate_mean_sq: f64 = std.math.pow(f64, 10.0, (-70.0 + 0.691) / 10.0);

    let sum_x: f64 = 0.0;
    let sum_sq: f64 = 0.0;
    let peak_abs: f64 = 0.0;
    let clipped: usize = 0;

    let total_k_sq: f64 = 0.0;
    let block_k_sq: f64 = 0.0;
    let block_count_in_cur: usize = 0;
    let gated_block_sum: f64 = 0.0;
    let gated_block_count: usize = 0;

    let clean_chunk: [64]f32 = [0.0; 64];
    let chunk_n: usize = 0;

    let i: usize = 0;
    while (i < samples.len) : (i += 1) {
        const raw: f32 = samples[i];
        const finite_f32: f32 = if (isFiniteF32(raw)) raw else 0.0;
        const x: f64 = @as(f64, finite_f32);
        const ax: f64 = absF64(x);

        sum_x += x;
        clean_chunk[chunk_n] = finite_f32;
        chunk_n += 1;
        if (chunk_n == 64) {
            sum_sq += @as(f64, @dot(clean_chunk[0..64], clean_chunk[0..64]));
            chunk_n = 0;
        }
        if (ax > peak_abs) {
            peak_abs = ax;
        }
        if (ax >= CLIP_THRESHOLD_F64) {
            clipped += 1;
        }

        const y_k: f64 = k_filter.processSample(x);
        const y_k_sq: f64 = y_k * y_k;
        total_k_sq += y_k_sq;
        block_k_sq += y_k_sq;
        block_count_in_cur += 1;

        if (block_count_in_cur >= block_len) {
            const mean_block: f64 = block_k_sq / @as(f64, @floatFromInt(block_count_in_cur));
            if (mean_block >= abs_gate_mean_sq) {
                gated_block_sum += mean_block;
                gated_block_count += 1;
            }
            block_k_sq = 0.0;
            block_count_in_cur = 0;
        }
    }
    if (chunk_n > 0) {
        sum_sq += @as(f64, @dot(clean_chunk[0..chunk_n], clean_chunk[0..chunk_n]));
    }

    // Handle trailing partial block (or short buffers < 400 ms).
    if (block_count_in_cur > 0 && gated_block_count == 0) {
        const mean_tail: f64 = block_k_sq / @as(f64, @floatFromInt(block_count_in_cur));
        if (mean_tail >= abs_gate_mean_sq) {
            gated_block_sum += mean_tail;
            gated_block_count += 1;
        }
    }

    const n_f64: f64 = @as(f64, @floatFromInt(samples.len));
    const dc_offset: f64 = sum_x / n_f64;
    const rms: f64 = std.math.sqrt(sum_sq / n_f64);
    const rms_dbfs: f64 = ampToDb(rms, DEFAULT_DB_FLOOR);
    const peak_dbfs: f64 = ampToDb(peak_abs, DEFAULT_DB_FLOOR);
    const crest_factor_db: f64 = if (peak_abs > 0.0 && rms > 0.0) peak_dbfs - rms_dbfs else 0.0;

    let k_weighted_lufs: f64 = DEFAULT_DB_FLOOR;
    let effective_k_mean_sq: f64 = 0.0;
    if (gated_block_count > 0) {
        effective_k_mean_sq = gated_block_sum / @as(f64, @floatFromInt(gated_block_count));
    } else {
        effective_k_mean_sq = total_k_sq / n_f64;
    }

    if (effective_k_mean_sq > 0.0 && isFiniteF64(effective_k_mean_sq)) {
        const raw_lufs: f64 = -0.691 + 10.0 * std.math.log10(effective_k_mean_sq);
        k_weighted_lufs = if (raw_lufs < DEFAULT_DB_FLOOR) DEFAULT_DB_FLOOR else raw_lufs;
    }

    return LoudnessMetrics{
        rms_dbfs: rms_dbfs,
        peak_dbfs: peak_dbfs,
        crest_factor_db: crest_factor_db,
        k_weighted_lufs: k_weighted_lufs,
        dc_offset: dc_offset,
        clipped_samples: clipped,
    };
}

// ============================================================================
// 7. Native SIMD `@dot` DSP Primitives (`dotF32Simd`, `convolveFirF32Simd`)
// ============================================================================

/// Computes the hardware SIMD `@dot` inner product of two `f32` slices (bounded by `min(a.len, b.len)`).
pub fn dotF32Simd(a: []const f32, b: []const f32): f32 {
    const n: usize = if (a.len < b.len) a.len else b.len;
    if (n == 0) return 0.0;
    return @dot(a[0..n], b[0..n]);
}

/// Applies a causal Finite Impulse Response (FIR) filter `y[n] = sum_{k=0}^{M-1} coeffs[k] * input[n - k]`
/// using chunked native SIMD `@dot` acceleration with zero dynamic heap allocation and arbitrary tap count.
/// Returns the number of output samples written (`input.len`).
pub fn convolveFirF32Simd(
    input: []const f32,
    coeffs: []const f32,
    out: *[]f32,
): usize ! DspKernelError {
    if (coeffs.len == 0) return DspKernelError.LengthMismatch;
    if (out.len < input.len) return DspKernelError.OutputTooSmall;
    if (input.len == 0) return 0;

    // Fast path: pre-reverse up to 256 taps ONCE outside the sample loop so every steady-state
    // sample evaluates a single contiguous `@dot` SIMD inner product with zero per-sample copying.
    if (coeffs.len <= 256) {
        let rev_all: [256]f32 = [0.0; 256];
        const m: usize = coeffs.len;
        let k: usize = 0;
        while (k < m) : (k += 1) {
            rev_all[k] = coeffs[m - 1 - k];
        }
        const rev_full: []const f32 = rev_all[0..m];
        let n: usize = 0;
        while (n < input.len) : (n += 1) {
            if (n + 1 >= m) {
                const start: usize = n + 1 - m;
                out[n] = @dot(input[start..start + m], rev_full);
            } else {
                const take: usize = n + 1;
                out[n] = @dot(input[0..take], rev_all[m - take..m]);
            }
        }
        return input.len;
    }

    let rev_chunk: [64]f32 = [0.0; 64];
    let n: usize = 0;
    while (n < input.len) : (n += 1) {
        const active_taps: usize = if (n + 1 < coeffs.len) n + 1 else coeffs.len;
        let acc: f64 = 0.0;
        let tap_off: usize = 0;
        while (tap_off < active_taps) {
            const rem: usize = active_taps - tap_off;
            const take: usize = if (rem > 64) 64 else rem;
            // Input contiguous slice: input[n - tap_off - take + 1 .. n - tap_off + 1]
            // Corresponding coeffs in reverse order: coeffs[tap_off + take - 1 .. tap_off]
            const in_start: usize = n + 1 - tap_off - take;
            let j: usize = 0;
            while (j < take) : (j += 1) {
                rev_chunk[j] = coeffs[tap_off + take - 1 - j];
            }
            acc += @as(f64, @dot(input[in_start..in_start + take], rev_chunk[0..take]));
            tap_off += take;
        }
        out[n] = acc as f32;
    }
    return input.len;
}
