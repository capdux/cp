// Origin: Pure-CP Stdlib (Whisper/Conformer 80/128-Bin Log-Mel, MFCC+Delta/Delta-Delta, CMVN & Streaming Endpointing VAD) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice acoustic feature frontend for ASR/TTS:
//   - HTK & Slaney (Whisper/librosa) Mel frequency scale conversion and equal-area triangular filterbank builder
//   - Whisper/Conformer Log-Mel spectrogram extractor powered by `fftRadix2InPlace` + native SIMD `@dot`
//     with Whisper `log10` dynamic-range clamping (`max - 8.0`, `(x + 4.0) / 4.0`) or natural-log mode
//   - Orthonormal DCT-II MFCC cepstral extractor, regression-slope Delta / Delta-Delta, and in-place CMVN
//   - Streaming dual-threshold ASR Endpointing VAD combining short-time RMS energy, adaptive noise floor,
//     zero-crossing rate (ZCR), Wiener spectral flatness, and onset/hangover hysteresis state machine

const std = @import("std");
import { fftRadix2InPlace } from "./fftFilter.cp";

// ============================================================================
// 1. Constants, Error Set, Enums & Configuration
// ============================================================================

const ASR_PI: f64 = 3.14159265358979323846;
const ASR_TWO_PI: f64 = 6.28318530717958647692;
const ASR_SLANEY_F_SP: f64 = 200.0 / 3.0;
const ASR_SLANEY_MIN_LOG_HZ: f64 = 1000.0;
const ASR_SLANEY_MIN_LOG_MEL: f64 = 15.0; // 1000.0 / (200.0 / 3.0)
const ASR_SLANEY_LOGSTEP: f64 = 0.06875177742094912; // ln(6.4) / 27.0

/// Error set for ASR acoustic feature extraction, MFCC/CMVN, and endpointing VAD.
pub err AsrFeatureError {
    InvalidSampleRate,
    InvalidFftSize,
    InvalidHopLength,
    InvalidMelBins,
    InvalidMfccCount,
    InvalidFreqRange,
    BufferTooSmall,
    InputTooShort,
}

/// Mel frequency scale convention.
pub enum MelScaleKind {
    /// Kaldi / HTK formula: `2595 * log10(1 + f / 700)`.
    htk,
    /// Slaney / librosa / Whisper formula: linear below 1000 Hz, log above 1000 Hz, with equal-area normalization.
    slaney,
}

/// Configuration for the Whisper / Conformer acoustic feature extraction frontend.
pub struct AsrFrontendConfig {
    sample_rate_hz: u32 = 16000;
    n_fft: usize = 512;
    win_length: usize = 400;
    hop_length: usize = 160;
    n_mels: usize = 80;
    f_min_hz: f64 = 0.0;
    f_max_hz: f64 = 8000.0;
    pre_emphasis: f32 = 0.0;
    mel_scale: MelScaleKind = MelScaleKind.slaney;
    whisper_log10_clamp: bool = true;

    /// OpenAI Whisper v1/v2 standard 80-bin 16 kHz frontend defaults (25 ms window, 10 ms hop, Slaney Mel).
    static fn whisperDefaults(): AsrFrontendConfig {
        return AsrFrontendConfig{
            sample_rate_hz: 16000,
            n_fft: 512,
            win_length: 400,
            hop_length: 160,
            n_mels: 80,
            f_min_hz: 0.0,
            f_max_hz: 8000.0,
            pre_emphasis: 0.0,
            mel_scale: MelScaleKind.slaney,
            whisper_log10_clamp: true,
        };
    }

    /// OpenAI Whisper Large-v3 128-bin 16 kHz frontend defaults.
    static fn whisperLargeV3Defaults(): AsrFrontendConfig {
        return AsrFrontendConfig{
            sample_rate_hz: 16000,
            n_fft: 512,
            win_length: 400,
            hop_length: 160,
            n_mels: 128,
            f_min_hz: 0.0,
            f_max_hz: 8000.0,
            pre_emphasis: 0.0,
            mel_scale: MelScaleKind.slaney,
            whisper_log10_clamp: true,
        };
    }

    /// Returns the effective analysis window length (`win_length` clamped to `n_fft`; `0` defaults to `n_fft`).
    fn effectiveWinLen(): usize {
        if (win_length == 0 || win_length > n_fft) return n_fft;
        return win_length;
    }

    /// Returns the number of single-sided FFT frequency bins (`n_fft / 2 + 1`).
    fn nBins(): usize {
        return (n_fft / 2) + 1;
    }

    /// Validates all frontend parameters.
    fn isValid(): bool {
        if (sample_rate_hz == 0) return false;
        if (!isPow2(n_fft) || n_fft < 16) return false;
        if (hop_length == 0 || hop_length > n_fft) return false;
        if (n_mels == 0) return false;
        const nyquist: f64 = (sample_rate_hz as f64) * 0.5;
        if (f_min_hz < 0.0 || f_max_hz <= f_min_hz || f_max_hz > nyquist + 1.0e-6) return false;
        return true;
    }
}

fn isPow2(n: usize): bool {
    return n > 0 && ((n & (n - 1)) == 0);
}

