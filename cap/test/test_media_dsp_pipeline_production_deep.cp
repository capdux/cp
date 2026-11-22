// Origin: Pure-CP Stdlib Test Suite (lib/media/dsp_pipeline.cp) | Created: 2026-11-22
// Production Deep Verification Suite for SIMD Audio DSP Pipeline & Adaptive Energy VAD:
//   1. analyzeAudioFrameF32 exact SIMD @dot RMS, dBFS, peak & ZCR on DC, alternating, silent & empty frames
//   2. AdaptiveEnergyVad onset hysteresis, sustained speech, hangover smoothing, SpeechEnd & reset
//   3. AdaptiveEnergyVad ZCR hiss gating & adaptive EMA noise-floor threshold elevation
//   4. applyPreEmphasisF32, applyHannWindowF32 symmetry & normalizePeakF32 anti-clipping
//   5. resampleLinearF32 48kHz->16kHz downsampling & 16kHz->48kHz upsampling accuracy
//   6. Empty, undersized buffer, zero-rate & invalid/NaN parameter guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AudioFrameStats,
    analyzeAudioFrameF32,
    VadConfig,
    VadState,
    VadDecision,
    AdaptiveEnergyVad,
    applyPreEmphasisF32,
    applyHannWindowF32,
    normalizePeakF32,
    resampleLinearF32,
} from "cap:media";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: SIMD Frame Energy, dBFS, Peak & ZCR Analysis
// ============================================================================

fn testAnalyzeAudioFrameExact()! {
    // 1a. Constant DC frame of 0.5: RMS=0.5, peak=0.5, ZCR=0.0, dBFS ≈ -6.0206
    const dc: [16]f32 = [0.5; 16];
    const st_dc: AudioFrameStats = cap.media.analyzeAudioFrameF32(dc[0..]);
    expect_eq(st_dc.sample_count, 16);
    expect_eq(approxEqF32(st_dc.rms, 0.5, 0.0001), true);
    expect_eq(approxEqF32(st_dc.peak_abs, 0.5, 0.0001), true);
    expect_eq(approxEqF32(st_dc.zcr, 0.0, 0.0001), true);
    expect_eq(approxEqF32(st_dc.dbfs, -6.0206, 0.01), true);

    // 1b. Alternating +0.5 / -0.5 square wave: RMS=0.5, peak=0.5, ZCR=1.0
    const alt: [8]f32 = [0.5, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5, -0.5];
    const st_alt = analyzeAudioFrameF32(alt[0..]);
    expect_eq(approxEqF32(st_alt.rms, 0.5, 0.0001), true);
    expect_eq(approxEqF32(st_alt.zcr, 1.0, 0.0001), true);

    // 1c. Silence & empty frame
    const silent: [8]f32 = [0.0; 8];
    const st_sil = analyzeAudioFrameF32(silent[0..]);
    expect_eq(approxEqF32(st_sil.rms, 0.0, 0.0001), true);
    expect_eq(approxEqF32(st_sil.dbfs, -120.0, 0.001), true);

    const empty: []const f32 = silent[0..0];
    const st_empty = analyzeAudioFrameF32(empty);
    expect_eq(st_empty.sample_count, 0);
    expect_eq(approxEqF32(st_empty.dbfs, -120.0, 0.001), true);
}

// ============================================================================
// Test 2: AdaptiveEnergyVad Onset, Sustained Speech, Hangover & SpeechEnd
// ============================================================================

