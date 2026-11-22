// Origin: Pure-CP Stdlib (AI Audio/SSM 1D Causal & Depthwise Conv + Mel Filterbank using @dot/@fma) | Created: 2026-11-22
// Pure CP zero-allocation 1D causal dilated convolution, depthwise separable 1D convolution,
// HTK Mel frequency conversions, triangular Mel filterbank builder, and SIMD @dot log-Mel projection.

const std = @import("std");

// ============================================================================
// Internal Validation Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn checkedMul(a: usize, b: usize): usize? {
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a > std.math.maxInt(usize) / b) {
        return null;
    }
    return a * b;
}

// ============================================================================
// 1. Causal Dilated 1D Convolution (WaveNet / Whisper / Mamba)
// ============================================================================

/// Computes 1D causal convolution with implicit left-padding `(kernel_size - 1) * dilation`.
/// - `input`: `[in_ch, seq_len]` channel-first
/// - `weight`: `[out_ch, in_ch, kernel_size]`
/// - `bias`: optional (`len == 0` disables bias; otherwise `len >= out_ch`)
/// - `out`: `[out_ch, seq_len]`
///
/// Guarantees zero future-frame leakage (`src_t <= t`). Uses 8-lane SIMD `@dot` on contiguous
/// interior windows (`dilation == 1`) and hardware `@fma` elsewhere.
pub fn conv1dCausalF32(
    input: []const f32,
    weight: []const f32,
    bias: []const f32,
    in_ch: usize,
    out_ch: usize,
    seq_len: usize,
    kernel_size: usize,
    dilation: usize,
    out: *[]f32,
): bool {
    if (in_ch == 0 || out_ch == 0 || seq_len == 0 || kernel_size == 0 || dilation == 0) {
        return false;
    }
    if (bias.len != 0 && bias.len < out_ch) {
        return false;
    }
    const in_elems: usize = checkedMul(in_ch, seq_len) ?? return false;
    const out_elems: usize = checkedMul(out_ch, seq_len) ?? return false;
    const oc_ic: usize = checkedMul(out_ch, in_ch) ?? return false;
    const w_elems: usize = checkedMul(oc_ic, kernel_size) ?? return false;
    if (input.len < in_elems || weight.len < w_elems || out.len < out_elems) {
        return false;
    }
    const k_minus_1: usize = kernel_size - 1;
    const max_offset: usize = checkedMul(k_minus_1, dilation) ?? return false;
    const has_bias: bool = bias.len >= out_ch;

    let oc: usize = 0;
    while (oc < out_ch) : (oc += 1) {
        const out_ch_base: usize = oc * seq_len;
        const b_val: f32 = if (has_bias) bias[oc] else 0.0;

        let t: usize = 0;
        while (t < seq_len) : (t += 1) {
            let acc: f32 = b_val;
            if (dilation == 1 && t >= k_minus_1) {
                const start_t: usize = t - k_minus_1;
                let ic: usize = 0;
                while (ic < in_ch) : (ic += 1) {
                    const in_base: usize = ic * seq_len + start_t;
                    const w_base: usize = (oc * in_ch + ic) * kernel_size;
                    acc += @dot(input[in_base .. in_base + kernel_size], weight[w_base .. w_base + kernel_size]);
                }
            } else {
                let ic: usize = 0;
                while (ic < in_ch) : (ic += 1) {
                    const in_ch_base: usize = ic * seq_len;
                    const w_base: usize = (oc * in_ch + ic) * kernel_size;
                    let k: usize = 0;
                    while (k < kernel_size) : (k += 1) {
                        const offset: usize = max_offset - k * dilation;
                        if (t >= offset) {
                            acc = @fma(input[in_ch_base + (t - offset)], weight[w_base + k], acc);
                        }
                    }
                }
            }
            out[out_ch_base + t] = acc;
        }
    }
    return true;
}

// ============================================================================
// 2. Depthwise Separable 1D Convolution (ConvNeXt-1D / Conformer / Mamba)
// ============================================================================

/// Computes per-channel depthwise 1D convolution with configurable `stride` and symmetric `padding`.
/// - `input`: `[channels, seq_len]`
/// - `weight`: `[channels, kernel_size]`
/// - `bias`: optional (`len == 0` disables bias; otherwise `len >= channels`)
/// - `out`: `[channels, out_len]` where `out_len = (seq_len + 2 * padding - kernel_size) / stride + 1`.
pub fn depthwiseConv1dF32(
    input: []const f32,
    weight: []const f32,
    bias: []const f32,
    channels: usize,
    seq_len: usize,
    kernel_size: usize,
    stride: usize,
    padding: usize,
    out: *[]f32,
): bool {
    if (channels == 0 || seq_len == 0 || kernel_size == 0 || stride == 0) {
        return false;
    }
    if (bias.len != 0 && bias.len < channels) {
        return false;
    }
    const two_pad: usize = checkedMul(padding, 2) ?? return false;
    if (seq_len > std.math.maxInt(usize) - two_pad) {
        return false;
    }
    const padded_len: usize = seq_len + two_pad;
    if (padded_len < kernel_size) {
        return false;
    }
    const out_len: usize = (padded_len - kernel_size) / stride + 1;

    const in_elems: usize = checkedMul(channels, seq_len) ?? return false;
    const w_elems: usize = checkedMul(channels, kernel_size) ?? return false;
    const out_elems: usize = checkedMul(channels, out_len) ?? return false;
    if (input.len < in_elems || weight.len < w_elems || out.len < out_elems) {
        return false;
    }
    const has_bias: bool = bias.len >= channels;

    let ch: usize = 0;
    while (ch < channels) : (ch += 1) {
        const in_base: usize = ch * seq_len;
        const w_base: usize = ch * kernel_size;
        const out_base: usize = ch * out_len;
        const b_val: f32 = if (has_bias) bias[ch] else 0.0;
        const w_slice = weight[w_base .. w_base + kernel_size];

        let ot: usize = 0;
        while (ot < out_len) : (ot += 1) {
            const start_padded: usize = ot * stride;
            let acc: f32 = b_val;

            if (start_padded >= padding && (start_padded - padding + kernel_size) <= seq_len) {
                const in_start: usize = in_base + (start_padded - padding);
                acc += @dot(input[in_start .. in_start + kernel_size], w_slice);
            } else {
                let k: usize = 0;
                while (k < kernel_size) : (k += 1) {
                    const pos_padded: usize = start_padded + k;
                    if (pos_padded >= padding) {
                        const src_idx: usize = pos_padded - padding;
                        if (src_idx < seq_len) {
                            acc = @fma(input[in_base + src_idx], w_slice[k], acc);
                        }
                    }
                }
            }
            out[out_base + ot] = acc;
        }
    }
    return true;
}

