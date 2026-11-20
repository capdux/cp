// Source: cp/lib/wav/wav.cp (Pure CP RIFF/WAVE PCM Audio Container, Mixer & Resampler)
// Pure in-memory RIFF/WAVE codec supporting PCM 8/16/24/32-bit integer and IEEE Float32.

const std = @import("std");

/// Error set returned by WAV decoding, encoding, and audio DSP operations.
pub err WavError {
    InvalidRiffHeader,
    InvalidWaveFormat,
    UnsupportedSampleFormat,
    MissingDataChunk,
    TruncatedData,
    InvalidChannelCount,
    InvalidSampleRate,
    OutOfMemory,
}

/// Supported RIFF/WAVE sample encodings.
pub enum SampleFormat: u8 {
    Pcm8 = 1,
    Pcm16 = 2,
    Pcm24 = 3,
    Pcm32 = 4,
    Float32 = 5,
}

/// Normalized interleaved f32 audio container (`[-1.0, 1.0]`).
pub struct WavAudio {
    sample_rate: u32 = 0;
    channels: u16 = 0;
    format: SampleFormat = SampleFormat.Pcm16;
    samples_f32: []f32 = [];

    /// Returns total number of audio frames (samples per channel).
    pub fn frameCount(): usize {
        if (_.channels == 0) return 0;
        return _.samples_f32.len / (_.channels as usize);
    }

    /// Returns total audio duration in seconds.
    pub fn durationSec(): f64 {
        if (_.sample_rate == 0 || _.channels == 0) return 0.0;
        const frames = _.frameCount();
        return (frames as f64) / (_.sample_rate as f64);
    }

    /// Frees the owned `samples_f32` buffer using the provided allocator.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.samples_f32.len > 0) {
            allocator.free(_.samples_f32);
            _.samples_f32 = [];
        }
    }
}

fn readU16Le(b: []const u8, off: usize): u16 {
    return (b[off] as u16) | ((b[off + 1] as u16) << 8);
}