fn validateConfig(cfg: AsrFrontendConfig): void ! AsrFeatureError {
    if (cfg.sample_rate_hz == 0) return AsrFeatureError.InvalidSampleRate;
    if (!isPow2(cfg.n_fft) || cfg.n_fft < 16) return AsrFeatureError.InvalidFftSize;
    if (cfg.hop_length == 0 || cfg.hop_length > cfg.n_fft) return AsrFeatureError.InvalidHopLength;
    if (cfg.n_mels == 0) return AsrFeatureError.InvalidMelBins;
    const nyquist: f64 = (cfg.sample_rate_hz as f64) * 0.5;
    if (cfg.f_min_hz < 0.0 || cfg.f_max_hz <= cfg.f_min_hz || cfg.f_max_hz > nyquist + 1.0e-6) {
        return AsrFeatureError.InvalidFreqRange;
    }
}

/// Exact number of STFT frames extracted from `num_samples` with window/FFT size `n_fft` and stride `hop_length`.
pub fn asrFrameCount(num_samples: usize, n_fft: usize, hop_length: usize): usize {
    if (n_fft == 0 || hop_length == 0 || num_samples < n_fft) return 0;
    return ((num_samples - n_fft) / hop_length) + 1;
}

/// Exact `f32` buffer length required for the Mel filterbank weight matrix (`n_mels * (n_fft / 2 + 1)`).
pub fn asrMelBankLen(n_mels: usize, n_fft: usize): usize {
    return n_mels * ((n_fft / 2) + 1);
}

/// Exact `f32` buffer length required for the Log-Mel spectrogram output (`num_frames * n_mels`).
pub fn asrMelOutLen(num_frames: usize, n_mels: usize): usize {
    return num_frames * n_mels;
}

// ============================================================================
// 2. HTK / Slaney Mel Scale & Triangular Filterbank Matrix Builder
// ============================================================================

/// Converts frequency in Hz to Mel scale (`MelScaleKind.htk` or `MelScaleKind.slaney`).
pub fn hzToMelScale(hz: f64, kind: MelScaleKind): f64 {
    const f: f64 = if (hz < 0.0) 0.0 else hz;
    return match (kind) {
        MelScaleKind.htk => 2595.0 * std.math.log10(1.0 + f / 700.0),
        MelScaleKind.slaney => if (f < ASR_SLANEY_MIN_LOG_HZ)
            (f / ASR_SLANEY_F_SP)
        else
            (ASR_SLANEY_MIN_LOG_MEL + @log(f / ASR_SLANEY_MIN_LOG_HZ) / ASR_SLANEY_LOGSTEP),
    };
}

/// Converts Mel scale value back to frequency in Hz (`MelScaleKind.htk` or `MelScaleKind.slaney`).
pub fn melScaleToHz(mel: f64, kind: MelScaleKind): f64 {
    const m: f64 = if (mel < 0.0) 0.0 else mel;
    return match (kind) {
        MelScaleKind.htk => 700.0 * (std.math.pow(f64, 10.0, m / 2595.0) - 1.0),
        MelScaleKind.slaney => if (m < ASR_SLANEY_MIN_LOG_MEL)
            (m * ASR_SLANEY_F_SP)
        else
            (ASR_SLANEY_MIN_LOG_HZ * std.math.exp((m - ASR_SLANEY_MIN_LOG_MEL) * ASR_SLANEY_LOGSTEP)),
    };
}

/// Builds a contiguous `[n_mels][n_fft/2 + 1]` triangular Mel filterbank weight matrix into `out_weights`.
/// When `cfg.mel_scale == MelScaleKind.slaney`, applies Whisper/librosa equal-area normalization
/// `enorm = 2.0 / (f_right - f_left)` to each triangular band.
pub fn buildMelFilterbankMatrix(
    cfg: AsrFrontendConfig,
    out_weights: []f32,
): usize ! AsrFeatureError {
    validateConfig(cfg)?;
    const n_bins: usize = cfg.nBins();
    const total_len: usize = cfg.n_mels * n_bins;
    if (out_weights.len < total_len) {
        return AsrFeatureError.BufferTooSmall;
    }

    const mel_min: f64 = hzToMelScale(cfg.f_min_hz, cfg.mel_scale);
    const mel_max: f64 = hzToMelScale(cfg.f_max_hz, cfg.mel_scale);
    const mel_span: f64 = mel_max - mel_min;
    const n_pts: usize = cfg.n_mels + 2;
    const bin_hz_step: f64 = (cfg.sample_rate_hz as f64) / (cfg.n_fft as f64);

    let m: usize = 0;
    while (m < cfg.n_mels) : (m += 1) {
        const mel_l: f64 = mel_min + mel_span * ((m as f64) / ((n_pts - 1) as f64));
        const mel_c: f64 = mel_min + mel_span * (((m + 1) as f64) / ((n_pts - 1) as f64));
        const mel_r: f64 = mel_min + mel_span * (((m + 2) as f64) / ((n_pts - 1) as f64));

        const f_l: f64 = melScaleToHz(mel_l, cfg.mel_scale);
        const f_c: f64 = melScaleToHz(mel_c, cfg.mel_scale);
        const f_r: f64 = melScaleToHz(mel_r, cfg.mel_scale);

        const enorm: f64 = if (cfg.mel_scale == MelScaleKind.slaney && (f_r - f_l) > 1.0e-9)
            (2.0 / (f_r - f_l))
        else
            1.0;

        const row_off: usize = m * n_bins;
        let k: usize = 0;
        while (k < n_bins) : (k += 1) {
            const fk: f64 = (k as f64) * bin_hz_step;
            let w: f64 = 0.0;
            if (fk >= f_l && fk <= f_c && (f_c - f_l) > 1.0e-12) {
                w = (fk - f_l) / (f_c - f_l);
            } else if (fk > f_c && fk <= f_r && (f_r - f_c) > 1.0e-12) {
                w = (f_r - fk) / (f_r - f_c);
            }
            out_weights[row_off + k] = (w * enorm) as f32;
        }
    }

    return total_len;
}