fn testAdaptiveVadStateMachine()! {
    let vad = AdaptiveEnergyVad.init(VadConfig{
        energy_threshold: 0.05,
        noise_multiplier: 2.0,
        noise_alpha: 0.1,
        min_zcr: 0.0,
        max_zcr: 0.5,
        min_speech_frames: 2,
        hangover_frames: 2,
    }) ?? return error.InitFailed;

    const quiet: [16]f32 = [0.005; 16];
    const speech: [16]f32 = [0.25; 16];

    // Background silence calibrates noise_floor
    const d0: VadDecision = vad.processFrame(quiet[0..]);
    expect_eq(d0.state == VadState.Silence, true);
    expect_eq(d0.is_speech, false);

    // Single brief spike (1 frame < min_speech_frames=2) must NOT trigger SpeechStart
    const d_spike = vad.processFrame(speech[0..]);
    expect_eq(d_spike.state == VadState.Silence, true);
    const d_back = vad.processFrame(quiet[0..]);
    expect_eq(d_back.state == VadState.Silence, true);

    // Two consecutive speech frames -> frame 1 Silence (confirming), frame 2 SpeechStart
    expect_eq(vad.processFrame(speech[0..]).state == VadState.Silence, true);
    const d_start = vad.processFrame(speech[0..]);
    expect_eq(d_start.state == VadState.SpeechStart, true);
    expect_eq(d_start.is_speech, true);
    expect_eq(vad.isInSpeech(), true);

    // Sustained speech frame -> Speech
    const d_mid = vad.processFrame(speech[0..]);
    expect_eq(d_mid.state == VadState.Speech, true);
    expect_eq(d_mid.is_speech, true);

    // Hangover holds speech active for 2 quiet frames, then 3rd quiet frame emits SpeechEnd
    expect_eq(vad.processFrame(quiet[0..]).state == VadState.Speech, true);
    expect_eq(vad.processFrame(quiet[0..]).state == VadState.Speech, true);
    const d_end = vad.processFrame(quiet[0..]);
    expect_eq(d_end.state == VadState.SpeechEnd, true);
    expect_eq(d_end.is_speech, false);
    expect_eq(vad.isInSpeech(), false);

    // Next quiet frame returns to Silence
    expect_eq(vad.processFrame(quiet[0..]).state == VadState.Silence, true);
    expect_eq(vad.framesProcessed(), 10);
    expect_eq(vad.speechFrames(), 4);

    // Reset clears all state
    vad.reset();
    expect_eq(vad.framesProcessed(), 0);
    expect_eq(vad.speechFrames(), 0);
    expect_eq(vad.isInSpeech(), false);
}

// ============================================================================
// Test 3: AdaptiveEnergyVad ZCR Hiss Gating & Adaptive Noise Floor
// ============================================================================

fn testVadZcrGatingAndNoiseFloor()! {
    let vad = AdaptiveEnergyVad.init(VadConfig{
        energy_threshold: 0.02,
        noise_multiplier: 3.0,
        noise_alpha: 0.5,
        min_zcr: 0.0,
        max_zcr: 0.4,
        min_speech_frames: 1,
        hangover_frames: 0,
    }) ?? return error.InitFailed;

    // High-energy alternating static (RMS=0.4, ZCR=1.0 > max_zcr=0.4) -> rejected as Silence
    const hiss: [8]f32 = [0.4, -0.4, 0.4, -0.4, 0.4, -0.4, 0.4, -0.4];
    const d_hiss = vad.processFrame(hiss[0..]);
    expect_eq(d_hiss.state == VadState.Silence, true);
    expect_eq(d_hiss.is_speech, false);

    // Adaptive noise floor: initialize at ambient RMS=0.05 -> active_threshold = 0.05 * 3.0 = 0.15
    vad.reset();
    const ambient: [8]f32 = [0.05; 8];
    // Feed high-ZCR ambient or low-RMS ambient first; wait: 0.05 > 0.02, so use min_speech_frames=2 or start with 0.015
    let vad2 = AdaptiveEnergyVad.init(VadConfig{
        energy_threshold: 0.02,
        noise_multiplier: 3.0,
        noise_alpha: 0.5,
        min_zcr: 0.0,
        max_zcr: 0.5,
        min_speech_frames: 1,
        hangover_frames: 0,
    }) ?? return error.InitFailed;
    const low_noise: [8]f32 = [0.015; 8];
    * = vad2.processFrame(low_noise[0..]); // noise_floor = 0.015 -> dynamic thresh = 0.045
    const weak_sig: [8]f32 = [0.03; 8];    // > 0.02 base threshold, but < 0.045 dynamic threshold!
    const d_weak = vad2.processFrame(weak_sig[0..]);
    expect_eq(d_weak.state == VadState.Silence, true);
    expect_eq(d_weak.active_threshold > 0.04, true);
    * = ambient;
}

