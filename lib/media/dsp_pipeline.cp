// Origin: Pure-CP Stdlib (SIMD Audio Preprocessing & Adaptive Energy VAD Pipeline) | Created: 2026-11-22
// Pure CP zero-heap-allocation real-time audio DSP & Voice Activity Detection primitives:
//   - 8-lane SIMD `@dot` frame RMS energy, dBFS, peak amplitude & zero-crossing rate (`analyzeAudioFrameF32`)
//   - Adaptive noise-floor + onset/hangover hysteresis VAD state machine (`AdaptiveEnergyVad`)
//   - FMA-accelerated pre-emphasis (`applyPreEmphasisF32`), Hann window (`applyHannWindowF32`),
//     anti-clipping peak normalization (`normalizePeakF32`), and arbitrary-rate linear resampler (`resampleLinearF32`)

const std = @import("std");

const TWO_PI_F64: f64 = 6.28318530717958647692;
const DBFS_FLOOR: f32 = -120.0;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

// ============================================================================
// 1. SIMD Frame Energy, dBFS, Peak & Zero-Crossing Rate Analysis
// ============================================================================

/// Acoustic summary of a single PCM `f32` audio frame.
pub struct AudioFrameStats {
    sample_count: usize = 0;
    rms: f32 = 0.0;
    dbfs: f32 = -120.0;
    peak_abs: f32 = 0.0;
    zcr: f32 = 0.0;
}

/// Computes RMS energy (via 8-lane SIMD `@dot`), dBFS, peak absolute amplitude,
/// and zero-crossing rate (ZCR) for a PCM `f32` frame with zero heap allocation.
pub fn analyzeAudioFrameF32(samples: []const f32): AudioFrameStats {
    if (samples.len == 0) {
        return AudioFrameStats{};
    }

    const sum_sq: f32 = @dot(samples, samples);
    const mean_sq: f32 = if (sum_sq > 0.0) sum_sq / (samples.len as f32) else 0.0;
    const rms: f32 = std.math.sqrt(mean_sq);

    let dbfs: f32 = DBFS_FLOOR;
    if (rms > 0.000001) {
        const raw_db: f32 = (20.0 * std.math.log10(rms as f64)) as f32;
        dbfs = if (raw_db < DBFS_FLOOR) DBFS_FLOOR else raw_db;
    }

    let peak: f32 = absF32(samples[0]);
    let crossings: usize = 0;
    let i: usize = 1;
    while (i < samples.len) : (i += 1) {
        const a = absF32(samples[i]);
        if (a > peak) peak = a;
        const prev = samples[i - 1];
        const cur = samples[i];
        if ((prev >= 0.0 && cur < 0.0) || (prev < 0.0 && cur >= 0.0)) {
            crossings += 1;
        }
    }

    const zcr: f32 = if (samples.len > 1)
        (crossings as f32) / ((samples.len - 1) as f32)
    else
        0.0;

    return AudioFrameStats{
        sample_count: samples.len,
        rms: rms,
        dbfs: dbfs,
        peak_abs: peak,
        zcr: zcr,
    };
}

// ============================================================================
// 2. Adaptive Noise-Floor & Hysteresis Voice Activity Detector
// ============================================================================

/// Configuration for `AdaptiveEnergyVad`.
pub struct VadConfig {
    energy_threshold: f32 = 0.02;
    noise_multiplier: f32 = 2.0;
    noise_alpha: f32 = 0.05;
    min_zcr: f32 = 0.0;
    max_zcr: f32 = 0.60;
    min_speech_frames: u32 = 2;
    hangover_frames: u32 = 4;
}

/// Frame-level speech state emitted by `AdaptiveEnergyVad.processFrame`.
pub enum VadState {
    Silence,
    SpeechStart,
    Speech,
    SpeechEnd,
}

/// Complete per-frame VAD decision including telemetry and active threshold.
pub struct VadDecision {
    state: VadState = VadState.Silence;
    is_speech: bool = false;
    rms: f32 = 0.0;
    dbfs: f32 = -120.0;
    zcr: f32 = 0.0;
    noise_floor: f32 = 0.0;
    active_threshold: f32 = 0.0;
}

/// Zero-heap-allocation streaming Voice Activity Detector combining SIMD energy metering,
/// ZCR gating, adaptive EMA noise-floor tracking, and onset/hangover hysteresis.
pub struct AdaptiveEnergyVad {
    config: VadConfig;
    in_speech: bool = false;
    onset_streak: u32 = 0;
    hangover_streak: u32 = 0;
    noise_floor: f32 = 0.0;
    noise_initialized: bool = false;
    frames_processed: u64 = 0;
    speech_frames: u64 = 0;

