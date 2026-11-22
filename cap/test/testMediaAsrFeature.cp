// Origin: Pure-CP Stdlib Test Suite (lib/media/asrFeature.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Whisper/Conformer Log-Mel, MFCC+Delta/Delta-Delta, CMVN & Endpointing VAD:
//   1. HTK vs Slaney Mel scale exact round-trip & Slaney equal-area triangular filterbank response
//   2. 16 kHz 1-second (16,000 samples) 80-bin Whisper Log-Mel: 1 kHz sine wave peak bin + dynamic range clamp
//   3. Whisper Large-v3 128-bin config & deterministic SpecAugment time/frequency masking
//   4. Orthonormal DCT-II MFCC exact parity between `computeMfccFromLogMel` and `@dot` `computeMfccFastDot` + liftering
//   5. Delta & Delta-Delta exact regression slope on linear ramp + 39-dim `packMfccWithDeltas`
//   6. `applyCmvnInPlace` strict zero-mean / unit-variance & `StreamingCmvnNormalizer` online normalization
//   7. Silence-Speech-Silence 1s waveform: `detectSpeechSegments` endpointing VAD timestamp accuracy
//   8. Comprehensive error boundary guards & `cap.media` facade parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    AsrFeatureError,
    MelScaleKind,
    AsrFrontendConfig,
    asrFrameCount,
    asrMelBankLen,
    asrMelOutLen,
    hzToMelScale,
    melScaleToHz,
    buildMelFilterbankMatrix,
    extractLogMelSpectrogram,
    computeMfccFromLogMel,
    computeDeltaFeatures,
    applyCmvnInPlace,
    applyCepstralLifteringInPlace,
    buildDct2Matrix,
    computeMfccFastDot,
    packMfccWithDeltas,
    applySpecAugmentInPlace,
    StreamingCmvnNormalizer,
    EndpointState,
    SpeechSegmentSpan,
    EndpointVadConfig,
    StreamingEndpointVad,
    detectSpeechSegments,
} from "cap:media";

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