// ============================================================================
// 3. Whisper/Conformer Log-Mel Spectrogram Extractor (`@dot` Accelerated)
// ============================================================================

fn computeFramePowerSpectrum(
    pcm: []const f32,
    frame_start: usize,
    cfg: AsrFrontendConfig,
    fft_re: []f64,
    fft_im: []f64,
    power_buf: []f32,
): void ! AsrFeatureError {
    const n_fft: usize = cfg.n_fft;
    const win_len: usize = cfg.effectiveWinLen();
    const n_bins: usize = cfg.nBins();
    const alpha: f32 = cfg.pre_emphasis;
    const win_denom: f64 = win_len as f64;

    let i: usize = 0;
    while (i < win_len) : (i += 1) {
        const cur: f32 = pcm[frame_start + i];
        let sample: f32 = cur;
        if (alpha != 0.0 && i > 0) {
            sample = cur - alpha * pcm[frame_start + i - 1];
        }
        // Periodic Hann window (matching PyTorch/Whisper `torch.hann_window(N, periodic=True)`)
        const hann: f64 = 0.5 * (1.0 - @cos(ASR_TWO_PI * (i as f64) / win_denom));
        fft_re[i] = (sample as f64) * hann;
        fft_im[i] = 0.0;
    }
    while (i < n_fft) : (i += 1) {
        fft_re[i] = 0.0;
        fft_im[i] = 0.0;
    }

    let re_s: []f64 = fft_re[0..n_fft];
    let im_s: []f64 = fft_im[0..n_fft];
    fftRadix2InPlace(*re_s, *im_s) catch {
        return AsrFeatureError.InvalidFftSize;
    };

    let k: usize = 0;
    while (k < n_bins) : (k += 1) {
        const re_v: f64 = fft_re[k];
        const im_v: f64 = fft_im[k];
        power_buf[k] = (re_v * re_v + im_v * im_v) as f32;
    }
}

/// Extracts a `[num_frames][n_mels]` Log-Mel spectrogram from arbitrary-length `pcm` using caller-provided
/// scratch buffers (`fft_re`, `fft_im`, `power_buf`) and prebuilt `mel_weights`, accelerated by native `@dot`.
///
/// When `cfg.whisper_log10_clamp` is `true`, applies OpenAI Whisper's exact `log10(max(e, 1e-10))`,
/// `max(v, max_v - 8.0)`, and `(v + 4.0) / 4.0` normalization; otherwise outputs natural log `ln(max(e, 1e-10))`.
pub fn extractLogMelSpectrogram(
    pcm: []const f32,
    cfg: AsrFrontendConfig,
    mel_weights: []const f32,
    fft_re: []f64,
    fft_im: []f64,
    power_buf: []f32,
    out_log_mel: []f32,
): usize ! AsrFeatureError {
    validateConfig(cfg)?;
    if (pcm.len < cfg.n_fft) {
        return AsrFeatureError.InputTooShort;
    }

    const n_bins: usize = cfg.nBins();
    const num_frames: usize = asrFrameCount(pcm.len, cfg.n_fft, cfg.hop_length);
    if (mel_weights.len < cfg.n_mels * n_bins
        || fft_re.len < cfg.n_fft
        || fft_im.len < cfg.n_fft
        || power_buf.len < n_bins
        || out_log_mel.len < num_frames * cfg.n_mels)
    {
        return AsrFeatureError.BufferTooSmall;
    }

    let global_max: f32 = -1.0e30;
    let f: usize = 0;
    while (f < num_frames) : (f += 1) {
        computeFramePowerSpectrum(pcm, f * cfg.hop_length, cfg, fft_re, fft_im, power_buf)?;

        const out_off: usize = f * cfg.n_mels;
        const pow_slice: []const f32 = power_buf[0..n_bins];
        let m: usize = 0;
        while (m < cfg.n_mels) : (m += 1) {
            const w_row: []const f32 = mel_weights[m * n_bins .. (m + 1) * n_bins];
            const energy: f32 = @dot(pow_slice, w_row);
            const clamped: f64 = if (energy > 1.0e-10) (energy as f64) else 1.0e-10;
            const log_val: f32 = if (cfg.whisper_log10_clamp)
                (std.math.log10(clamped) as f32)
            else
                (@log(clamped) as f32);
            out_log_mel[out_off + m] = log_val;
            if (log_val > global_max) {
                global_max = log_val;
            }
        }
    }

    if (cfg.whisper_log10_clamp) {
        const floor_val: f32 = global_max - 8.0;
        const total: usize = num_frames * cfg.n_mels;
        let idx: usize = 0;
        while (idx < total) : (idx += 1) {
            const v: f32 = if (out_log_mel[idx] < floor_val) floor_val else out_log_mel[idx];
            out_log_mel[idx] = (v + 4.0) * 0.25;
        }
    }

    return num_frames;
}

// ============================================================================
// 4. MFCC (Orthonormal DCT-II), Delta / Delta-Delta & In-Place CMVN
// ============================================================================