// ============================================================================
// Test 4: Pre-Emphasis, Hann Window & Peak Normalization
// ============================================================================

fn testPreEmphasisHannAndNormalize()! {
    // 4a. Pre-emphasis: y[0] = x[0], y[i] = x[i] - 0.97 * x[i-1]
    const input: [4]f32 = [1.0, 1.0, 0.5, -0.5];
    let pre_out: [4]f32 = [0.0; 4];
    let pre_s: []f32 = pre_out[0..];
    const written = applyPreEmphasisF32(input[0..], pre_s, 0.97) ?? return error.PreEmphFailed;
    expect_eq(written, 4);
    expect_eq(approxEqF32(pre_out[0], 1.0, 0.0001), true);
    expect_eq(approxEqF32(pre_out[1], 0.03, 0.0001), true);
    expect_eq(approxEqF32(pre_out[2], -0.47, 0.0001), true);
    expect_eq(approxEqF32(pre_out[3], -0.985, 0.0001), true);

    // 4b. Hann window of length 5: [0.0, 0.5, 1.0, 0.5, 0.0]
    let win: [5]f32 = [1.0, 1.0, 1.0, 1.0, 1.0];
    let win_s: []f32 = win[0..];
    expect_eq(applyHannWindowF32(win_s), 5);
    expect_eq(approxEqF32(win[0], 0.0, 0.0001), true);
    expect_eq(approxEqF32(win[1], 0.5, 0.0001), true);
    expect_eq(approxEqF32(win[2], 1.0, 0.0001), true);
    expect_eq(approxEqF32(win[3], 0.5, 0.0001), true);
    expect_eq(approxEqF32(win[4], 0.0, 0.0001), true);

    // 4c. Peak normalization to 0.90 (anti-clipping) & silent frame no-op
    let sig: [4]f32 = [0.25, -0.5, 0.1, 0.0];
    let sig_s: []f32 = sig[0..];
    const gain = normalizePeakF32(sig_s, 0.9) ?? return error.NormFailed;
    expect_eq(approxEqF32(gain, 1.8, 0.0001), true);
    expect_eq(approxEqF32(sig[1], -0.9, 0.0001), true);

    let sil: [4]f32 = [0.0; 4];
    let sil_s: []f32 = sil[0..];
    const sil_gain = normalizePeakF32(sil_s, 0.9) ?? return error.NormFailed;
    expect_eq(approxEqF32(sil_gain, 1.0, 0.0001), true);
}

// ============================================================================
// Test 5: Linear Resampler (48kHz -> 16kHz & 16kHz -> 48kHz)
// ============================================================================

fn testResampleLinear48kTo16k()! {
    // 48kHz linear ramp of 48 samples -> 16kHz produces 16 samples with exact 3-sample stride
    let in48: [48]f32 = [0.0; 48];
    let i: usize = 0;
    while (i < 48) : (i += 1) {
        in48[i] = (i as f32) / 48.0;
    }
    let out16: [16]f32 = [0.0; 16];
    let out16_s: []f32 = out16[0..];
    const n16 = resampleLinearF32(in48[0..], 48000, out16_s, 16000) ?? return error.ResampleFailed;
    expect_eq(n16, 16);
    expect_eq(approxEqF32(out16[0], 0.0, 0.0001), true);
    expect_eq(approxEqF32(out16[1], 3.0 / 48.0, 0.0001), true);
    expect_eq(approxEqF32(out16[15], 45.0 / 48.0, 0.0001), true);

    // Upsampling 16kHz (4 samples) -> 48kHz (12 samples) interpolates smoothly
    const in16: [4]f32 = [0.0, 0.3, 0.6, 0.9];
    let out48: [12]f32 = [0.0; 12];
    let out48_s: []f32 = out48[0..];
    const n48 = resampleLinearF32(in16[0..], 16000, out48_s, 48000) ?? return error.UpsampleFailed;
    expect_eq(n48, 12);
    expect_eq(approxEqF32(out48[0], 0.0, 0.0001), true);
    expect_eq(approxEqF32(out48[1], 0.1, 0.0005), true);
    expect_eq(approxEqF32(out48[2], 0.2, 0.0005), true);
    expect_eq(approxEqF32(out48[3], 0.3, 0.0005), true);
}