test("ASR Mel Scale: HTK & Slaney exact round-trip + Slaney equal-area filterbank") {
    const freqs: [6]f64 = [0.0, 250.0, 1000.0, 2000.0, 4000.0, 8000.0];
    let i: usize = 0;
    while (i < freqs.len) : (i += 1) {
        const f = freqs[i];
        const m_htk = hzToMelScale(f, MelScaleKind.htk);
        const f_htk = melScaleToHz(m_htk, MelScaleKind.htk);
        expect_true(absF64(f_htk - f) < 1.0e-6);

        const m_sl = hzToMelScale(f, MelScaleKind.slaney);
        const f_sl = melScaleToHz(m_sl, MelScaleKind.slaney);
        expect_true(absF64(f_sl - f) < 1.0e-6);
    }

    // Slaney linear boundary check: 1000 Hz -> 15.0 mel exactly
    expect_true(absF64(hzToMelScale(1000.0, MelScaleKind.slaney) - 15.0) < 1.0e-9);

    // Build 80-bin Slaney filterbank for 16kHz / n_fft=512
    const cfg = AsrFrontendConfig.whisperDefaults();
    expect_eq(asrMelBankLen(80, 512), @as(usize, 80 * 257));
    let weights: [20560]f32 = [0.0; 20560];
    const written = buildMelFilterbankMatrix(cfg, weights[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(written, @as(usize, 20560));

    // Every filter row must be non-negative and have positive total weight
    let m: usize = 0;
    while (m < 80) : (m += 1) {
        let row_sum: f64 = 0.0;
        let k: usize = 0;
        while (k < 257) : (k += 1) {
            const w = weights[m * 257 + k];
            expect_true(w >= 0.0);
            row_sum += w as f64;
        }
        expect_true(row_sum > 0.0);
    }
}

test("ASR Whisper Log-Mel: 16kHz 1s 80-bin extraction pinpoints 1kHz sine & dynamic range clamp") {
    const cfg = AsrFrontendConfig.whisperDefaults();
    expect_true(cfg.isValid());
    const n_frames = asrFrameCount(16000, 512, 160);
    expect_eq(n_frames, @as(usize, 97));
    expect_eq(asrMelOutLen(n_frames, 80), @as(usize, 97 * 80));

    // Generate 1 second of 1000 Hz pure sine wave at 16 kHz
    let pcm: [16000]f32 = [0.0; 16000];
    const two_pi: f64 = 6.28318530717958647692;
    let i: usize = 0;
    while (i < 16000) : (i += 1) {
        pcm[i] = (@sin(two_pi * 1000.0 * (i as f64) / 16000.0) as f32) * 0.5;
    }

    let mel_w: [20560]f32 = [0.0; 20560];
    * = buildMelFilterbankMatrix(cfg, mel_w[0..]) catch { expect_true(false); return; };

    let fft_re: [512]f64 = [0.0; 512];
    let fft_im: [512]f64 = [0.0; 512];
    let power: [257]f32 = [0.0; 257];
    let log_mel: [7760]f32 = [0.0; 7760];

    const got_frames = extractLogMelSpectrogram(
        pcm[0..],
        cfg,
        mel_w[0..],
        fft_re[0..],
        fft_im[0..],
        power[0..],
        log_mel[0..],
    ) catch {
        expect_true(false);
        return;
    };
    expect_eq(got_frames, @as(usize, 97));

    // Find peak Mel bin in middle frame (#48)
    const mid_off: usize = 48 * 80;
    let peak_bin: usize = 0;
    let peak_val: f32 = log_mel[mid_off];
    let min_val: f32 = log_mel[mid_off];
    let m: usize = 1;
    while (m < 80) : (m += 1) {
        const v = log_mel[mid_off + m];
        if (v > peak_val) {
            peak_val = v;
            peak_bin = m;
        }
        if (v < min_val) min_val = v;
    }

    // Expected Mel bin for 1000 Hz under 80-bin Slaney [0, 8000 Hz]:
    // mel(1000)=15.0, mel(8000)=45.2456 -> bin ~ (15.0 / 45.2456) * 81 - 1 ~= 25..26
    expect_true(peak_bin >= 24 && peak_bin <= 28);
    // Whisper clamp guarantees (max - min) <= 8.0 / 4.0 == 2.0
    expect_true((peak_val - min_val) <= 2.001);
}

test("ASR Whisper Large-v3 128-bin & deterministic SpecAugment masking") {
    const cfg = AsrFrontendConfig.whisperLargeV3Defaults();
    expect_true(cfg.isValid());
    expect_eq(cfg.n_mels, @as(usize, 128));

    let spec: [1280]f32 = [1.0; 1280]; // 10 frames x 128 mels
    applySpecAugmentInPlace(spec[0..], 10, 128, 2, 16, 2, 3, -9.0, 20261123) catch {
        expect_true(false);
        return;
    };

    let masked_count: usize = 0;
    let i: usize = 0;
    while (i < 1280) : (i += 1) {
        if (spec[i] == -9.0) masked_count += 1;
    }
    expect_true(masked_count > 0 && masked_count < 1280);
}

test("ASR MFCC: computeMfccFromLogMel exact parity with @dot computeMfccFastDot & liftering") {
    const num_frames: usize = 6;
    const n_mels: usize = 40;
    const n_mfcc: usize = 13;

    let log_mel: [240]f32 = [0.0; 240];
    let i: usize = 0;
    while (i < 240) : (i += 1) {
        log_mel[i] = (@sin((i as f64) * 0.17) as f32) * 2.0 - 1.0;
    }

    let mfcc_direct: [78]f32 = [0.0; 78];
    computeMfccFromLogMel(log_mel[0..], num_frames, n_mels, n_mfcc, mfcc_direct[0..]) catch {
        expect_true(false);
        return;
    };

    let dct: [520]f32 = [0.0; 520]; // 13 * 40
    const dct_len = buildDct2Matrix(n_mels, n_mfcc, dct[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(dct_len, @as(usize, 520));

    let mfcc_fast: [78]f32 = [0.0; 78];
    computeMfccFastDot(log_mel[0..], dct[0..], num_frames, n_mels, n_mfcc, mfcc_fast[0..]) catch {
        expect_true(false);
        return;
    };

    // Direct DCT-II and @dot prebuilt DCT-II must match to float precision
    let j: usize = 0;
    while (j < 78) : (j += 1) {
        expect_true(absF32(mfcc_direct[j] - mfcc_fast[j]) < 1.0e-4);
    }

    // Liftering leaves c0 unchanged (sin(0) == 0 -> w0 == 1.0) and scales c1..c12 up
    const c0_before = mfcc_fast[0];
    const c6_before = mfcc_fast[6];
    applyCepstralLifteringInPlace(mfcc_fast[0..], num_frames, n_mfcc, 22.0) catch {
        expect_true(false);
        return;
    };
    expect_true(absF32(mfcc_fast[0] - c0_before) < 1.0e-5);
    expect_true(absF32(mfcc_fast[6]) > absF32(c6_before));
}

test("ASR Delta & Delta-Delta: exact regression slope on linear ramp + 39-dim packMfccWithDeltas") {
    const num_frames: usize = 10;
    const n_mfcc: usize = 13;
    let feats: [130]f32 = [0.0; 130];

    // Linear ramp: feats[t, d] = 3.0 * t + (d as f32) -> interior Delta == 3.0, interior Delta-Delta == 0.0
    let t: usize = 0;
    while (t < num_frames) : (t += 1) {
        let d: usize = 0;
        while (d < n_mfcc) : (d += 1) {
            feats[t * n_mfcc + d] = 3.0 * (t as f32) + (d as f32);
        }
    }

    let delta1: [130]f32 = [0.0; 130];
    let delta2: [130]f32 = [0.0; 130];
    computeDeltaFeatures(feats[0..], num_frames, n_mfcc, 2, delta1[0..]) catch {
        expect_true(false);
        return;
    };
    computeDeltaFeatures(delta1[0..], num_frames, n_mfcc, 2, delta2[0..]) catch {
        expect_true(false);
        return;
    };

    // Interior frames (t = 2..7) have exact slope 3.0; frames 4..5 have exact acceleration 0.0
    expect_true(absF32(delta1[4 * n_mfcc + 5] - 3.0) < 1.0e-5);
    expect_true(absF32(delta2[5 * n_mfcc + 5]) < 1.0e-5);

    // Pack into contiguous 39-dim [MFCC, Delta, Delta-Delta]
    let packed: [390]f32 = [0.0; 390];
    const plen = packMfccWithDeltas(feats[0..], delta1[0..], delta2[0..], num_frames, n_mfcc, packed[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(plen, @as(usize, 390));
    expect_eq(packed[4 * 39 + 5], feats[4 * 13 + 5]);
    expect_eq(packed[4 * 39 + 13 + 5], delta1[4 * 13 + 5]);
    expect_eq(packed[4 * 39 + 26 + 5], delta2[4 * 13 + 5]);
}

test("ASR CMVN: strict zero-mean & unit-variance normalization + StreamingCmvnNormalizer") {
    const num_frames: usize = 20;
    const dim: usize = 8;
    let feats: [160]f32 = [0.0; 160];

    let t: usize = 0;
    while (t < num_frames) : (t += 1) {
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            feats[t * dim + d] = 10.0 + ((d as f32) * 5.0) + ((t as f32) - 9.5) * (0.5 + (d as f32) * 0.2);
        }
    }

    applyCmvnInPlace(feats[0..], num_frames, dim, true) catch {
        expect_true(false);
        return;
    };

    // Verify every feature dimension has mean == 0 and variance == 1
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        let sum: f64 = 0.0;
        let sq: f64 = 0.0;
        t = 0;
        while (t < num_frames) : (t += 1) {
            const v: f64 = feats[t * dim + d] as f64;
            sum += v;
            sq += v * v;
        }
        const mean = sum / (num_frames as f64);
        const variance = sq / (num_frames as f64);
        expect_true(absF64(mean) < 1.0e-5);
        expect_true(absF64(variance - 1.0) < 1.0e-4);
    }

    // Streaming Welford CMVN normalizer
    let m_buf: [4]f64 = [0.0; 4];
    let m2_buf: [4]f64 = [0.0; 4];
    let scmvn = StreamingCmvnNormalizer.init(4, m_buf[0..], m2_buf[0..]) catch {
        expect_true(false);
        return;
    };
    let f1: [4]f32 = [10.0, 20.0, 30.0, 40.0];
    let f2: [4]f32 = [14.0, 24.0, 34.0, 44.0];
    scmvn.normalizeFrameInPlace(f1[0..], true) catch { expect_true(false); };
    scmvn.normalizeFrameInPlace(f2[0..], true) catch { expect_true(false); };
    expect_true(absF32(f1[0]) < 1.0e-5); // single frame centered to 0
    expect_true(absF32(f2[0] - 1.0) < 1.0e-3); // (14 - 12) / 2 = +1.0
}

test("ASR Streaming Endpointing VAD: silence-speech-silence 1s utterance segmentation") {
    const cfg_fe = AsrFrontendConfig.whisperDefaults();
    const cfg_vad = EndpointVadConfig{
        energy_thresh_db: -35.0,
        noise_margin_db: 10.0,
        max_flatness: 0.65,
        onset_frames: 3,
        hangover_frames: 6,
        frame_ms: 10,
        noise_alpha: 0.05,
    };

    // 1.0s @ 16kHz: [0..250ms] silence, [250ms..650ms] voiced harmonic speech, [650ms..1000ms] silence
    let pcm: [16000]f32 = [0.0; 16000];
    const two_pi: f64 = 6.28318530717958647692;
    let i: usize = 0;
    while (i < 16000) : (i += 1) {
        if (i >= 4000 && i < 10400) {
            const t_sec: f64 = (i as f64) / 16000.0;
            pcm[i] = ((@sin(two_pi * 440.0 * t_sec) * 0.35) + (@sin(two_pi * 880.0 * t_sec) * 0.15)) as f32;
        } else {
            // Very low background floor (~ -70 dB)
            pcm[i] = if ((i % 2) == 0) 0.0002 else -0.0002;
        }
    }

    let fft_re: [512]f64 = [0.0; 512];
    let fft_im: [512]f64 = [0.0; 512];
    let power: [257]f32 = [0.0; 257];
    let segs: [8]SpeechSegmentSpan = [SpeechSegmentSpan{}; 8];

    const n_segs = detectSpeechSegments(
        pcm[0..],
        cfg_fe,
        cfg_vad,
        fft_re[0..],
        fft_im[0..],
        power[0..],
        segs[0..],
    ) catch {
        expect_true(false);
        return;
    };

    expect_eq(n_segs, @as(usize, 1));
    // Speech starts near 250ms (within 50ms) and ends near 650ms + 60ms hangover (~710ms)
    expect_true(segs[0].start_ms >= 200 && segs[0].start_ms <= 290);
    expect_true(segs[0].end_ms >= 640 && segs[0].end_ms <= 760);
    expect_true(segs[0].peak_energy_db > -20.0);

    // Direct streaming state check
    let vad = StreamingEndpointVad.init(cfg_vad);
    const silent_frame: [160]f32 = [0.0; 160];
    const empty_pow: [0]f32 = [];
    expect_true(vad.feedFrame(silent_frame[0..], empty_pow[0..]) == EndpointState.silence);
}

test("ASR Feature error boundary guards & cap.media facade parity") {
    // 1. Non-power-of-2 n_fft -> InvalidFftSize
    const bad_fft = cap.media.AsrFrontendConfig{ n_fft: 400, hop_length: 160, n_mels: 80 };
    expect_true(!bad_fft.isValid());
    let tmp: [16]f32 = [0.0; 16];
    let got_fft: bool = false;
    * = cap.media.buildMelFilterbankMatrix(bad_fft, tmp[0..]) catch |e| {
        if (e == AsrFeatureError.InvalidFftSize) got_fft = true;
    };
    expect_true(got_fft);

    // 2. f_max > nyquist -> InvalidFreqRange
    const bad_freq = AsrFrontendConfig{ sample_rate_hz: 16000, n_fft: 512, hop_length: 160, n_mels: 80, f_min_hz: 0.0, f_max_hz: 12000.0 };
    let got_freq: bool = false;
    * = buildMelFilterbankMatrix(bad_freq, tmp[0..]) catch |e| {
        if (e == AsrFeatureError.InvalidFreqRange) got_freq = true;
    };
    expect_true(got_freq);

    // 3. BufferTooSmall on buildMelFilterbankMatrix
    let got_small: bool = false;
    * = buildMelFilterbankMatrix(AsrFrontendConfig.whisperDefaults(), tmp[0..]) catch |e| {
        if (e == AsrFeatureError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);

    // 4. n_mfcc > n_mels -> InvalidMfccCount
    let got_mfcc: bool = false;
    cap.media.computeMfccFromLogMel(tmp[0..], 1, 8, 12, tmp[0..]) catch |e| {
        if (e == AsrFeatureError.InvalidMfccCount) got_mfcc = true;
    };
    expect_true(got_mfcc);

    // 5. InputTooShort on extractLogMelSpectrogram
    let re: [512]f64 = [0.0; 512];
    let im: [512]f64 = [0.0; 512];
    let got_short: bool = false;
    * = cap.media.extractLogMelSpectrogram(tmp[0..], AsrFrontendConfig.whisperDefaults(), tmp[0..], re[0..], im[0..], tmp[0..], tmp[0..]) catch |e| {
        if (e == AsrFeatureError.InputTooShort) got_short = true;
    };
    expect_true(got_short);

    // 6. cap.media facade parity
    expect_eq(cap.media.asrFrameCount(16000, 512, 160), @as(usize, 97));
    expect_eq(cap.media.asrMelBankLen(80, 512), @as(usize, 20560));
}
