// Source: cp/lib/audio/dsp.cp (Voice AI PCM Resampler, RMS/dB Meter & Energy VAD)
// Pure CP audio DSP utilities for ASR/Whisper 16kHz preprocessing, metering, and energy VAD.

const std = @import("std");
import { AudioError } from "./wav.cp";

/// Detected active speech region in sample indices `[start_sample, end_sample)`.
pub struct SpeechSegment {
    start_sample: usize = 0;
    end_sample: usize = 0;

    /// Returns the number of samples spanned by this speech segment.
    fn lenSamples(): usize {
        if (_.end_sample <= _.start_sample) return 0;
        return _.end_sample - _.start_sample;
    }
}

/// Configurable frame-energy Voice Activity Detector (VAD) with hangover smoothing.
pub struct EnergyVad {
    frame_samples: usize = 480;
    rms_threshold: f64 = 0.02;
    hangover_frames: usize = 5;

    /// Constructs an `EnergyVad` with custom frame length, RMS threshold, and hangover frames.
    static fn init(frame_samples: usize, rms_threshold: f64, hangover_frames: usize): EnergyVad {
        return EnergyVad{
            frame_samples: frame_samples,
            rms_threshold: rms_threshold,
            hangover_frames: hangover_frames,
        };
    }

    /// Segments `samples` into active speech intervals `[start_sample, end_sample)`.
    fn detectSegments(alloc: std.mem.Allocator, samples: []const f32): ([]SpeechSegment)! {
        if (_.frame_samples == 0 || _.rms_threshold < 0.0) {
            return AudioError.InvalidParameter;
        }
        if (samples.len == 0) {
            return alloc.alloc(SpeechSegment, 0) catch return AudioError.OutOfMemory;
        }

        // Pass 1: count speech segments
        let seg_count: usize = 0;
        let in_speech = false;
        let silence_frames: usize = 0;
        let pos: usize = 0;
        while (pos < samples.len) {
            const end = if (pos + _.frame_samples <= samples.len) pos + _.frame_samples else samples.len;
            const rms = AudioDsp.computeRms(samples[pos..end]);
            if (rms >= _.rms_threshold) {
                if (!in_speech) {
                    in_speech = true;
                }
                silence_frames = 0;
            } else if (in_speech) {
                silence_frames += 1;
                if (silence_frames > _.hangover_frames) {
                    seg_count += 1;
                    in_speech = false;
                    silence_frames = 0;
                }
            }
            pos = end;
        }
        if (in_speech) {
            seg_count += 1;
        }

        let segments = alloc.alloc(SpeechSegment, seg_count) catch return AudioError.OutOfMemory;
        if (seg_count == 0) return segments;

        // Pass 2: record exact [start_sample, end_sample) boundaries
        let idx: usize = 0;
        let in_speech2 = false;
        let seg_start: usize = 0;
        let last_end: usize = 0;
        let silence_frames2: usize = 0;
        let pos2: usize = 0;
        while (pos2 < samples.len) {
            const end = if (pos2 + _.frame_samples <= samples.len) pos2 + _.frame_samples else samples.len;
            const rms = AudioDsp.computeRms(samples[pos2..end]);
            if (rms >= _.rms_threshold) {
                if (!in_speech2) {
                    in_speech2 = true;
                    seg_start = pos2;
                }
                last_end = end;
                silence_frames2 = 0;
            } else if (in_speech2) {
                silence_frames2 += 1;
                if (silence_frames2 <= _.hangover_frames) {
                    last_end = end;
                } else {
                    segments[idx] = SpeechSegment{
                        start_sample: seg_start,
                        end_sample: last_end,
                    };
                    idx += 1;
                    in_speech2 = false;
                    silence_frames2 = 0;
                }
            }
            pos2 = end;
        }
        if (in_speech2) {
            segments[idx] = SpeechSegment{
                start_sample: seg_start,
                end_sample: last_end,
            };
        }

        return segments;
    }
}

/// Voice AI PCM conversion, resampling, metering, and test-tone synthesis utilities.
pub static struct AudioDsp {
    /// Converts 16-bit signed PCM samples into normalized `f32` samples in `[-1.0, 1.0]`.
    static fn pcm16ToF32(alloc: std.mem.Allocator, pcm: []const i16): ([]f32)! {
        let out = alloc.alloc(f32, pcm.len) catch return AudioError.OutOfMemory;
        for (i of 0..pcm.len) {
            const s = pcm[i];
            if (s < 0) {
                out[i] = (s as f32) / 32768.0;
            } else {
                out[i] = (s as f32) / 32767.0;
            }
        }
        return out;
    }

    /// Converts `f32` samples to 16-bit signed PCM with `[-1.0, 1.0]` saturation clamping.
    static fn f32ToPcm16(alloc: std.mem.Allocator, floats: []const f32): ([]i16)! {
        let out = alloc.alloc(i16, floats.len) catch return AudioError.OutOfMemory;
        for (i of 0..floats.len) {
            const v = floats[i];
            if (std.math.isNan(v)) {
                out[i] = 0;
            } else if (v >= 1.0) {
                out[i] = 32767;
            } else if (v <= -1.0) {
                out[i] = -32768;
            } else if (v >= 0.0) {
                const scaled: f32 = @round(v * 32767.0);
                out[i] = scaled as i16;
            } else {
                const scaled: f32 = @round(v * 32768.0);
                out[i] = scaled as i16;
            }
        }
        return out;
    }

