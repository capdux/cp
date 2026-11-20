// Source: cp/lib/audio/wav.cp (RIFF/WAVE PCM16 & Float32 Audio Codec)
// Pure CP RIFF/WAVE audio container codec supporting PCM16 and IEEE Float32 streams.

const std = @import("std");

/// Error set returned by WAV audio encoding, decoding, and DSP routines.
pub err AudioError {
    InvalidHeader,
    UnsupportedFormat,
    TruncatedData,
    InvalidParameter,
    OutOfMemory,
}

/// Decoded 16-bit PCM WAV audio buffer with sample format metadata.
pub struct WavAudio {
    allocator: std.mem.Allocator = std.heap.page_allocator;
    sample_rate: u32 = 0;
    channels: u16 = 0;
    bits_per_sample: u16 = 0;
    samples: []i16 = [];

    /// Returns total audio duration in milliseconds.
    fn durationMs(): u64 {
        if (_.sample_rate == 0 || _.channels == 0) return 0;
        const frames: u64 = (_.samples.len / (_.channels as usize)) as u64;
        return (frames * 1000) / (_.sample_rate as u64);
    }

    /// Releases the sample buffer owned by this `WavAudio`.
    fn deinit(*_) {
        if (_.samples.len > 0) {
            _.allocator.free(_.samples);
            _.samples = [];
        }
    }
}

/// Decoded IEEE 32-bit float WAV audio buffer with sample format metadata.
pub struct WavFloatAudio {
    allocator: std.mem.Allocator = std.heap.page_allocator;
    sample_rate: u32 = 0;
    channels: u16 = 0;
    bits_per_sample: u16 = 0;
    samples: []f32 = [];

    /// Returns total audio duration in milliseconds.
    fn durationMs(): u64 {
        if (_.sample_rate == 0 || _.channels == 0) return 0;
        const frames: u64 = (_.samples.len / (_.channels as usize)) as u64;
        return (frames * 1000) / (_.sample_rate as u64);
    }