/// Computes `[num_frames][n_mfcc]` MFCC cepstral coefficients from `[num_frames][n_mels]` Log-Mel frames
/// using orthonormal DCT-II projection.
pub fn computeMfccFromLogMel(
    log_mel_frames: []const f32,
    num_frames: usize,
    n_mels: usize,
    n_mfcc: usize,
    out_mfcc: []f32,
): void ! AsrFeatureError {
    if (n_mels == 0) return AsrFeatureError.InvalidMelBins;
    if (n_mfcc == 0 || n_mfcc > n_mels) return AsrFeatureError.InvalidMfccCount;
    if (num_frames == 0) return AsrFeatureError.InputTooShort;
    if (log_mel_frames.len < num_frames * n_mels || out_mfcc.len < num_frames * n_mfcc) {
        return AsrFeatureError.BufferTooSmall;
    }

    const n_mels_f: f64 = n_mels as f64;
    const scale0: f64 = std.math.sqrt(1.0 / n_mels_f);
    const scalek: f64 = std.math.sqrt(2.0 / n_mels_f);

    let f: usize = 0;
    while (f < num_frames) : (f += 1) {
        const in_off: usize = f * n_mels;
        const out_off: usize = f * n_mfcc;
        let k: usize = 0;
        while (k < n_mfcc) : (k += 1) {
            const kf: f64 = k as f64;
            let sum: f64 = 0.0;
            let m: usize = 0;
            while (m < n_mels) : (m += 1) {
                const angle: f64 = ASR_PI * kf * ((m as f64) + 0.5) / n_mels_f;
                sum += (log_mel_frames[in_off + m] as f64) * @cos(angle);
            }
            const sc: f64 = if (k == 0) scale0 else scalek;
            out_mfcc[out_off + k] = (sum * sc) as f32;
        }
    }
}

/// Computes first-order Delta (or second-order Delta-Delta when applied to Delta) regression slopes:
/// `delta[t, d] = sum_{n=1..R} n * (c[t+n, d] - c[t-n, d]) / (2 * sum_{n=1..R} n^2)`
/// with edge replication at sequence boundaries.
pub fn computeDeltaFeatures(
    feats: []const f32,
    num_frames: usize,
    feat_dim: usize,
    window_radius: usize,
    out_delta: []f32,
): void ! AsrFeatureError {
    if (num_frames == 0) return AsrFeatureError.InputTooShort;
    if (feat_dim == 0 || window_radius == 0) return AsrFeatureError.InvalidMfccCount;
    if (feats.len < num_frames * feat_dim || out_delta.len < num_frames * feat_dim) {
        return AsrFeatureError.BufferTooSmall;
    }

    let denom_sq: f64 = 0.0;
    let r: usize = 1;
    while (r <= window_radius) : (r += 1) {
        const rf: f64 = r as f64;
        denom_sq += rf * rf;
    }
    const inv_denom: f64 = 1.0 / (2.0 * denom_sq);

    let t: usize = 0;
    while (t < num_frames) : (t += 1) {
        const out_off: usize = t * feat_dim;
        let d: usize = 0;
        while (d < feat_dim) : (d += 1) {
            let num: f64 = 0.0;
            let n: usize = 1;
            while (n <= window_radius) : (n += 1) {
                const t_plus: usize = if (t + n < num_frames) (t + n) else (num_frames - 1);
                const t_minus: usize = if (t >= n) (t - n) else 0;
                const diff: f64 = (feats[t_plus * feat_dim + d] as f64) - (feats[t_minus * feat_dim + d] as f64);
                num += (n as f64) * diff;
            }
            out_delta[out_off + d] = (num * inv_denom) as f32;
        }
    }
}

/// Applies Cepstral Mean and Variance Normalization (CMVN) in place across the time axis for each
/// feature dimension `d in 0..feat_dim`, removing channel bias and gain mismatch.
pub fn applyCmvnInPlace(
    feats: []f32,
    num_frames: usize,
    feat_dim: usize,
    normalize_variance: bool,
): void ! AsrFeatureError {
    if (num_frames == 0) return AsrFeatureError.InputTooShort;
    if (feat_dim == 0) return AsrFeatureError.InvalidMfccCount;
    if (feats.len < num_frames * feat_dim) return AsrFeatureError.BufferTooSmall;

    const inv_t: f64 = 1.0 / (num_frames as f64);
    let d: usize = 0;
    while (d < feat_dim) : (d += 1) {
        let sum: f64 = 0.0;
        let t: usize = 0;
        while (t < num_frames) : (t += 1) {
            sum += feats[t * feat_dim + d] as f64;
        }
        const mean: f64 = sum * inv_t;

        if (!normalize_variance) {
            t = 0;
            while (t < num_frames) : (t += 1) {
                feats[t * feat_dim + d] = ((feats[t * feat_dim + d] as f64) - mean) as f32;
            }
        } else {
            let var_sum: f64 = 0.0;
            t = 0;
            while (t < num_frames) : (t += 1) {
                const diff: f64 = (feats[t * feat_dim + d] as f64) - mean;
                var_sum += diff * diff;
            }
            const inv_std: f64 = 1.0 / std.math.sqrt(var_sum * inv_t + 1.0e-10);
            t = 0;
            while (t < num_frames) : (t += 1) {
                const centered: f64 = (feats[t * feat_dim + d] as f64) - mean;
                feats[t * feat_dim + d] = (centered * inv_std) as f32;
            }
        }
    }
}