    /// Downmixes interleaved stereo (`[L0, R0, L1, R1, ...]`) PCM16 samples to mono.
    static fn stereoToMono(alloc: std.mem.Allocator, stereo: []const i16): ([]i16)! {
        if (stereo.len % 2 != 0) return AudioError.InvalidParameter;
        const mono_len = stereo.len / 2;
        let out = alloc.alloc(i16, mono_len) catch return AudioError.OutOfMemory;
        for (i of 0..mono_len) {
            const left = stereo[i * 2] as i32;
            const right = stereo[i * 2 + 1] as i32;
            const avg = @divTrunc(left + right, 2);
            out[i] = avg as i16;
        }
        return out;
    }

    /// Linearly resamples mono `f32` PCM from `from_rate` Hz to `to_rate` Hz (e.g. 48000 -> 16000).
    static fn resampleLinear(
        alloc: std.mem.Allocator,
        samples: []const f32,
        from_rate: u32,
        to_rate: u32,
    ): ([]f32)! {
        if (from_rate == 0 || to_rate == 0) return AudioError.InvalidParameter;
        if (samples.len == 0) {
            return alloc.alloc(f32, 0) catch return AudioError.OutOfMemory;
        }
        if (from_rate == to_rate) {
            let copy = alloc.alloc(f32, samples.len) catch return AudioError.OutOfMemory;
            @memcpy(copy, samples);
            return copy;
        }

        const out_len_u64: u64 = ((samples.len as u64) * (to_rate as u64)) / (from_rate as u64);
        const out_len: usize = out_len_u64 as usize;
        let out = alloc.alloc(f32, out_len) catch return AudioError.OutOfMemory;
        if (out_len == 0) return out;

        const ratio: f64 = (from_rate as f64) / (to_rate as f64);
        for (i of 0..out_len) {
            const src_pos: f64 = (i as f64) * ratio;
            const idx0: usize = src_pos as usize;
            if (idx0 + 1 < samples.len) {
                const frac: f32 = (src_pos - (idx0 as f64)) as f32;
                out[i] = samples[idx0] + (samples[idx0 + 1] - samples[idx0]) * frac;
            } else {
                out[i] = samples[samples.len - 1];
            }
        }
        return out;
    }

    /// Computes Root Mean Square (RMS) energy of `samples`.
    static fn computeRms(samples: []const f32): f64 {
        if (samples.len == 0) return 0.0;
        let sum_sq: f64 = 0.0;
        for (i of 0..samples.len) {
            const s = samples[i] as f64;
            sum_sq += s * s;
        }
        return @sqrt(sum_sq / (samples.len as f64));
    }

    /// Computes RMS loudness in decibels relative to full scale (dBFS, floored at -120.0).
    static fn computeRmsDb(samples: []const f32): f64 {
        const rms = AudioDsp.computeRms(samples);
        if (rms <= 0.000000000001) return -120.0;
        return 20.0 * std.math.log10(rms);
    }

    /// Computes absolute peak amplitude across `samples`.
    static fn computePeak(samples: []const f32): f32 {
        if (samples.len == 0) return 0.0;
        let peak: f32 = 0.0;
        for (i of 0..samples.len) {
            const v = @abs(samples[i]);
            if (v > peak) peak = v;
        }
        return peak;
    }

    /// Computes zero-crossing rate (ZCR) in `[0.0, 1.0]` over consecutive sample pairs.
    static fn zeroCrossingRate(samples: []const f32): f64 {
        if (samples.len < 2) return 0.0;
        let crossings: usize = 0;
        for (i of 1..samples.len) {
            const prev = samples[i - 1];
            const curr = samples[i];
            if ((prev >= 0.0 && curr < 0.0) || (prev < 0.0 && curr >= 0.0)) {
                crossings += 1;
            }
        }
        return (crossings as f64) / ((samples.len - 1) as f64);
    }

    /// Generates a mono `f32` sine wave at `freq_hz` Hz for DSP and VAD testing.
    static fn generateSineWave(
        alloc: std.mem.Allocator,
        freq_hz: f64,
        sample_rate: u32,
        num_samples: usize,
        amplitude: f32,
    ): ([]f32)! {
        if (sample_rate == 0 || freq_hz < 0.0) return AudioError.InvalidParameter;
        let out = alloc.alloc(f32, num_samples) catch return AudioError.OutOfMemory;
        const phase_inc: f64 = (2.0 * std.math.pi * freq_hz) / (sample_rate as f64);
        const amp: f64 = amplitude as f64;
        for (i of 0..num_samples) {
            out[i] = (amp * @sin(phase_inc * (i as f64))) as f32;
        }
        return out;
    }
}