    /// Initializes an `AdaptiveEnergyVad`. Returns `null` if `config` contains invalid or NaN parameters.
    static fn init(config: VadConfig): AdaptiveEnergyVad? {
        if (!isFiniteF32(config.energy_threshold) || config.energy_threshold < 0.0) return null;
        if (!isFiniteF32(config.noise_multiplier) || config.noise_multiplier < 1.0) return null;
        if (!isFiniteF32(config.noise_alpha) || config.noise_alpha < 0.0 || config.noise_alpha > 1.0) return null;
        if (!isFiniteF32(config.min_zcr) || !isFiniteF32(config.max_zcr)) return null;
        if (config.min_zcr < 0.0 || config.max_zcr > 1.0 || config.min_zcr > config.max_zcr) return null;
        if (config.min_speech_frames == 0) return null;

        return AdaptiveEnergyVad{
            config: config,
            in_speech: false,
            onset_streak: 0,
            hangover_streak: 0,
            noise_floor: 0.0,
            noise_initialized: false,
            frames_processed: 0,
            speech_frames: 0,
        };
    }

    /// Processes one audio frame and advances the VAD hysteresis state machine.
    fn processFrame(*_, samples: []const f32): VadDecision {
        const stats = analyzeAudioFrameF32(samples);
        _.frames_processed += 1;

        const dynamic_thresh: f32 = if (_.noise_initialized)
            _.noise_floor * _.config.noise_multiplier
        else
            0.0;
        const active_thresh: f32 = if (dynamic_thresh > _.config.energy_threshold)
            dynamic_thresh
        else
            _.config.energy_threshold;

        const frame_active = stats.sample_count > 0 &&
            stats.rms >= active_thresh &&
            stats.zcr >= _.config.min_zcr &&
            stats.zcr <= _.config.max_zcr;

        let out_state: VadState = VadState.Silence;
        let is_speech = false;

        if (!_.in_speech) {
            if (frame_active) {
                _.onset_streak += 1;
                if (_.onset_streak >= _.config.min_speech_frames) {
                    _.in_speech = true;
                    _.hangover_streak = 0;
                    _.speech_frames += 1;
                    out_state = VadState.SpeechStart;
                    is_speech = true;
                } else {
                    out_state = VadState.Silence;
                    is_speech = false;
                }
            } else {
                _.onset_streak = 0;
                if (stats.sample_count > 0) {
                    if (!_.noise_initialized) {
                        _.noise_floor = stats.rms;
                        _.noise_initialized = true;
                    } else {
                        _.noise_floor = @fma(_.config.noise_alpha, stats.rms - _.noise_floor, _.noise_floor);
                    }
                }
                out_state = VadState.Silence;
                is_speech = false;
            }
        } else {
            if (frame_active) {
                _.hangover_streak = 0;
                _.speech_frames += 1;
                out_state = VadState.Speech;
                is_speech = true;
            } else {
                _.hangover_streak += 1;
                if (_.hangover_streak > _.config.hangover_frames) {
                    _.in_speech = false;
                    _.onset_streak = 0;
                    _.hangover_streak = 0;
                    out_state = VadState.SpeechEnd;
                    is_speech = false;
                } else {
                    _.speech_frames += 1;
                    out_state = VadState.Speech;
                    is_speech = true;
                }
            }
        }

        return VadDecision{
            state: out_state,
            is_speech: is_speech,
            rms: stats.rms,
            dbfs: stats.dbfs,
            zcr: stats.zcr,
            noise_floor: _.noise_floor,
            active_threshold: active_thresh,
        };
    }

    /// Returns `true` when the VAD is currently inside an active speech segment.
    fn isInSpeech(): bool {
        return _.in_speech;
    }

    /// Returns the current smoothed background noise-floor RMS estimate.
    fn noiseFloor(): f32 {
        return _.noise_floor;
    }

    /// Returns total number of frames processed.
    fn framesProcessed(): u64 {
        return _.frames_processed;
    }

    /// Returns total number of frames classified as speech (including hangover).
    fn speechFrames(): u64 {
        return _.speech_frames;
    }

    /// Resets detector state and noise-floor tracker while keeping `config`.
    fn reset(*_) {
        _.in_speech = false;
        _.onset_streak = 0;
        _.hangover_streak = 0;
        _.noise_floor = 0.0;
        _.noise_initialized = false;
        _.frames_processed = 0;
        _.speech_frames = 0;
    }
}