/// Applies HTK/Kaldi sinusoidal cepstral liftering `w_k = 1 + (L/2) * sin(pi * k / L)` in place
/// to `[num_frames][n_mfcc]` MFCC frames (`lifter_coeff` typically `22.0`).
pub fn applyCepstralLifteringInPlace(
    mfcc: []f32,
    num_frames: usize,
    n_mfcc: usize,
    lifter_coeff: f32,
): void ! AsrFeatureError {
    if (num_frames == 0) return AsrFeatureError.InputTooShort;
    if (n_mfcc == 0) return AsrFeatureError.InvalidMfccCount;
    if (mfcc.len < num_frames * n_mfcc) return AsrFeatureError.BufferTooSmall;
    if (lifter_coeff <= 0.0) return;

    const l_f: f64 = lifter_coeff as f64;
    const half_l: f64 = l_f * 0.5;
    let k: usize = 0;
    while (k < n_mfcc) : (k += 1) {
        const w: f32 = (1.0 + half_l * @sin(ASR_PI * (k as f64) / l_f)) as f32;
        let f: usize = 0;
        while (f < num_frames) : (f += 1) {
            mfcc[f * n_mfcc + k] *= w;
        }
    }
}

/// Builds a reusable `[n_mfcc][n_mels]` orthonormal DCT-II projection matrix into `out_dct`
/// for SIMD `@dot`-accelerated MFCC extraction via `computeMfccFastDot`.
pub fn buildDct2Matrix(
    n_mels: usize,
    n_mfcc: usize,
    out_dct: []f32,
): usize ! AsrFeatureError {
    if (n_mels == 0) return AsrFeatureError.InvalidMelBins;
    if (n_mfcc == 0 || n_mfcc > n_mels) return AsrFeatureError.InvalidMfccCount;
    const total: usize = n_mfcc * n_mels;
    if (out_dct.len < total) return AsrFeatureError.BufferTooSmall;

    const n_mels_f: f64 = n_mels as f64;
    const scale0: f64 = std.math.sqrt(1.0 / n_mels_f);
    const scalek: f64 = std.math.sqrt(2.0 / n_mels_f);

    let k: usize = 0;
    while (k < n_mfcc) : (k += 1) {
        const kf: f64 = k as f64;
        const sc: f64 = if (k == 0) scale0 else scalek;
        const row_off: usize = k * n_mels;
        let m: usize = 0;
        while (m < n_mels) : (m += 1) {
            const angle: f64 = ASR_PI * kf * ((m as f64) + 0.5) / n_mels_f;
            out_dct[row_off + m] = (sc * @cos(angle)) as f32;
        }
    }
    return total;
}

/// SIMD `@dot`-accelerated MFCC extraction using a prebuilt `[n_mfcc][n_mels]` DCT-II matrix.
pub fn computeMfccFastDot(
    log_mel_frames: []const f32,
    dct_matrix: []const f32,
    num_frames: usize,
    n_mels: usize,
    n_mfcc: usize,
    out_mfcc: []f32,
): void ! AsrFeatureError {
    if (n_mels == 0) return AsrFeatureError.InvalidMelBins;
    if (n_mfcc == 0 || n_mfcc > n_mels) return AsrFeatureError.InvalidMfccCount;
    if (num_frames == 0) return AsrFeatureError.InputTooShort;
    if (log_mel_frames.len < num_frames * n_mels
        || dct_matrix.len < n_mfcc * n_mels
        || out_mfcc.len < num_frames * n_mfcc)
    {
        return AsrFeatureError.BufferTooSmall;
    }

    let f: usize = 0;
    while (f < num_frames) : (f += 1) {
        const mel_row: []const f32 = log_mel_frames[f * n_mels .. (f + 1) * n_mels];
        const out_off: usize = f * n_mfcc;
        let k: usize = 0;
        while (k < n_mfcc) : (k += 1) {
            const dct_row: []const f32 = dct_matrix[k * n_mels .. (k + 1) * n_mels];
            out_mfcc[out_off + k] = @dot(mel_row, dct_row);
        }
    }
}

/// Packs static MFCC, first-order Delta, and second-order Delta-Delta into a contiguous
/// `[num_frames][3 * n_mfcc]` feature tensor (e.g. classic 39-dim ASR acoustic vectors for `n_mfcc = 13`).
pub fn packMfccWithDeltas(
    mfcc: []const f32,
    delta: []const f32,
    delta2: []const f32,
    num_frames: usize,
    n_mfcc: usize,
    out_packed: []f32,
): usize ! AsrFeatureError {
    if (num_frames == 0) return AsrFeatureError.InputTooShort;
    if (n_mfcc == 0) return AsrFeatureError.InvalidMfccCount;
    const base_len: usize = num_frames * n_mfcc;
    const out_dim: usize = n_mfcc * 3;
    const out_len: usize = num_frames * out_dim;
    if (mfcc.len < base_len || delta.len < base_len || delta2.len < base_len || out_packed.len < out_len) {
        return AsrFeatureError.BufferTooSmall;
    }

    let f: usize = 0;
    while (f < num_frames) : (f += 1) {
        const in_off: usize = f * n_mfcc;
        const out_off: usize = f * out_dim;
        let d: usize = 0;
        while (d < n_mfcc) : (d += 1) {
            out_packed[out_off + d] = mfcc[in_off + d];
            out_packed[out_off + n_mfcc + d] = delta[in_off + d];
            out_packed[out_off + n_mfcc * 2 + d] = delta2[in_off + d];
        }
    }
    return out_len;
}