fn readU32Le(b: []const u8, off: usize): u32 {
    const b0 = b[off] as u32;
    const b1 = b[off + 1] as u32;
    const b2 = b[off + 2] as u32;
    const b3 = b[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn writeU16Le(b: []u8, off: usize, val: u16) {
    b[off] = (val & 0xFF) as u8;
    b[off + 1] = ((val >> 8) & 0xFF) as u8;
}

fn writeU32Le(b: []u8, off: usize, val: u32) {
    b[off] = (val & 0xFF) as u8;
    b[off + 1] = ((val >> 8) & 0xFF) as u8;
    b[off + 2] = ((val >> 16) & 0xFF) as u8;
    b[off + 3] = ((val >> 24) & 0xFF) as u8;
}

fn writeTag(b: []u8, off: usize, a: u8, c1: u8, c2: u8, c3: u8) {
    b[off] = a;
    b[off + 1] = c1;
    b[off + 2] = c2;
    b[off + 3] = c3;
}

fn tagEql(b: []const u8, off: usize, a: u8, c1: u8, c2: u8, c3: u8): bool {
    return b[off] == a && b[off + 1] == c1 && b[off + 2] == c2 && b[off + 3] == c3;
}

fn formatBitsPerSample(fmt: SampleFormat): u16 {
    return switch (fmt) {
        SampleFormat.Pcm8 => 8,
        SampleFormat.Pcm16 => 16,
        SampleFormat.Pcm24 => 24,
        SampleFormat.Pcm32 => 32,
        SampleFormat.Float32 => 32,
    };
}

fn formatAudioTag(fmt: SampleFormat): u16 {
    return switch (fmt) {
        SampleFormat.Float32 => 3,
        SampleFormat.Pcm8, SampleFormat.Pcm16, SampleFormat.Pcm24, SampleFormat.Pcm32 => 1,
    };
}

fn resolveSampleFormat(audio_tag: u16, bits_per_sample: u16): WavError!SampleFormat {
    if (audio_tag == 1) {
        if (bits_per_sample == 8) return SampleFormat.Pcm8;
        if (bits_per_sample == 16) return SampleFormat.Pcm16;
        if (bits_per_sample == 24) return SampleFormat.Pcm24;
        if (bits_per_sample == 32) return SampleFormat.Pcm32;
        return WavError.UnsupportedSampleFormat;
    }
    if (audio_tag == 3) {
        if (bits_per_sample == 32) return SampleFormat.Float32;
        return WavError.UnsupportedSampleFormat;
    }
    return WavError.UnsupportedSampleFormat;
}

fn clampUnitF32(s: f32): f32 {
    if (std.math.isNan(s)) return 0.0;
    if (s > 1.0) return 1.0;
    if (s < -1.0) return -1.0;
    return s;
}

/// Decodes an in-memory RIFF/WAVE byte stream into normalized `WavAudio` (`[-1.0, 1.0]` f32).
pub fn decode(allocator: std.mem.Allocator, wav_bytes: []const u8): WavError!WavAudio {
    if (wav_bytes.len >= 4 && !tagEql(wav_bytes, 0, 'R', 'I', 'F', 'F')) {
        return WavError.InvalidRiffHeader;
    }
    if (wav_bytes.len < 12) return WavError.TruncatedData;
    if (!tagEql(wav_bytes, 8, 'W', 'A', 'V', 'E')) {
        return WavError.InvalidRiffHeader;
    }

    const riff_size = readU32Le(wav_bytes, 4);
    if (riff_size < 4) return WavError.InvalidRiffHeader;

    const riff_end_u64: u64 = (riff_size as u64) + 8;
    if (riff_end_u64 > (wav_bytes.len as u64)) {
        return WavError.TruncatedData;
    }
    const limit: usize = riff_end_u64 as usize;

    let has_fmt = false;
    let has_data = false;
    let sample_rate: u32 = 0;
    let channels: u16 = 0;
    let sample_format: SampleFormat = SampleFormat.Pcm16;
    let bits_per_sample: u16 = 0;
    let data_start: usize = 0;
    let data_len: usize = 0;

    let pos: usize = 12;
    while (pos < limit) {
        if (pos + 8 > limit) return WavError.TruncatedData;
        const chunk_size = readU32Le(wav_bytes, pos + 4);
        const payload_start = pos + 8;
        const payload_end_u64: u64 = (payload_start as u64) + (chunk_size as u64);
        if (payload_end_u64 > (limit as u64)) return WavError.TruncatedData;
        const payload_end: usize = payload_end_u64 as usize;

        if (tagEql(wav_bytes, pos, 'f', 'm', 't', ' ')) {
            if (chunk_size < 16) return WavError.InvalidWaveFormat;
            const audio_tag = readU16Le(wav_bytes, payload_start);
            const ch = readU16Le(wav_bytes, payload_start + 2);
            const sr = readU32Le(wav_bytes, payload_start + 4);
            const block_align = readU16Le(wav_bytes, payload_start + 12);
            const bps = readU16Le(wav_bytes, payload_start + 14);

            if (ch == 0) return WavError.InvalidChannelCount;
            if (sr == 0) return WavError.InvalidSampleRate;

            const resolved_fmt = resolveSampleFormat(audio_tag, bps)?;
            const expected_align: u32 = (ch as u32) * ((bps / 8) as u32);
            if ((block_align as u32) != expected_align) {
                return WavError.InvalidWaveFormat;
            }

            has_fmt = true;
            channels = ch;
            sample_rate = sr;
            sample_format = resolved_fmt;
            bits_per_sample = bps;
        } else if (tagEql(wav_bytes, pos, 'd', 'a', 't', 'a')) {
            if (!has_data) {
                has_data = true;
                data_start = payload_start;
                data_len = chunk_size as usize;
            }
        }

        const pad: u64 = (chunk_size & 1) as u64;
        const next_pos_u64: u64 = (payload_end as u64) + pad;
        if (next_pos_u64 > (limit as u64)) {
            pos = limit;
        } else {
            pos = next_pos_u64 as usize;
        }
    }

    if (!has_fmt) return WavError.InvalidWaveFormat;
    if (!has_data) return WavError.MissingDataChunk;

    const bytes_per_sample: usize = (bits_per_sample / 8) as usize;
    const frame_bytes: usize = (channels as usize) * bytes_per_sample;
    if (frame_bytes == 0 || (data_len % frame_bytes) != 0) {
        return WavError.TruncatedData;
    }

    const total_samples: usize = data_len / bytes_per_sample;
    let samples_f32 = allocator.alloc(f32, total_samples) catch return WavError.OutOfMemory;

    for (i of 0..total_samples) {
        const off = data_start + (i as usize) * bytes_per_sample;
        switch (sample_format) {
            SampleFormat.Pcm8 => {
                samples_f32[i as usize] = ((wav_bytes[off] as f32) - 128.0) / 128.0;
            },
            SampleFormat.Pcm16 => {
                const s_i16 = @as(i16, @bitCast(readU16Le(wav_bytes, off)));
                samples_f32[i as usize] = (s_i16 as f32) / 32768.0;
            },
            SampleFormat.Pcm24 => {
                const b0 = wav_bytes[off] as u32;
                const b1 = wav_bytes[off + 1] as u32;
                const b2 = wav_bytes[off + 2] as u32;
                let raw24: u32 = b0 | (b1 << 8) | (b2 << 16);
                if ((b2 & 0x80) != 0) {
                    raw24 = raw24 | 0xFF000000;
                }
                const s_i32 = @as(i32, @bitCast(raw24));
                samples_f32[i as usize] = (s_i32 as f32) / 8388608.0;
            },
            SampleFormat.Pcm32 => {
                const s_i32 = @as(i32, @bitCast(readU32Le(wav_bytes, off)));
                samples_f32[i as usize] = ((s_i32 as f64) / 2147483648.0) as f32;
            },
            SampleFormat.Float32 => {
                const v = @as(f32, @bitCast(readU32Le(wav_bytes, off)));
                samples_f32[i as usize] = if (std.math.isNan(v)) 0.0 else v;
            },
        }
    }

    return WavAudio {
        sample_rate: sample_rate,
        channels: channels,
        format: sample_format,
        samples_f32: samples_f32,
    };
}

/// Encodes normalized `WavAudio` into a standard 44-byte header RIFF/WAVE byte stream.
pub fn encode(
    allocator: std.mem.Allocator,
    audio: WavAudio,
    target_format: SampleFormat,
): WavError![]u8 {
    if (audio.channels == 0) return WavError.InvalidChannelCount;
    if (audio.sample_rate == 0) return WavError.InvalidSampleRate;
    if ((audio.samples_f32.len % (audio.channels as usize)) != 0) {
        return WavError.InvalidWaveFormat;
    }

    const audio_tag = formatAudioTag(target_format);
    const bits_per_sample = formatBitsPerSample(target_format);
    const bytes_per_sample: usize = (bits_per_sample / 8) as usize;

    const data_bytes_u64: u64 = (audio.samples_f32.len as u64) * (bytes_per_sample as u64);
    const riff_size_u64: u64 = data_bytes_u64 + 36;
    if (riff_size_u64 > 4294967295) return WavError.InvalidWaveFormat;

    const block_align_u64: u64 = (audio.channels as u64) * (bytes_per_sample as u64);
    const byte_rate_u64: u64 = (audio.sample_rate as u64) * block_align_u64;
    if (block_align_u64 > 65535 || byte_rate_u64 > 4294967295) {
        return WavError.InvalidWaveFormat;
    }

    const total_len: usize = (44 as usize) + (data_bytes_u64 as usize);
    let out = allocator.alloc(u8, total_len) catch return WavError.OutOfMemory;

    writeTag(out, 0, 'R', 'I', 'F', 'F');
    writeU32Le(out, 4, riff_size_u64 as u32);
    writeTag(out, 8, 'W', 'A', 'V', 'E');

    writeTag(out, 12, 'f', 'm', 't', ' ');
    writeU32Le(out, 16, 16);
    writeU16Le(out, 20, audio_tag);
    writeU16Le(out, 22, audio.channels);
    writeU32Le(out, 24, audio.sample_rate);
    writeU32Le(out, 28, byte_rate_u64 as u32);
    writeU16Le(out, 32, block_align_u64 as u16);
    writeU16Le(out, 34, bits_per_sample);

    writeTag(out, 36, 'd', 'a', 't', 'a');
    writeU32Le(out, 40, data_bytes_u64 as u32);

    for (i of 0..audio.samples_f32.len) {
        const off: usize = (44 as usize) + (i as usize) * bytes_per_sample;
        const v = clampUnitF32(audio.samples_f32[i as usize]);

        switch (target_format) {
            SampleFormat.Pcm8 => {
                const scaled = @round((v * 128.0) + 128.0);
                let byte_val: u8 = 0;
                if (scaled <= 0.0) {
                    byte_val = 0;
                } else if (scaled >= 255.0) {
                    byte_val = 255;
                } else {
                    byte_val = @as(u8, @intFromFloat(scaled));
                }
                out[off] = byte_val;
            },
            SampleFormat.Pcm16 => {
                const scaled = @round((v as f64) * 32768.0);
                let s_i16: i16 = 0;
                if (scaled <= -32768.0) {
                    s_i16 = -32768;
                } else if (scaled >= 32767.0) {
                    s_i16 = 32767;
                } else {
                    s_i16 = @as(i16, @intFromFloat(scaled));
                }
                writeU16Le(out, off, @as(u16, @bitCast(s_i16)));
            },
            SampleFormat.Pcm24 => {
                const scaled = @round((v as f64) * 8388608.0);
                let s_i32: i32 = 0;
                if (scaled <= -8388608.0) {
                    s_i32 = -8388608;
                } else if (scaled >= 8388607.0) {
                    s_i32 = 8388607;
                } else {
                    s_i32 = @as(i32, @intFromFloat(scaled));
                }
                const u = @as(u32, @bitCast(s_i32));
                out[off] = (u & 0xFF) as u8;
                out[off + 1] = ((u >> 8) & 0xFF) as u8;
                out[off + 2] = ((u >> 16) & 0xFF) as u8;
            },
            SampleFormat.Pcm32 => {
                const scaled = @round((v as f64) * 2147483648.0);
                let s_i32: i32 = 0;
                if (scaled <= -2147483648.0) {
                    s_i32 = -2147483648;
                } else if (scaled >= 2147483647.0) {
                    s_i32 = 2147483647;
                } else {
                    s_i32 = @as(i32, @intFromFloat(scaled));
                }
                writeU32Le(out, off, @as(u32, @bitCast(s_i32)));
            },
            SampleFormat.Float32 => {
                writeU32Le(out, off, @as(u32, @bitCast(v)));
            },
        }
    }

    return out;
}

/// Mixes multi-channel interleaved audio down to single-channel mono by averaging channels.
pub fn toMono(allocator: std.mem.Allocator, audio: WavAudio): WavError!WavAudio {
    if (audio.channels == 0) return WavError.InvalidChannelCount;
    if (audio.sample_rate == 0) return WavError.InvalidSampleRate;

    const frames = audio.frameCount();
    let mono = allocator.alloc(f32, frames) catch return WavError.OutOfMemory;
    const ch = audio.channels as usize;

    if (ch == 1) {
        for (i of 0..frames) {
            mono[i as usize] = audio.samples_f32[i as usize];
        }
    } else {
        const ch_f64 = ch as f64;
        for (f of 0..frames) {
            let sum: f64 = 0.0;
            const base = (f as usize) * ch;
            for (c of 0..ch) {
                sum += audio.samples_f32[base + (c as usize)] as f64;
            }
            mono[f as usize] = (sum / ch_f64) as f32;
        }
    }

    return WavAudio {
        sample_rate: audio.sample_rate,
        channels: 1,
        format: audio.format,
        samples_f32: mono,
    };
}

/// Resamples interleaved audio to `target_sample_rate` using linear interpolation.
pub fn resampleLinear(
    allocator: std.mem.Allocator,
    audio: WavAudio,
    target_sample_rate: u32,
): WavError!WavAudio {
    if (audio.channels == 0) return WavError.InvalidChannelCount;
    if (audio.sample_rate == 0 || target_sample_rate == 0) return WavError.InvalidSampleRate;

    const in_frames = audio.frameCount();
    const ch = audio.channels as usize;

    if (in_frames == 0) {
        const empty = allocator.alloc(f32, 0) catch return WavError.OutOfMemory;
        return WavAudio {
            sample_rate: target_sample_rate,
            channels: audio.channels,
            format: audio.format,
            samples_f32: empty,
        };
    }

    if (audio.sample_rate == target_sample_rate) {
        let copy = allocator.alloc(f32, in_frames * ch) catch return WavError.OutOfMemory;
        for (i of 0..(in_frames * ch)) {
            copy[i as usize] = audio.samples_f32[i as usize];
        }
        return WavAudio {
            sample_rate: target_sample_rate,
            channels: audio.channels,
            format: audio.format,
            samples_f32: copy,
        };
    }

    let out_frames_u64: u64 = ((in_frames as u64) * (target_sample_rate as u64)) / (audio.sample_rate as u64);
    if (out_frames_u64 == 0) out_frames_u64 = 1;
    const out_frames: usize = out_frames_u64 as usize;

    let out = allocator.alloc(f32, out_frames * ch) catch return WavError.OutOfMemory;
    const src_rate_f64 = audio.sample_rate as f64;
    const dst_rate_f64 = target_sample_rate as f64;

    for (i of 0..out_frames) {
        const src_pos: f64 = ((i as f64) * src_rate_f64) / dst_rate_f64;
        let idx0: usize = @as(usize, @intFromFloat(@floor(src_pos)));
        if (idx0 >= in_frames) idx0 = in_frames - 1;
        const idx1: usize = if (idx0 + 1 < in_frames) idx0 + 1 else idx0;
        const frac: f32 = (src_pos - (idx0 as f64)) as f32;

        const base0 = idx0 * ch;
        const base1 = idx1 * ch;
        const out_base = (i as usize) * ch;
        for (c of 0..ch) {
            const s0 = audio.samples_f32[base0 + (c as usize)];
            const s1 = audio.samples_f32[base1 + (c as usize)];
            out[out_base + (c as usize)] = s0 + (s1 - s0) * frac;
        }
    }

    return WavAudio {
        sample_rate: target_sample_rate,
        channels: audio.channels,
        format: audio.format,
        samples_f32: out,
    };
}

/// Returns the maximum absolute sample amplitude in `audio`.
pub fn peakAmplitude(audio: WavAudio): f32 {
    let peak: f32 = 0.0;
    for (s of audio.samples_f32) {
        if (std.math.isNan(s)) continue;
        const a = @abs(s);
        if (a > peak) peak = a;
    }
    return peak;
}

/// Scales `audio.samples_f32` in-place so its peak amplitude matches `target_peak`.
pub fn normalizePeak(audio: *WavAudio, target_peak: f32) {
    if (target_peak <= 0.0 || std.math.isNan(target_peak)) return;
    const peak = peakAmplitude(audio.*);
    if (peak <= 0.0) return;
    const gain: f32 = target_peak / peak;
    for (i of 0..audio.samples_f32.len) {
        audio.samples_f32[i as usize] = audio.samples_f32[i as usize] * gain;
    }
}