// ============================================================================
// Test 6: Empty, Undersized Buffer, Zero-Rate & Invalid/NaN Parameter Guards
// ============================================================================

fn testDspPipelineBoundaryGuards()! {
    // Invalid VAD configs -> null
    expect_eq(AdaptiveEnergyVad.init(VadConfig{ energy_threshold: -0.1 }) == null, true);
    expect_eq(AdaptiveEnergyVad.init(VadConfig{ noise_multiplier: 0.5 }) == null, true);
    expect_eq(AdaptiveEnergyVad.init(VadConfig{ noise_alpha: 1.5 }) == null, true);
    expect_eq(AdaptiveEnergyVad.init(VadConfig{ min_zcr: 0.7, max_zcr: 0.3 }) == null, true);
    expect_eq(AdaptiveEnergyVad.init(VadConfig{ min_speech_frames: 0 }) == null, true);
    expect_eq(AdaptiveEnergyVad.init(VadConfig{ energy_threshold: std.math.nan(f32) }) == null, true);

    const src: [6]f32 = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6];
    let small: [2]f32 = [0.0; 2];
    let small_s: []f32 = small[0..];
    let ok_buf: [6]f32 = [0.0; 6];
    let ok_s: []f32 = ok_buf[0..];

    // Pre-emphasis guards
    expect_eq(applyPreEmphasisF32(src[0..], small_s, 0.97) == null, true);
    expect_eq(applyPreEmphasisF32(src[0..], ok_s, -0.1) == null, true);
    expect_eq(applyPreEmphasisF32(src[0..], ok_s, 1.1) == null, true);

    // Peak normalization guards
    expect_eq(normalizePeakF32(ok_s, 0.0) == null, true);
    expect_eq(normalizePeakF32(ok_s, 1.2) == null, true);
    expect_eq(normalizePeakF32(ok_s, std.math.nan(f32)) == null, true);

    // Resampler guards
    expect_eq(resampleLinearF32(src[0..], 0, ok_s, 16000) == null, true);
    expect_eq(resampleLinearF32(src[0..], 48000, ok_s, 0) == null, true);
    // 6 samples at 16kHz -> 48kHz requires 18 samples; small_s (len 2) must return null
    expect_eq(resampleLinearF32(src[0..], 16000, small_s, 48000) == null, true);
    // Empty input returns 0 cleanly
    const empty_in: []const f32 = src[0..0];
    expect_eq(resampleLinearF32(empty_in, 48000, small_s, 16000) ?? 99, 0);
}

test("media dsp_pipeline: SIMD analyzeAudioFrameF32 exact RMS, dBFS, peak & ZCR", testAnalyzeAudioFrameExact);
test("media dsp_pipeline: AdaptiveEnergyVad onset, speech, hangover & SpeechEnd", testAdaptiveVadStateMachine);
test("media dsp_pipeline: AdaptiveEnergyVad ZCR hiss gating & adaptive noise floor", testVadZcrGatingAndNoiseFloor);
test("media dsp_pipeline: pre-emphasis, Hann window symmetry & peak normalization", testPreEmphasisHannAndNormalize);
test("media dsp_pipeline: linear resampler 48kHz->16kHz & 16kHz->48kHz", testResampleLinear48kTo16k);
test("media dsp_pipeline: empty, undersized buffer, zero-rate & NaN guards", testDspPipelineBoundaryGuards);