    /// Releases the float sample buffer owned by this `WavFloatAudio`.
    fn deinit(*_) {
        if (_.samples.len > 0) {
            _.allocator.free(_.samples);
            _.samples = [];
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

fn writeWaveHeader(
    buf: []u8,
    audio_format: u16,
    channels: u16,
    sample_rate: u32,
    bits_per_sample: u16,
    data_bytes_u32: u32,
): bool {
    const bytes_per_sample: u64 = (bits_per_sample / 8) as u64;
    const block_align_u64: u64 = (channels as u64) * bytes_per_sample;
    const byte_rate_u64: u64 = (sample_rate as u64) * block_align_u64;
    if (block_align_u64 > 65535 || byte_rate_u64 > 4294967295) return false;

    const riff_size_u64: u64 = (data_bytes_u32 as u64) + 36;
    if (riff_size_u64 > 4294967295) return false;

    writeTag(buf, 0, 'R', 'I', 'F', 'F');
    writeU32Le(buf, 4, riff_size_u64 as u32);
    writeTag(buf, 8, 'W', 'A', 'V', 'E');

    writeTag(buf, 12, 'f', 'm', 't', ' ');
    writeU32Le(buf, 16, 16);
    writeU16Le(buf, 20, audio_format);
    writeU16Le(buf, 22, channels);
    writeU32Le(buf, 24, sample_rate);
    writeU32Le(buf, 28, byte_rate_u64 as u32);
    writeU16Le(buf, 32, block_align_u64 as u16);
    writeU16Le(buf, 34, bits_per_sample);

    writeTag(buf, 36, 'd', 'a', 't', 'a');
    writeU32Le(buf, 40, data_bytes_u32);
    return true;
}

struct ParsedWavChunks {
    sample_rate: u32 = 0;
    channels: u16 = 0;
    bits_per_sample: u16 = 0;
    data_start: usize = 0;
    data_len: usize = 0;
}

fn parseWavChunks(bytes: []const u8, expected_format: u16, expected_bits: u16): ParsedWavChunks! {
    if (bytes.len < 4) return AudioError.TruncatedData;
    if (!tagEql(bytes, 0, 'R', 'I', 'F', 'F')) return AudioError.InvalidHeader;
    if (bytes.len < 12) return AudioError.TruncatedData;

    const riff_size = readU32Le(bytes, 4);
    if (!tagEql(bytes, 8, 'W', 'A', 'V', 'E')) return AudioError.InvalidHeader;
    if (riff_size < 4) return AudioError.InvalidHeader;

    const riff_end_u64: u64 = (riff_size as u64) + 8;
    if (riff_end_u64 > (bytes.len as u64)) return AudioError.TruncatedData;
    const limit: usize = riff_end_u64 as usize;

    let has_fmt = false;
    let has_data = false;
    let sample_rate: u32 = 0;
    let channels: u16 = 0;
    let bits_per_sample: u16 = 0;
    let data_start: usize = 0;
    let data_len: usize = 0;

    let pos: usize = 12;
    while (pos < limit) {
        if (pos + 8 > limit) return AudioError.TruncatedData;
        const chunk_size: usize = readU32Le(bytes, pos + 4) as usize;
        const payload_start = pos + 8;
        if (chunk_size > limit - payload_start) return AudioError.TruncatedData;

        if (tagEql(bytes, pos, 'f', 'm', 't', ' ')) {
            if (chunk_size < 16) return AudioError.InvalidHeader;
            const audio_format = readU16Le(bytes, payload_start);
            channels = readU16Le(bytes, payload_start + 2);
            sample_rate = readU32Le(bytes, payload_start + 4);
            bits_per_sample = readU16Le(bytes, payload_start + 14);
            if (channels == 0 || sample_rate == 0) return AudioError.InvalidHeader;
            if (audio_format != expected_format || bits_per_sample != expected_bits) {
                return AudioError.UnsupportedFormat;
            }
            has_fmt = true;
        } else if (tagEql(bytes, pos, 'd', 'a', 't', 'a')) {
            data_start = payload_start;
            data_len = chunk_size;
            has_data = true;
        }

        const pad: usize = chunk_size & 1;
        if (payload_start + chunk_size + pad <= limit) {
            pos = payload_start + chunk_size + pad;
        } else {
            pos = payload_start + chunk_size;
        }
    }

    if (!has_fmt) return AudioError.InvalidHeader;
    if (!has_data) return AudioError.TruncatedData;

    const bytes_per_sample: usize = (expected_bits / 8) as usize;
    const frame_bytes: usize = (channels as usize) * bytes_per_sample;
    if (frame_bytes == 0 || data_len % frame_bytes != 0) return AudioError.TruncatedData;

    return ParsedWavChunks{
        sample_rate: sample_rate,
        channels: channels,
        bits_per_sample: bits_per_sample,
        data_start: data_start,
        data_len: data_len,
    };
}

/// RIFF/WAVE PCM16 & IEEE Float32 encoder and decoder.
pub static struct WavCodec {
    /// Encodes interleaved 16-bit PCM samples into a complete RIFF/WAVE byte buffer.
    static fn encodePcm16(
        alloc: std.mem.Allocator,
        samples: []const i16,
        sample_rate: u32,
        channels: u16,
    ): ([]u8)! {
        if (sample_rate == 0 || channels == 0) return AudioError.InvalidParameter;
        if (samples.len % (channels as usize) != 0) return AudioError.InvalidParameter;

        const data_bytes_u64: u64 = (samples.len as u64) * 2;
        if (data_bytes_u64 + 44 > 4294967295) return AudioError.InvalidParameter;
        const data_bytes: usize = data_bytes_u64 as usize;

        let out = alloc.alloc(u8, 44 + data_bytes) catch return AudioError.OutOfMemory;
        if (!writeWaveHeader(out, 1, channels, sample_rate, 16, data_bytes as u32)) {
            alloc.free(out);
            return AudioError.InvalidParameter;
        }

        let i: usize = 0;
        while (i < samples.len) : (i += 1) {
            const u: u16 = @as(u16, @bitCast(samples[i]));
            writeU16Le(out, 44 + i * 2, u);
        }
        return out;
    }

    /// Decodes a RIFF/WAVE PCM16 byte buffer, skipping `LIST`/`JUNK`/`fact` chunks.
    static fn decodePcm16(alloc: std.mem.Allocator, bytes: []const u8): WavAudio! {
        const parsed = parseWavChunks(bytes, 1, 16)?;
        const num_samples = parsed.data_len / 2;
        let samples = alloc.alloc(i16, num_samples) catch return AudioError.OutOfMemory;

        let i: usize = 0;
        while (i < num_samples) : (i += 1) {
            const raw = readU16Le(bytes, parsed.data_start + i * 2);
            samples[i] = @as(i16, @bitCast(raw));
        }

        return WavAudio{
            allocator: alloc,
            sample_rate: parsed.sample_rate,
            channels: parsed.channels,
            bits_per_sample: parsed.bits_per_sample,
            samples: samples,
        };
    }

    /// Encodes interleaved 32-bit IEEE float samples into a RIFF/WAVE byte buffer.
    static fn encodeF32(
        alloc: std.mem.Allocator,
        samples: []const f32,
        sample_rate: u32,
        channels: u16,
    ): ([]u8)! {
        if (sample_rate == 0 || channels == 0) return AudioError.InvalidParameter;
        if (samples.len % (channels as usize) != 0) return AudioError.InvalidParameter;

        const data_bytes_u64: u64 = (samples.len as u64) * 4;
        if (data_bytes_u64 + 44 > 4294967295) return AudioError.InvalidParameter;
        const data_bytes: usize = data_bytes_u64 as usize;

        let out = alloc.alloc(u8, 44 + data_bytes) catch return AudioError.OutOfMemory;
        if (!writeWaveHeader(out, 3, channels, sample_rate, 32, data_bytes as u32)) {
            alloc.free(out);
            return AudioError.InvalidParameter;
        }

        let i: usize = 0;
        while (i < samples.len) : (i += 1) {
            const bits: u32 = @as(u32, @bitCast(samples[i]));
            writeU32Le(out, 44 + i * 4, bits);
        }
        return out;
    }

    /// Decodes an IEEE 32-bit float RIFF/WAVE byte buffer.
    static fn decodeF32(alloc: std.mem.Allocator, bytes: []const u8): WavFloatAudio! {
        const parsed = parseWavChunks(bytes, 3, 32)?;
        const num_samples = parsed.data_len / 4;
        let samples = alloc.alloc(f32, num_samples) catch return AudioError.OutOfMemory;

        let i: usize = 0;
        while (i < num_samples) : (i += 1) {
            const raw = readU32Le(bytes, parsed.data_start + i * 4);
            samples[i] = @as(f32, @bitCast(raw));
        }

        return WavFloatAudio{
            allocator: alloc,
            sample_rate: parsed.sample_rate,
            channels: parsed.channels,
            bits_per_sample: parsed.bits_per_sample,
            samples: samples,
        };
    }
}