/// Deterministic SpecAugment (Park et al., 2019) frequency and time masking over a `[num_frames][n_mels]`
/// Log-Mel spectrogram in place, filling masked spans with `mask_value`.
pub fn applySpecAugmentInPlace(
    log_mel: []f32,
    num_frames: usize,
    n_mels: usize,
    num_freq_masks: usize,
    max_freq_width: usize,
    num_time_masks: usize,
    max_time_width: usize,
    mask_value: f32,
    seed: u64,
): void ! AsrFeatureError {
    if (num_frames == 0) return AsrFeatureError.InputTooShort;
    if (n_mels == 0) return AsrFeatureError.InvalidMelBins;
    if (log_mel.len < num_frames * n_mels) return AsrFeatureError.BufferTooSmall;

    let rng: u64 = if (seed == 0) 0x9E3779B97F4A7C15 else seed;

    // 1. Frequency masks
    let fm: usize = 0;
    while (fm < num_freq_masks) : (fm += 1) {
        if (max_freq_width == 0) break;
        rng = rng +% 0x9E3779B97F4A7C15;
        let z1: u64 = (rng ^ (rng >> 30)) *% 0xBF58476D1CE4E5B9;
        z1 = (z1 ^ (z1 >> 27)) *% 0x94D049BB133111EB;
        z1 = z1 ^ (z1 >> 31);

        const cap_w: usize = if (max_freq_width < n_mels) max_freq_width else n_mels;
        const w: usize = @as(usize, @intCast(z1 % @as(u64, cap_w + 1)));
        if (w == 0) continue;

        rng = rng +% 0x9E3779B97F4A7C15;
        let z2: u64 = (rng ^ (rng >> 30)) *% 0xBF58476D1CE4E5B9;
        z2 = (z2 ^ (z2 >> 27)) *% 0x94D049BB133111EB;
        z2 = z2 ^ (z2 >> 31);
        const f0: usize = @as(usize, @intCast(z2 % @as(u64, (n_mels - w) + 1)));

        let t: usize = 0;
        while (t < num_frames) : (t += 1) {
            const row: usize = t * n_mels;
            let m: usize = f0;
            while (m < f0 + w) : (m += 1) {
                log_mel[row + m] = mask_value;
            }
        }
    }

    // 2. Time masks
    let tm: usize = 0;
    while (tm < num_time_masks) : (tm += 1) {
        if (max_time_width == 0) break;
        rng = rng +% 0x9E3779B97F4A7C15;
        let z1: u64 = (rng ^ (rng >> 30)) *% 0xBF58476D1CE4E5B9;
        z1 = (z1 ^ (z1 >> 27)) *% 0x94D049BB133111EB;
        z1 = z1 ^ (z1 >> 31);

        const cap_w: usize = if (max_time_width < num_frames) max_time_width else num_frames;
        const w: usize = @as(usize, @intCast(z1 % @as(u64, cap_w + 1)));
        if (w == 0) continue;

        rng = rng +% 0x9E3779B97F4A7C15;
        let z2: u64 = (rng ^ (rng >> 30)) *% 0xBF58476D1CE4E5B9;
        z2 = (z2 ^ (z2 >> 27)) *% 0x94D049BB133111EB;
        z2 = z2 ^ (z2 >> 31);
        const t0: usize = @as(usize, @intCast(z2 % @as(u64, (num_frames - w) + 1)));

        let t: usize = t0;
        while (t < t0 + w) : (t += 1) {
            const row: usize = t * n_mels;
            let m: usize = 0;
            while (m < n_mels) : (m += 1) {
                log_mel[row + m] = mask_value;
            }
        }
    }
}

/// Causal online Welford mean/variance normalizer backed by caller-provided slices (`mean_buf`, `m2_buf`)
/// for low-latency streaming ASR without full-utterance lookahead.
pub struct StreamingCmvnNormalizer {
    feat_dim: usize = 0;
    frame_count: u64 = 0;
    mean_buf: []f64 = [],
    m2_buf: []f64 = [],

    static fn init(feat_dim: usize, mean_buf: []f64, m2_buf: []f64): StreamingCmvnNormalizer ! AsrFeatureError {
        if (feat_dim == 0) return AsrFeatureError.InvalidMfccCount;
        if (mean_buf.len < feat_dim || m2_buf.len < feat_dim) return AsrFeatureError.BufferTooSmall;
        let i: usize = 0;
        while (i < feat_dim) : (i += 1) {
            mean_buf[i] = 0.0;
            m2_buf[i] = 0.0;
        }
        return StreamingCmvnNormalizer{
            feat_dim: feat_dim,
            frame_count: 0,
            mean_buf: mean_buf,
            m2_buf: m2_buf,
        };
    }