// ============================================================================
// 3. Zero-Heap Audio Frontend DSP Operators
// ============================================================================

/// Applies first-order pre-emphasis filter `output[i] = input[i] - alpha * input[i - 1]`
/// (with `output[0] = input[0]`) using hardware `@fma`.
/// Returns number of samples written, or `null` if `alpha` is outside `[0.0, 1.0]` or `output.len < input.len`.
pub fn applyPreEmphasisF32(input: []const f32, output: *[]f32, alpha: f32): usize? {
    if (!isFiniteF32(alpha) || alpha < 0.0 || alpha > 1.0) return null;
    if (output.len < input.len) return null;
    if (input.len == 0) return 0;

    const neg_alpha: f32 = 0.0 - alpha;
    let prev: f32 = input[0];
    output[0] = prev;
    let i: usize = 1;
    while (i < input.len) : (i += 1) {
        const cur: f32 = input[i];
        output[i] = @fma(neg_alpha, prev, cur);
        prev = cur;
    }
    return input.len;
}

/// Applies a symmetric Hann window `0.5 * (1 - cos(2*pi*i/(N-1)))` in-place to `samples`.
/// Returns `samples.len`.
pub fn applyHannWindowF32(samples: *[]f32): usize {
    const n = samples.len;
    if (n == 0) return 0;
    if (n == 1) return 1;

    const denom: f64 = (n - 1) as f64;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const phase: f64 = (TWO_PI_F64 * (i as f64)) / denom;
        const w: f32 = (0.5 * (1.0 - std.math.cos(phase))) as f32;
        samples[i] = samples[i] * w;
    }
    return n;
}

/// Peak-normalizes `samples` in-place so the maximum absolute amplitude equals `target_peak`
/// (in `(0.0, 1.0]`). Silent frames (`peak <= 1e-7`) are left untouched with gain `1.0`.
/// Returns the applied linear gain factor, or `null` if `target_peak` is invalid.
pub fn normalizePeakF32(samples: *[]f32, target_peak: f32): f32? {
    if (!isFiniteF32(target_peak) || target_peak <= 0.0 || target_peak > 1.0) return null;
    if (samples.len == 0) return 1.0;

    let peak: f32 = 0.0;
    let i: usize = 0;
    while (i < samples.len) : (i += 1) {
        const a = absF32(samples[i]);
        if (a > peak) peak = a;
    }
    if (peak <= 0.0000001) return 1.0;

    const gain: f32 = target_peak / peak;
    let j: usize = 0;
    while (j < samples.len) : (j += 1) {
        samples[j] = samples[j] * gain;
    }
    return gain;
}

/// Arbitrary-rate linear resampler (e.g., 48kHz -> 16kHz for ASR/Whisper frontends) with zero heap allocation.
/// Writes `floor(input.len * out_rate_hz / in_rate_hz)` samples (at least 1 when `input.len > 0`) into `output`.
/// Returns the number of output samples written, or `null` on zero sample rate or insufficient `output` capacity.
pub fn resampleLinearF32(
    input: []const f32,
    in_rate_hz: u32,
    output: *[]f32,
    out_rate_hz: u32,
): usize? {
    if (in_rate_hz == 0 || out_rate_hz == 0) return null;
    if (input.len == 0) return 0;

    const scaled_u64: u64 = ((input.len as u64) * (out_rate_hz as u64)) / (in_rate_hz as u64);
    const out_len: usize = if (scaled_u64 == 0) 1 else (scaled_u64 as usize);
    if (output.len < out_len) return null;

    if (out_len == 1 || input.len == 1) {
        let k: usize = 0;
        while (k < out_len) : (k += 1) {
            output[k] = input[0];
        }
        return out_len;
    }

    const ratio: f64 = (in_rate_hz as f64) / (out_rate_hz as f64);
    let i: usize = 0;
    while (i < out_len) : (i += 1) {
        const src_pos: f64 = (i as f64) * ratio;
        let idx0: usize = src_pos as usize;
        if (idx0 >= input.len - 1) {
            output[i] = input[input.len - 1];
        } else {
            const frac: f32 = (src_pos - (idx0 as f64)) as f32;
            const s0 = input[idx0];
            const s1 = input[idx0 + 1];
            output[i] = @fma(frac, s1 - s0, s0);
        }
    }
    return out_len;
}