// ============================================================================
// 3. HTK Mel Scale Conversions & Triangular Mel Filterbank
// ============================================================================

/// Converts frequency in Hertz to HTK Mel scale: `2595 * log10(1 + hz / 700)`.
/// Clamps non-positive or NaN inputs safely to `0.0`.
pub fn hzToMel(hz: f64): f64 {
    if (!isFiniteF64(hz) || hz <= 0.0) {
        return 0.0;
    }
    return 2595.0 * std.math.log10(1.0 + hz / 700.0);
}

/// Converts HTK Mel scale back to Hertz: `700 * (10^(mel / 2595) - 1)`.
/// Clamps non-positive or NaN inputs safely to `0.0`.
pub fn melToHz(mel: f64): f64 {
    if (!isFiniteF64(mel) || mel <= 0.0) {
        return 0.0;
    }
    return 700.0 * (std.math.pow(f64, 10.0, mel / 2595.0) - 1.0);
}

/// Builds a zero-allocation triangular Mel filterbank matrix `[n_mels, n_bins]` in `out_fb`,
/// where `n_bins = n_fft / 2 + 1`.
/// Adjacent triangular filters form a partition of unity (weights sum to 1.0 between inner centers).
pub fn buildMelFilterbankF32(
    n_mels: usize,
    n_fft: usize,
    sample_rate: f64,
    f_min: f64,
    f_max: f64,
    out_fb: *[]f32,
): bool {
    if (n_mels == 0 || n_fft < 2 || (n_fft % 2) != 0) {
        return false;
    }
    if (!isFiniteF64(sample_rate) || sample_rate <= 0.0) {
        return false;
    }
    if (!isFiniteF64(f_min) || !isFiniteF64(f_max) || f_min < 0.0 || f_max <= f_min) {
        return false;
    }
    if (f_max > sample_rate * 0.5 + 0.000001) {
        return false;
    }

    const n_bins: usize = (n_fft / 2) + 1;
    const total: usize = checkedMul(n_mels, n_bins) ?? return false;
    if (out_fb.len < total) {
        return false;
    }

    const mel_min: f64 = hzToMel(f_min);
    const mel_max: f64 = hzToMel(f_max);
    const mel_step: f64 = (mel_max - mel_min) / ((n_mels + 1) as f64);
    const bin_hz_scale: f64 = sample_rate / (n_fft as f64);

    let m: usize = 0;
    while (m < n_mels) : (m += 1) {
        const f_left: f64 = melToHz(mel_min + (m as f64) * mel_step);
        const f_center: f64 = melToHz(mel_min + ((m + 1) as f64) * mel_step);
        const f_right: f64 = melToHz(mel_min + ((m + 2) as f64) * mel_step);
        const left_span: f64 = f_center - f_left;
        const right_span: f64 = f_right - f_center;
        const row_base: usize = m * n_bins;

        let b: usize = 0;
        while (b < n_bins) : (b += 1) {
            const hz: f64 = (b as f64) * bin_hz_scale;
            let w: f64 = 0.0;
            if (hz > f_left && hz <= f_center && left_span > 0.0) {
                w = (hz - f_left) / left_span;
            } else if (hz > f_center && hz < f_right && right_span > 0.0) {
                w = (f_right - hz) / right_span;
            }
            out_fb[row_base + b] = w as f32;
        }
    }
    return true;
}

/// Projects a power spectrum `power_spec` (`[n_bins]`) through `filterbank` (`[n_mels, n_bins]`)
/// using 8-lane SIMD `@dot` and outputs clamped natural-log Mel energies into `out_log_mel`.
pub fn applyMelFilterbankF32(
    power_spec: []const f32,
    filterbank: []const f32,
    n_mels: usize,
    n_bins: usize,
    log_floor: f32,
    out_log_mel: *[]f32,
): bool {
    if (n_mels == 0 || n_bins == 0 || !isFiniteF32(log_floor) || log_floor <= 0.0) {
        return false;
    }
    const fb_elems: usize = checkedMul(n_mels, n_bins) ?? return false;
    if (power_spec.len < n_bins || filterbank.len < fb_elems || out_log_mel.len < n_mels) {
        return false;
    }

    const spec_slice = power_spec[0..n_bins];
    let m: usize = 0;
    while (m < n_mels) : (m += 1) {
        const fb_row = filterbank[m * n_bins .. (m + 1) * n_bins];
        const energy: f32 = @dot(spec_slice, fb_row);
        if (std.math.isNan(energy)) {
            return false;
        }
        const clamped: f32 = if (energy < log_floor) log_floor else energy;
        out_log_mel[m] = @log(clamped);
    }
    return true;
}