    /// Updates running Welford statistics with `frame` and normalizes `frame` in place.
    fn normalizeFrameInPlace(*_, frame: []f32, normalize_variance: bool): void ! AsrFeatureError {
        if (frame.len < feat_dim) return AsrFeatureError.BufferTooSmall;
        frame_count += 1;
        const n_f: f64 = frame_count as f64;
        let d: usize = 0;
        while (d < feat_dim) : (d += 1) {
            const x: f64 = frame[d] as f64;
            const delta: f64 = x - mean_buf[d];
            mean_buf[d] += delta / n_f;
            const delta2: f64 = x - mean_buf[d];
            m2_buf[d] += delta * delta2;

            const centered: f64 = x - mean_buf[d];
            if (normalize_variance && frame_count > 1) {
                const inv_std: f64 = 1.0 / std.math.sqrt((m2_buf[d] / n_f) + 1.0e-10);
                frame[d] = (centered * inv_std) as f32;
            } else {
                frame[d] = centered as f32;
            }
        }
    }
}

// ============================================================================
// 5. Streaming Dual-Threshold ASR Endpointing VAD
// ============================================================================

/// Hysteresis state of the streaming ASR endpointing detector.
pub enum EndpointState {
    silence,
    speech_onset,
    speech_active,
    speech_hangover,
}

/// Detected speech utterance segment span with frame indices, timestamps (ms), and peak energy (dB).
pub struct SpeechSegmentSpan {
    start_frame: usize = 0;
    end_frame: usize = 0;
    start_ms: u64 = 0;
    end_ms: u64 = 0;
    peak_energy_db: f32 = -120.0;
}

/// Configuration for the dual-threshold adaptive ASR Endpointing VAD.
pub struct EndpointVadConfig {
    energy_thresh_db: f32 = -38.0;
    noise_margin_db: f32 = 8.0;
    max_flatness: f32 = 0.65;
    onset_frames: usize = 3;
    hangover_frames: usize = 8;
    frame_ms: u64 = 10;
    noise_alpha: f32 = 0.05;

    static fn defaults(): EndpointVadConfig {
        return EndpointVadConfig{};
    }
}

fn computeFrameEnergyDb(frame_pcm: []const f32): f32 {
    if (frame_pcm.len == 0) return -120.0;
    const sum_sq: f32 = @dot(frame_pcm, frame_pcm);
    const mean_sq: f64 = (sum_sq as f64) / (frame_pcm.len as f64);
    const clamped: f64 = if (mean_sq > 1.0e-12) mean_sq else 1.0e-12;
    return (10.0 * std.math.log10(clamped)) as f32;
}

fn computeFrameZcr(frame_pcm: []const f32): f32 {
    if (frame_pcm.len <= 1) return 0.0;
    let crossings: usize = 0;
    let i: usize = 1;
    while (i < frame_pcm.len) : (i += 1) {
        const a: bool = frame_pcm[i - 1] >= 0.0;
        const b: bool = frame_pcm[i] >= 0.0;
        if (a != b) crossings += 1;
    }
    return (crossings as f32) / ((frame_pcm.len - 1) as f32);
}

fn computeSpectralFlatness(power_spec: []const f32): f32 {
    if (power_spec.len == 0) return 0.0;
    let log_sum: f64 = 0.0;
    let lin_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < power_spec.len) : (i += 1) {
        const p: f64 = if (power_spec[i] > 1.0e-12) (power_spec[i] as f64) else 1.0e-12;
        log_sum += @log(p);
        lin_sum += p;
    }
    const n_f: f64 = power_spec.len as f64;
    const geo_mean: f64 = std.math.exp(log_sum / n_f);
    const arith_mean: f64 = lin_sum / n_f;
    if (arith_mean <= 1.0e-12) return 1.0;
    const flat: f64 = geo_mean / arith_mean;
    return (if (flat > 1.0) 1.0 else flat) as f32;
}

/// Real-time streaming dual-threshold ASR Endpointing VAD combining adaptive noise floor tracking,
/// short-time RMS energy (`@dot`), zero-crossing rate, Wiener spectral flatness, and onset/hangover hysteresis.
pub struct StreamingEndpointVad {
    cfg: EndpointVadConfig = EndpointVadConfig{},
    state: EndpointState = EndpointState.silence;
    noise_floor_db: f32 = -60.0;
    noise_initialized: bool = false;
    onset_count: usize = 0;
    hangover_count: usize = 0;
    last_energy_db: f32 = -120.0;
    last_zcr: f32 = 0.0;
    last_flatness: f32 = 1.0;

    static fn init(cfg: EndpointVadConfig): StreamingEndpointVad {
        return StreamingEndpointVad{ cfg: cfg };
    }

    fn reset(*_) {
        state = EndpointState.silence;
        noise_floor_db = -60.0;
        noise_initialized = false;
        onset_count = 0;
        hangover_count = 0;
        last_energy_db = -120.0;
        last_zcr = 0.0;
        last_flatness = 1.0;
    }

    /// Feeds one audio frame and its optional power spectrum, returning the updated `EndpointState`.
    fn feedFrame(*_, frame_pcm: []const f32, power_spec: []const f32): EndpointState {
        const energy_db: f32 = computeFrameEnergyDb(frame_pcm);
        const zcr: f32 = computeFrameZcr(frame_pcm);
        const flatness: f32 = computeSpectralFlatness(power_spec);

        last_energy_db = energy_db;
        last_zcr = zcr;
        last_flatness = flatness;

        if (!noise_initialized) {
            noise_floor_db = if (energy_db < cfg.energy_thresh_db) energy_db else (cfg.energy_thresh_db - cfg.noise_margin_db);
            noise_initialized = true;
        }

        const adaptive_thresh: f32 = noise_floor_db + cfg.noise_margin_db;
        const eff_thresh: f32 = if (cfg.energy_thresh_db > adaptive_thresh) cfg.energy_thresh_db else adaptive_thresh;
        const tonal_ok: bool = (power_spec.len == 0) || (flatness <= cfg.max_flatness);
        const is_speech: bool = (energy_db > eff_thresh) && tonal_ok;

        const min_onset: usize = if (cfg.onset_frames == 0) 1 else cfg.onset_frames;
        const min_hang: usize = if (cfg.hangover_frames == 0) 1 else cfg.hangover_frames;

        if (state == EndpointState.silence) {
            if (is_speech) {
                onset_count = 1;
                if (onset_count >= min_onset) {
                    state = EndpointState.speech_active;
                } else {
                    state = EndpointState.speech_onset;
                }
            } else {
                // Adapt background noise floor slowly during silence
                noise_floor_db = noise_floor_db * (1.0 - cfg.noise_alpha) + energy_db * cfg.noise_alpha;
            }
        } else if (state == EndpointState.speech_onset) {
            if (is_speech) {
                onset_count += 1;
                if (onset_count >= min_onset) {
                    state = EndpointState.speech_active;
                    hangover_count = 0;
                }
            } else {
                onset_count = 0;
                state = EndpointState.silence;
                noise_floor_db = noise_floor_db * (1.0 - cfg.noise_alpha) + energy_db * cfg.noise_alpha;
            }
        } else if (state == EndpointState.speech_active) {
            if (!is_speech) {
                hangover_count = 1;
                if (hangover_count >= min_hang) {
                    state = EndpointState.silence;
                    onset_count = 0;
                    hangover_count = 0;
                } else {
                    state = EndpointState.speech_hangover;
                }
            }
        } else {
            // EndpointState.speech_hangover
            if (is_speech) {
                state = EndpointState.speech_active;
                hangover_count = 0;
            } else {
                hangover_count += 1;
                if (hangover_count >= min_hang) {
                    state = EndpointState.silence;
                    onset_count = 0;
                    hangover_count = 0;
                }
            }
        }

        return state;
    }
}

/// Scans an arbitrary-length PCM waveform using `StreamingEndpointVad` + FFT spectral flatness
/// and writes all detected speech utterance spans into `out_segments` (zero dynamic heap allocation).
pub fn detectSpeechSegments(
    pcm: []const f32,
    cfg_fe: AsrFrontendConfig,
    cfg_vad: EndpointVadConfig,
    fft_re: []f64,
    fft_im: []f64,
    power_buf: []f32,
    out_segments: []SpeechSegmentSpan,
): usize ! AsrFeatureError {
    validateConfig(cfg_fe)?;
    if (pcm.len < cfg_fe.n_fft) return AsrFeatureError.InputTooShort;
    const n_bins: usize = cfg_fe.nBins();
    if (fft_re.len < cfg_fe.n_fft || fft_im.len < cfg_fe.n_fft || power_buf.len < n_bins) {
        return AsrFeatureError.BufferTooSmall;
    }

    const num_frames: usize = asrFrameCount(pcm.len, cfg_fe.n_fft, cfg_fe.hop_length);
    const win_len: usize = cfg_fe.effectiveWinLen();
    const min_onset: usize = if (cfg_vad.onset_frames == 0) 1 else cfg_vad.onset_frames;
    const frame_ms: u64 = if (cfg_vad.frame_ms > 0)
        cfg_vad.frame_ms
    else
        ((@as(u64, cfg_fe.hop_length) * 1000) / @as(u64, cfg_fe.sample_rate_hz));

    let vad = StreamingEndpointVad.init(cfg_vad);
    let seg_count: usize = 0;
    let in_segment: bool = false;
    let seg_start: usize = 0;
    let seg_peak_db: f32 = -120.0;

    let f: usize = 0;
    while (f < num_frames) : (f += 1) {
        const frame_off: usize = f * cfg_fe.hop_length;
        computeFramePowerSpectrum(pcm, frame_off, cfg_fe, fft_re, fft_im, power_buf)?;
        const st = vad.feedFrame(pcm[frame_off .. frame_off + win_len], power_buf[0..n_bins]);

        if (!in_segment && st == EndpointState.speech_active) {
            in_segment = true;
            seg_start = if (f + 1 >= min_onset) (f + 1 - min_onset) else 0;
            seg_peak_db = vad.last_energy_db;
        } else if (in_segment) {
            if (vad.last_energy_db > seg_peak_db) {
                seg_peak_db = vad.last_energy_db;
            }
            if (st == EndpointState.silence) {
                if (seg_count < out_segments.len) {
                    out_segments[seg_count] = SpeechSegmentSpan{
                        start_frame: seg_start,
                        end_frame: f,
                        start_ms: (@as(u64, seg_start)) * frame_ms,
                        end_ms: (@as(u64, f)) * frame_ms,
                        peak_energy_db: seg_peak_db,
                    };
                    seg_count += 1;
                }
                in_segment = false;
            }
        }
    }

    if (in_segment && seg_count < out_segments.len) {
        out_segments[seg_count] = SpeechSegmentSpan{
            start_frame: seg_start,
            end_frame: num_frames,
            start_ms: (@as(u64, seg_start)) * frame_ms,
            end_ms: (@as(u64, num_frames)) * frame_ms,
            peak_energy_db: seg_peak_db,
        };
        seg_count += 1;
    }

    return seg_count;
}
