// Source: cp/lib/audio/wav.cp (RIFF/WAVE, RF64, WAVEFORMATEXTENSIBLE, PCM/Float/G.711/IMA-ADPCM Streaming Codec)
// 100% pure CP multimedia audio container & sample codec:
//   - 100% backwards-compatible legacy `AudioError`, `WavAudio`, `WavFloatAudio`, `WavCodec`
//   - Zero-heap external-slice RIFF/WAVE, `WAVE_FORMAT_EXTENSIBLE` (0xFFFE + SubFormat GUIDs),
//     EBU Tech 3306 `RF64` + `ds64` 64-bit header/parser, `fact`, `LIST/INFO`, `cue `, and `smpl` chunks
//   - Streaming `WavStreamWriter` with multi-chunk PCM/Float writes and `finalize()` header patching
//   - Full bit-depth lossless/clamped conversions (U8 / S16LE / S24LE / S32LE / F32LE <-> normalized f32) + TPDF dither
//   - Arbitrary-channel planar/interleaved conversion & mono/stereo mixing helpers
//   - Standard 89-step IMA-ADPCM (Format Tag 0x0011) mono & stereo block encoder/decoder (~4:1 compression)
//   - ITU-T G.711 u-law (0x0007) & A-law (0x0006) WAV container encoder/decoder

const std = @import("std");

// ============================================================================
// 1. Constants, Format Tags, Speaker Masks & Error Set
// ============================================================================

/// Standard RIFF/WAVE format tag: uncompressed linear PCM (`0x0001`).
pub const WAV_FORMAT_PCM: u16 = 0x0001;
/// Standard RIFF/WAVE format tag: IEEE 754 floating-point PCM (`0x0003`).
pub const WAV_FORMAT_IEEE_FLOAT: u16 = 0x0003;
/// Standard RIFF/WAVE format tag: ITU-T G.711 A-law (`0x0006`).
pub const WAV_FORMAT_ALAW: u16 = 0x0006;
/// Standard RIFF/WAVE format tag: ITU-T G.711 u-law (`0x0007`).
pub const WAV_FORMAT_MULAW: u16 = 0x0007;
/// Standard RIFF/WAVE format tag: Microsoft/DVI IMA-ADPCM (`0x0011`).
pub const WAV_FORMAT_IMA_ADPCM: u16 = 0x0011;
/// Standard RIFF/WAVE format tag: `WAVE_FORMAT_EXTENSIBLE` (`0xFFFE`).
pub const WAV_FORMAT_EXTENSIBLE: u16 = 0xFFFE;

// --- Speaker Position Masks (WAVE_FORMAT_EXTENSIBLE dwChannelMask) ---
pub const WAV_SPEAKER_FRONT_LEFT: u32 = 0x00000001;
pub const WAV_SPEAKER_FRONT_RIGHT: u32 = 0x00000002;
pub const WAV_SPEAKER_FRONT_CENTER: u32 = 0x00000004;
pub const WAV_SPEAKER_LOW_FREQUENCY: u32 = 0x00000008;
pub const WAV_SPEAKER_BACK_LEFT: u32 = 0x00000010;
pub const WAV_SPEAKER_BACK_RIGHT: u32 = 0x00000020;
pub const WAV_CHANNEL_MASK_MONO: u32 = 0x00000004;
pub const WAV_CHANNEL_MASK_STEREO: u32 = 0x00000003;
pub const WAV_CHANNEL_MASK_5_1: u32 = 0x0000003F;

/// Error set returned by WAV audio encoding, decoding, and DSP routines.
pub err AudioError {
    InvalidHeader,
    UnsupportedFormat,
    TruncatedData,
    InvalidParameter,
    OutOfMemory,
    BufferTooSmall,
    InvalidChunk,
    InvalidState,
}

/// Supported WAV sample representation and codec formats.
pub enum WavSampleFormat {
    u8_pcm,
    s16_le,
    s24_le,
    s32_le,
    f32_le,
    mulaw,
    alaw,
    ima_adpcm,
    unknown,

    /// Returns nominal bits per sample (`4`, `8`, `16`, `24`, `32`, or `0`).
    fn bitsPerSample(): u16 {
        return match (_) {
            WavSampleFormat.u8_pcm => 8,
            WavSampleFormat.s16_le => 16,
            WavSampleFormat.s24_le => 24,
            WavSampleFormat.s32_le => 32,
            WavSampleFormat.f32_le => 32,
            WavSampleFormat.mulaw => 8,
            WavSampleFormat.alaw => 8,
            WavSampleFormat.ima_adpcm => 4,
            WavSampleFormat.unknown => 0,
        };
    }

    /// Returns bytes per sample for fixed-size formats (`1..4`), or `0` for block-compressed IMA-ADPCM.
    fn bytesPerSample(): usize {
        return match (_) {
            WavSampleFormat.u8_pcm => 1,
            WavSampleFormat.s16_le => 2,
            WavSampleFormat.s24_le => 3,
            WavSampleFormat.s32_le => 4,
            WavSampleFormat.f32_le => 4,
            WavSampleFormat.mulaw => 1,
            WavSampleFormat.alaw => 1,
            WavSampleFormat.ima_adpcm => 0,
            WavSampleFormat.unknown => 0,
        };
    }

    /// Returns the base RIFF/WAVE format tag for this sample format.
    fn formatTag(): u16 {
        return match (_) {
            WavSampleFormat.u8_pcm => WAV_FORMAT_PCM,
            WavSampleFormat.s16_le => WAV_FORMAT_PCM,
            WavSampleFormat.s24_le => WAV_FORMAT_PCM,
            WavSampleFormat.s32_le => WAV_FORMAT_PCM,
            WavSampleFormat.f32_le => WAV_FORMAT_IEEE_FLOAT,
            WavSampleFormat.mulaw => WAV_FORMAT_MULAW,
            WavSampleFormat.alaw => WAV_FORMAT_ALAW,
            WavSampleFormat.ima_adpcm => WAV_FORMAT_IMA_ADPCM,
            WavSampleFormat.unknown => 0,
        };
    }
}

// ============================================================================
// 2. Legacy Backwards-Compatible Types & Codec (`WavAudio`, `WavFloatAudio`, `WavCodec`)
// ============================================================================

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

fn readU64Le(b: []const u8, off: usize): u64 {
    const lo: u64 = readU32Le(b, off) as u64;
    const hi: u64 = readU32Le(b, off + 4) as u64;
    return lo | (hi << 32);
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

fn writeU64Le(b: []u8, off: usize, val: u64) {
    writeU32Le(b, off, (val & 0xFFFFFFFF) as u32);
    writeU32Le(b, off + 4, ((val >> 32) & 0xFFFFFFFF) as u32);
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

fn copySlice(dst: []u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

fn trimNullTail(s: []const u8): []const u8 {
    let end: usize = s.len;
    while (end > 0 && s[end - 1] == 0) {
        end -= 1;
    }
    return s[0..end];
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

/// RIFF/WAVE PCM16 & IEEE Float32 encoder and decoder (backwards-compatible allocator API).
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

        let out = alloc.alloc(u8, data_bytes + 44) catch return AudioError.OutOfMemory;
        if (!writeWaveHeader(out, 1, channels, sample_rate, 16, data_bytes as u32)) {
            alloc.free(out);
            return AudioError.InvalidParameter;
        }

        let i: usize = 0;
        while (i < samples.len) : (i += 1) {
            const u: u16 = @as(u16, @bitCast(samples[i]));
            writeU16Le(out, i * 2 + 44, u);
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

        let out = alloc.alloc(u8, data_bytes + 44) catch return AudioError.OutOfMemory;
        if (!writeWaveHeader(out, 3, channels, sample_rate, 32, data_bytes as u32)) {
            alloc.free(out);
            return AudioError.InvalidParameter;
        }

        let i: usize = 0;
        while (i < samples.len) : (i += 1) {
            const bits: u32 = @as(u32, @bitCast(samples[i]));
            writeU32Le(out, i * 4 + 44, bits);
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

// ============================================================================
// 3. Zero-Heap RIFF/WAVE, WAVE_FORMAT_EXTENSIBLE, RF64/ds64, LIST/INFO, cue & smpl
// ============================================================================

/// Trailing 14 bytes of standard Microsoft KSDATAFORMAT_SUBTYPE GUID (`0000xxxx-0000-0010-8000-00AA00389B71`).
const WAV_SUBFORMAT_GUID_TAIL: [14]u8 = [
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
    0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71,
];

fn writeSubFormatGuid(out: []u8, off: usize, format_tag: u16) {
    writeU16Le(out, off, format_tag);
    let i: usize = 0;
    while (i < 14) : (i += 1) {
        out[off + 2 + i] = WAV_SUBFORMAT_GUID_TAIL[i];
    }
}

/// Extracts the base 16-bit WAVE format tag from a 16-byte `WAVE_FORMAT_EXTENSIBLE` SubFormat GUID, or `0`.
pub fn parseWavSubFormatGuid(guid: []const u8): u16 {
    if (guid.len < 16) return 0;
    let i: usize = 0;
    while (i < 14) : (i += 1) {
        if (guid[i + 2] != WAV_SUBFORMAT_GUID_TAIL[i]) return 0;
    }
    return readU16Le(guid, 0);
}

fn classifySampleFormat(effective_tag: u16, bits: u16): WavSampleFormat {
    if (effective_tag == WAV_FORMAT_PCM) {
        if (bits == 8) return WavSampleFormat.u8_pcm;
        if (bits == 16) return WavSampleFormat.s16_le;
        if (bits == 24) return WavSampleFormat.s24_le;
        if (bits == 32) return WavSampleFormat.s32_le;
        return WavSampleFormat.unknown;
    }
    if (effective_tag == WAV_FORMAT_IEEE_FLOAT && bits == 32) return WavSampleFormat.f32_le;
    if (effective_tag == WAV_FORMAT_MULAW && bits == 8) return WavSampleFormat.mulaw;
    if (effective_tag == WAV_FORMAT_ALAW && bits == 8) return WavSampleFormat.alaw;
    if (effective_tag == WAV_FORMAT_IMA_ADPCM && bits == 4) return WavSampleFormat.ima_adpcm;
    return WavSampleFormat.unknown;
}

/// Zero-copy `LIST/INFO` metadata strings extracted from or written into a WAV container.
pub struct WavInfoMetadata {
    title: []const u8 = "";
    artist: []const u8 = "";
    album: []const u8 = "";
    date: []const u8 = "";
    comment: []const u8 = "";
    software: []const u8 = "";

    fn isEmpty(): bool {
        return title.len == 0 && artist.len == 0 && album.len == 0
            && date.len == 0 && comment.len == 0 && software.len == 0;
    }
}

/// A single RFC/RIFF `cue ` chunk sample marker point (24 bytes on wire).
pub struct WavCuePoint {
    cue_id: u32 = 0;
    position: u32 = 0;
    chunk_id: [4]u8 = ['d', 'a', 't', 'a'];
    chunk_start: u32 = 0;
    block_start: u32 = 0;
    sample_offset: u32 = 0;
}

/// Parsed `smpl` sampler chunk metadata (plus primary loop start/end if present).
pub struct WavSmplInfo {
    manufacturer: u32 = 0;
    product: u32 = 0;
    sample_period_ns: u32 = 0;
    midi_unity_note: u32 = 60;
    midi_pitch_fraction: u32 = 0;
    num_loops: u32 = 0;
    loop_start: u32 = 0;
    loop_end: u32 = 0;
}

/// EBU Tech 3306 RF64 `ds64` 64-bit size table header.
pub struct WavDs64Info {
    riff_size: u64 = 0;
    data_size: u64 = 0;
    sample_count: u64 = 0;
    table_length: u32 = 0;
}

/// Zero-copy view of a single RIFF / RF64 chunk.
pub struct WavChunkView {
    id: [4]u8 = [0, 0, 0, 0];
    size: u64 = 0;
    payload: []const u8 = "";
    offset: usize = 0;

    fn isTag(a: u8, b: u8, c: u8, d: u8): bool {
        return id[0] == a && id[1] == b && id[2] == c && id[3] == d;
    }
}

/// Zero-copy chunk iterator over RIFF/WAVE or RF64/WAVE chunks (supports RF64 `ds64` 64-bit data length).
pub struct WavChunkIter {
    data: []const u8 = "";
    pos: usize = 12;
    limit: usize = 0;
    is_rf64: bool = false;
    ds64_data_size: u64 = 0;
    has_ds64: bool = false;

    static fn init(bytes: []const u8): WavChunkIter ! AudioError {
        if (bytes.len < 12) return AudioError.TruncatedData;
        const is_riff = tagEql(bytes, 0, 'R', 'I', 'F', 'F');
        const is_rf = tagEql(bytes, 0, 'R', 'F', '6', '4');
        if (!is_riff && !is_rf) return AudioError.InvalidHeader;
        if (!tagEql(bytes, 8, 'W', 'A', 'V', 'E')) return AudioError.InvalidHeader;

        let lim: usize = bytes.len;
        if (is_riff) {
            const rsz: u64 = readU32Le(bytes, 4) as u64;
            if (rsz < 4) return AudioError.InvalidHeader;
            const end_u64: u64 = rsz + 8;
            if (end_u64 > (bytes.len as u64)) return AudioError.TruncatedData;
            lim = end_u64 as usize;
        }
        return WavChunkIter{
            data: bytes,
            pos: 12,
            limit: lim,
            is_rf64: is_rf,
            ds64_data_size: 0,
            has_ds64: false,
        };
    }

    fn next(*_): WavChunkView? ! AudioError {
        if (pos >= limit) return null;
        if (pos + 8 > limit) return AudioError.TruncatedData;
        const start_off: usize = pos;
        const raw_sz: u32 = readU32Le(data, pos + 4);
        const payload_start: usize = pos + 8;
        let eff_sz: u64 = raw_sz as u64;

        if (is_rf64 && tagEql(data, pos, 'd', 'a', 't', 'a') && raw_sz == 0xFFFFFFFF && has_ds64) {
            eff_sz = ds64_data_size;
        }
        const avail: u64 = (limit - payload_start) as u64;
        // For RF64 headers describing >4GB files in memory-header-only tests, expose available slice safely
        const slice_len: usize = if (eff_sz > avail) {
            if (is_rf64 && tagEql(data, pos, 'd', 'a', 't', 'a') && raw_sz == 0xFFFFFFFF) {
                return WavChunkView{
                    id: [data[start_off], data[start_off + 1], data[start_off + 2], data[start_off + 3]],
                    size: eff_sz,
                    payload: data[payload_start..limit],
                    offset: start_off,
                };
            }
            return AudioError.TruncatedData;
        } else (eff_sz as usize);

        const view = WavChunkView{
            id: [data[start_off], data[start_off + 1], data[start_off + 2], data[start_off + 3]],
            size: eff_sz,
            payload: data[payload_start..(payload_start + slice_len)],
            offset: start_off,
        };
        if (is_rf64 && view.isTag('d', 's', '6', '4') && view.payload.len >= 28) {
            ds64_data_size = readU64Le(view.payload, 8);
            has_ds64 = true;
        }
        const pad: usize = slice_len & 1;
        if (payload_start + slice_len + pad <= limit) {
            pos = payload_start + slice_len + pad;
        } else {
            pos = payload_start + slice_len;
        }
        return view;
    }
}

/// Comprehensive parsed metadata and zero-copy data view of a RIFF/WAVE or RF64/WAVE container.
pub struct WavParsedInfo {
    is_rf64: bool = false;
    is_extensible: bool = false;
    format_tag: u16 = 0;
    effective_format_tag: u16 = 0;
    sample_format: WavSampleFormat = WavSampleFormat.unknown;
    channels: u16 = 0;
    sample_rate: u32 = 0;
    byte_rate: u32 = 0;
    block_align: u16 = 0;
    bits_per_sample: u16 = 0;
    valid_bits_per_sample: u16 = 0;
    channel_mask: u32 = 0;
    samples_per_block: u16 = 0;
    riff_size_64: u64 = 0;
    data_size_64: u64 = 0;
    fact_sample_length: u64 = 0;
    data_offset: usize = 0;
    data_len: usize = 0;
    data_slice: []const u8 = "";
    info: WavInfoMetadata = WavInfoMetadata{};
    has_smpl: bool = false;
    smpl: WavSmplInfo = WavSmplInfo{};
    cue_count: usize = 0;
}

fn parseListInfo(payload: []const u8, meta: *WavInfoMetadata) {
    if (payload.len < 4 || !tagEql(payload, 0, 'I', 'N', 'F', 'O')) return;
    let off: usize = 4;
    while (off + 8 <= payload.len) {
        const sub_sz: usize = readU32Le(payload, off + 4) as usize;
        const val_start: usize = off + 8;
        if (val_start + sub_sz > payload.len) break;
        const val = trimNullTail(payload[val_start..(val_start + sub_sz)]);
        if (tagEql(payload, off, 'I', 'N', 'A', 'M')) {
            meta.title = val;
        } else if (tagEql(payload, off, 'I', 'A', 'R', 'T')) {
            meta.artist = val;
        } else if (tagEql(payload, off, 'I', 'A', 'L', 'B')) {
            meta.album = val;
        } else if (tagEql(payload, off, 'I', 'C', 'R', 'D')) {
            meta.date = val;
        } else if (tagEql(payload, off, 'I', 'C', 'M', 'T')) {
            meta.comment = val;
        } else if (tagEql(payload, off, 'I', 'S', 'F', 'T')) {
            meta.software = val;
        }
        const pad: usize = sub_sz & 1;
        off = val_start + sub_sz + pad;
    }
}

/// Parses a RIFF/WAVE or RF64/WAVE container (`fmt `, `ds64`, `fact`, `data`, `LIST/INFO`, `cue `, `smpl`)
/// with zero dynamic heap allocation, writing any cue points into caller-provided `out_cues`.
pub fn parseWavContainer(bytes: []const u8, out_cues: []WavCuePoint): WavParsedInfo ! AudioError {
    let it = WavChunkIter.init(bytes)?;
    let out = WavParsedInfo{
        is_rf64: it.is_rf64,
        riff_size_64: if (it.is_rf64) 0 else (readU32Le(bytes, 4) as u64),
    };
    let has_fmt: bool = false;
    let has_data: bool = false;

    while (true) {
        const opt = it.next()?;
        const ch = opt orelse break;

        if (ch.isTag('d', 's', '6', '4')) {
            if (ch.payload.len < 28) return AudioError.InvalidChunk;
            out.riff_size_64 = readU64Le(ch.payload, 0);
            out.data_size_64 = readU64Le(ch.payload, 8);
            out.fact_sample_length = readU64Le(ch.payload, 16);
        } else if (ch.isTag('f', 'm', 't', ' ')) {
            if (ch.payload.len < 16) return AudioError.InvalidHeader;
            out.format_tag = readU16Le(ch.payload, 0);
            out.channels = readU16Le(ch.payload, 2);
            out.sample_rate = readU32Le(ch.payload, 4);
            out.byte_rate = readU32Le(ch.payload, 8);
            out.block_align = readU16Le(ch.payload, 12);
            out.bits_per_sample = readU16Le(ch.payload, 14);
            if (out.channels == 0 || out.sample_rate == 0) return AudioError.InvalidHeader;

            out.effective_format_tag = out.format_tag;
            out.valid_bits_per_sample = out.bits_per_sample;
            if (out.format_tag == WAV_FORMAT_EXTENSIBLE) {
                if (ch.payload.len < 40) return AudioError.InvalidHeader;
                const cb_size = readU16Le(ch.payload, 16);
                if (cb_size < 22) return AudioError.InvalidHeader;
                out.is_extensible = true;
                out.valid_bits_per_sample = readU16Le(ch.payload, 18);
                out.channel_mask = readU32Le(ch.payload, 20);
                out.effective_format_tag = parseWavSubFormatGuid(ch.payload[24..40]);
            } else if (out.format_tag == WAV_FORMAT_IMA_ADPCM && ch.payload.len >= 20) {
                out.samples_per_block = readU16Le(ch.payload, 18);
            }
            out.sample_format = classifySampleFormat(out.effective_format_tag, out.bits_per_sample);
            has_fmt = true;
        } else if (ch.isTag('f', 'a', 'c', 't')) {
            if (ch.payload.len >= 4) {
                const f32_cnt = readU32Le(ch.payload, 0);
                if (!(out.is_rf64 && f32_cnt == 0xFFFFFFFF)) {
                    out.fact_sample_length = f32_cnt as u64;
                }
            }
        } else if (ch.isTag('L', 'I', 'S', 'T')) {
            parseListInfo(ch.payload, *out.info);
        } else if (ch.isTag('c', 'u', 'e', ' ')) {
            if (ch.payload.len >= 4) {
                const n_cues: usize = readU32Le(ch.payload, 0) as usize;
                let ci: usize = 0;
                while (ci < n_cues && ci < out_cues.len) : (ci += 1) {
                    const coff: usize = ci * 24 + 4;
                    if (coff + 24 > ch.payload.len) break;
                    out_cues[ci] = WavCuePoint{
                        cue_id: readU32Le(ch.payload, coff),
                        position: readU32Le(ch.payload, coff + 4),
                        chunk_id: [ch.payload[coff + 8], ch.payload[coff + 9], ch.payload[coff + 10], ch.payload[coff + 11]],
                        chunk_start: readU32Le(ch.payload, coff + 12),
                        block_start: readU32Le(ch.payload, coff + 16),
                        sample_offset: readU32Le(ch.payload, coff + 20),
                    };
                    out.cue_count += 1;
                }
            }
        } else if (ch.isTag('s', 'm', 'p', 'l')) {
            if (ch.payload.len >= 36) {
                out.has_smpl = true;
                out.smpl.manufacturer = readU32Le(ch.payload, 0);
                out.smpl.product = readU32Le(ch.payload, 4);
                out.smpl.sample_period_ns = readU32Le(ch.payload, 8);
                out.smpl.midi_unity_note = readU32Le(ch.payload, 12);
                out.smpl.midi_pitch_fraction = readU32Le(ch.payload, 16);
                out.smpl.num_loops = readU32Le(ch.payload, 28);
                if (out.smpl.num_loops > 0 && ch.payload.len >= 60) {
                    out.smpl.loop_start = readU32Le(ch.payload, 44);
                    out.smpl.loop_end = readU32Le(ch.payload, 48);
                }
            }
        } else if (ch.isTag('d', 'a', 't', 'a')) {
            out.data_offset = ch.offset + 8;
            out.data_len = ch.payload.len;
            out.data_slice = ch.payload;
            if (!out.is_rf64 || out.data_size_64 == 0) {
                out.data_size_64 = ch.size;
            }
            has_data = true;
            if (out.is_rf64 && ch.size > (ch.payload.len as u64)) {
                break; // Header-only RF64 descriptor for >4GB stream
            }
        }
    }

    if (!has_fmt) return AudioError.InvalidHeader;
    if (!has_data) return AudioError.TruncatedData;
    return out;
}

/// Configuration for `WavStreamWriter` and `encodeRf64Header`.
pub struct WavStreamConfig {
    sample_format: WavSampleFormat = WavSampleFormat.s16_le;
    sample_rate: u32 = 48000;
    channels: u16 = 2;
    valid_bits_per_sample: u16 = 0;
    channel_mask: u32 = 0;
    use_extensible: bool = false;
    block_align: u16 = 0;
    samples_per_block: u16 = 0;
}

fn writeFmtChunkBody(out: []u8, pos: usize, cfg: WavStreamConfig): usize ! AudioError {
    if (cfg.sample_rate == 0 || cfg.channels == 0) return AudioError.InvalidParameter;
    const bits: u16 = cfg.sample_format.bitsPerSample();
    if (bits == 0) return AudioError.UnsupportedFormat;

    const tag: u16 = cfg.sample_format.formatTag();
    const balign: u16 = if (cfg.block_align != 0) cfg.block_align else ((cfg.channels as usize) * cfg.sample_format.bytesPerSample()) as u16;
    if (balign == 0) return AudioError.InvalidParameter;

    let byte_rate: u32 = (cfg.sample_rate as u64 * (balign as u64)) as u32;
    if (cfg.sample_format == WavSampleFormat.ima_adpcm && cfg.samples_per_block > 0) {
        byte_rate = (((cfg.sample_rate as u64) * (balign as u64)) / (cfg.samples_per_block as u64)) as u32;
    }

    if (cfg.use_extensible) {
        if (pos + 48 > out.len) return AudioError.BufferTooSmall;
        writeTag(out, pos, 'f', 'm', 't', ' ');
        writeU32Le(out, pos + 4, 40);
        writeU16Le(out, pos + 8, WAV_FORMAT_EXTENSIBLE);
        writeU16Le(out, pos + 10, cfg.channels);
        writeU32Le(out, pos + 12, cfg.sample_rate);
        writeU32Le(out, pos + 16, byte_rate);
        writeU16Le(out, pos + 20, balign);
        writeU16Le(out, pos + 22, bits);
        writeU16Le(out, pos + 24, 22); // cbSize
        writeU16Le(out, pos + 26, if (cfg.valid_bits_per_sample != 0) cfg.valid_bits_per_sample else bits);
        const cmask: u32 = if (cfg.channel_mask != 0) cfg.channel_mask else if (cfg.channels == 1) WAV_CHANNEL_MASK_MONO else WAV_CHANNEL_MASK_STEREO;
        writeU32Le(out, pos + 28, cmask);
        writeSubFormatGuid(out, pos + 32, tag);
        return pos + 48;
    }

    if (cfg.sample_format == WavSampleFormat.ima_adpcm) {
        if (pos + 28 > out.len) return AudioError.BufferTooSmall;
        writeTag(out, pos, 'f', 'm', 't', ' ');
        writeU32Le(out, pos + 4, 20);
        writeU16Le(out, pos + 8, WAV_FORMAT_IMA_ADPCM);
        writeU16Le(out, pos + 10, cfg.channels);
        writeU32Le(out, pos + 12, cfg.sample_rate);
        writeU32Le(out, pos + 16, byte_rate);
        writeU16Le(out, pos + 20, balign);
        writeU16Le(out, pos + 22, 4);
        writeU16Le(out, pos + 24, 2); // cbSize = 2
        writeU16Le(out, pos + 26, cfg.samples_per_block);
        return pos + 28;
    }

    if (pos + 24 > out.len) return AudioError.BufferTooSmall;
    writeTag(out, pos, 'f', 'm', 't', ' ');
    writeU32Le(out, pos + 4, 16);
    writeU16Le(out, pos + 8, tag);
    writeU16Le(out, pos + 10, cfg.channels);
    writeU32Le(out, pos + 12, cfg.sample_rate);
    writeU32Le(out, pos + 16, byte_rate);
    writeU16Le(out, pos + 20, balign);
    writeU16Le(out, pos + 22, bits);
    return pos + 24;
}

/// Encodes an EBU Tech 3306 `RF64` + `ds64` + `fmt ` + `data` header supporting >4GB audio streams.
pub fn encodeRf64Header(
    out: []u8,
    cfg: WavStreamConfig,
    riff_size_64: u64,
    data_size_64: u64,
    sample_count_64: u64,
): usize ! AudioError {
    if (out.len < 80) return AudioError.BufferTooSmall;
    writeTag(out, 0, 'R', 'F', '6', '4');
    writeU32Le(out, 4, 0xFFFFFFFF);
    writeTag(out, 8, 'W', 'A', 'V', 'E');

    // ds64 chunk (28 bytes body)
    writeTag(out, 12, 'd', 's', '6', '4');
    writeU32Le(out, 16, 28);
    writeU64Le(out, 20, riff_size_64);
    writeU64Le(out, 28, data_size_64);
    writeU64Le(out, 36, sample_count_64);
    writeU32Le(out, 44, 0); // tableLength = 0

    let pos: usize = writeFmtChunkBody(out, 48, cfg)?;
    if (pos + 8 > out.len) return AudioError.BufferTooSmall;
    writeTag(out, pos, 'd', 'a', 't', 'a');
    writeU32Le(out, pos + 4, 0xFFFFFFFF);
    return pos + 8;
}

fn appendInfoSubchunk(out: []u8, pos: usize, a: u8, b: u8, c: u8, d: u8, text: []const u8): usize ! AudioError {
    if (text.len == 0) return pos;
    const raw_len: usize = text.len + 1; // include NUL terminator per RIFF INFO spec
    const pad: usize = raw_len & 1;
    if (pos + 8 + raw_len + pad > out.len) return AudioError.BufferTooSmall;
    writeTag(out, pos, a, b, c, d);
    writeU32Le(out, pos + 4, raw_len as u32);
    copySlice(out, pos + 8, text);
    out[pos + 8 + text.len] = 0;
    if (pad == 1) out[pos + 8 + raw_len] = 0;
    return pos + 8 + raw_len + pad;
}

/// Zero-heap streaming RIFF/WAVE writer supporting `WAVE_FORMAT_EXTENSIBLE`, `fact`, `LIST/INFO`,
/// `cue ` points, incremental PCM/Float/raw block writes, and `finalize()` header patching.
pub struct WavStreamWriter {
    buf: []u8 = [];
    pos: usize = 0;
    cfg: WavStreamConfig = WavStreamConfig{};
    fact_samples_off: usize = 0;
    data_size_off: usize = 0;
    data_start: usize = 0;
    data_bytes: usize = 0;
    total_frames: u64 = 0;
    finalized: bool = false;

    /// Initializes a streaming WAV writer on caller-owned `out`, writing `RIFF`, `fmt `, optional `fact` & `LIST/INFO`, and `data`.
    static fn init(out: []u8, config: WavStreamConfig, meta: WavInfoMetadata): WavStreamWriter ! AudioError {
        if (out.len < 64) return AudioError.BufferTooSmall;
        writeTag(out, 0, 'R', 'I', 'F', 'F');
        writeU32Le(out, 4, 0); // patched on finalize()
        writeTag(out, 8, 'W', 'A', 'V', 'E');

        let cur: usize = writeFmtChunkBody(out, 12, config)?;
        let fact_off: usize = 0;
        if (config.sample_format != WavSampleFormat.u8_pcm
            && config.sample_format != WavSampleFormat.s16_le
            && config.sample_format != WavSampleFormat.s24_le
            && config.sample_format != WavSampleFormat.s32_le)
        {
            if (cur + 12 > out.len) return AudioError.BufferTooSmall;
            writeTag(out, cur, 'f', 'a', 'c', 't');
            writeU32Le(out, cur + 4, 4);
            fact_off = cur + 8;
            writeU32Le(out, fact_off, 0);
            cur += 12;
        }

        if (!meta.isEmpty()) {
            if (cur + 12 > out.len) return AudioError.BufferTooSmall;
            const list_sz_off: usize = cur + 4;
            writeTag(out, cur, 'L', 'I', 'S', 'T');
            writeU32Le(out, list_sz_off, 0);
            writeTag(out, cur + 8, 'I', 'N', 'F', 'O');
            const list_body_start: usize = cur + 8;
            cur += 12;
            cur = appendInfoSubchunk(out, cur, 'I', 'N', 'A', 'M', meta.title)?;
            cur = appendInfoSubchunk(out, cur, 'I', 'A', 'R', 'T', meta.artist)?;
            cur = appendInfoSubchunk(out, cur, 'I', 'A', 'L', 'B', meta.album)?;
            cur = appendInfoSubchunk(out, cur, 'I', 'C', 'R', 'D', meta.date)?;
            cur = appendInfoSubchunk(out, cur, 'I', 'C', 'M', 'T', meta.comment)?;
            cur = appendInfoSubchunk(out, cur, 'I', 'S', 'F', 'T', meta.software)?;
            writeU32Le(out, list_sz_off, (cur - list_body_start) as u32);
        }

        if (cur + 8 > out.len) return AudioError.BufferTooSmall;
        writeTag(out, cur, 'd', 'a', 't', 'a');
        const dsz_off: usize = cur + 4;
        writeU32Le(out, dsz_off, 0);
        cur += 8;

        return WavStreamWriter{
            buf: out,
            pos: cur,
            cfg: config,
            fact_samples_off: fact_off,
            data_size_off: dsz_off,
            data_start: cur,
            data_bytes: 0,
            total_frames: 0,
            finalized: false,
        };
    }

    /// Appends raw encoded bytes directly into the `data` chunk, updating `total_frames` by `frames_added`.
    fn writeRawBytes(*_, raw: []const u8, frames_added: u64): usize ! AudioError {
        if (finalized) return AudioError.InvalidState;
        if (pos + raw.len > buf.len) return AudioError.BufferTooSmall;
        copySlice(buf, pos, raw);
        pos += raw.len;
        data_bytes += raw.len;
        total_frames += frames_added;
        return raw.len;
    }

    /// Appends interleaved 16-bit PCM samples into the `data` chunk.
    fn writePcm16(*_, samples: []const i16): usize ! AudioError {
        if (finalized) return AudioError.InvalidState;
        if (cfg.sample_format != WavSampleFormat.s16_le) return AudioError.UnsupportedFormat;
        if (samples.len % (cfg.channels as usize) != 0) return AudioError.InvalidParameter;
        const nbytes: usize = samples.len * 2;
        if (pos + nbytes > buf.len) return AudioError.BufferTooSmall;
        let i: usize = 0;
        while (i < samples.len) : (i += 1) {
            writeU16Le(buf, pos + i * 2, @as(u16, @bitCast(samples[i])));
        }
        pos += nbytes;
        data_bytes += nbytes;
        total_frames += (samples.len / (cfg.channels as usize)) as u64;
        return nbytes;
    }

    /// Appends interleaved 32-bit IEEE float samples into the `data` chunk.
    fn writeF32(*_, samples: []const f32): usize ! AudioError {
        if (finalized) return AudioError.InvalidState;
        if (cfg.sample_format != WavSampleFormat.f32_le) return AudioError.UnsupportedFormat;
        if (samples.len % (cfg.channels as usize) != 0) return AudioError.InvalidParameter;
        const nbytes: usize = samples.len * 4;
        if (pos + nbytes > buf.len) return AudioError.BufferTooSmall;
        let i: usize = 0;
        while (i < samples.len) : (i += 1) {
            writeU32Le(buf, pos + i * 4, @as(u32, @bitCast(samples[i])));
        }
        pos += nbytes;
        data_bytes += nbytes;
        total_frames += (samples.len / (cfg.channels as usize)) as u64;
        return nbytes;
    }

    /// Finalizes the stream, optionally appending a `cue ` chunk, and patches `data`, `fact`, and `RIFF` sizes.
    fn finalizeWithCues(*_, cues: []const WavCuePoint): usize ! AudioError {
        if (finalized) return AudioError.InvalidState;
        writeU32Le(buf, data_size_off, data_bytes as u32);
        if ((data_bytes & 1) != 0) {
            if (pos + 1 > buf.len) return AudioError.BufferTooSmall;
            buf[pos] = 0;
            pos += 1;
        }
        if (fact_samples_off != 0) {
            writeU32Le(buf, fact_samples_off, total_frames as u32);
        }
        if (cues.len > 0) {
            const cue_body: usize = cues.len * 24 + 4;
            if (pos + 8 + cue_body > buf.len) return AudioError.BufferTooSmall;
            writeTag(buf, pos, 'c', 'u', 'e', ' ');
            writeU32Le(buf, pos + 4, cue_body as u32);
            writeU32Le(buf, pos + 8, cues.len as u32);
            let ci: usize = 0;
            while (ci < cues.len) : (ci += 1) {
                const off: usize = pos + 12 + ci * 24;
                const cp = cues[ci];
                writeU32Le(buf, off, cp.cue_id);
                writeU32Le(buf, off + 4, cp.position);
                writeTag(buf, off + 8, cp.chunk_id[0], cp.chunk_id[1], cp.chunk_id[2], cp.chunk_id[3]);
                writeU32Le(buf, off + 12, cp.chunk_start);
                writeU32Le(buf, off + 16, cp.block_start);
                writeU32Le(buf, off + 20, cp.sample_offset);
            }
            pos += 8 + cue_body;
        }
        writeU32Le(buf, 4, (pos - 8) as u32);
        finalized = true;
        return pos;
    }

    /// Finalizes the stream with no cue points and returns total bytes written.
    fn finalize(*_): usize ! AudioError {
        let empty: [0]WavCuePoint = [];
        return finalizeWithCues(empty[0..]);
    }

    fn bytes(): []const u8 {
        return buf[0..pos];
    }
}

// ============================================================================
// 4. Full Sample Bit-Depth Conversions (U8/S16/S24/S32/F32 <-> f32) & Mixing
// ============================================================================

fn clampF32(x: f32): f32 {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

fn nextTpdfDither(state: *u32): f32 {
    // Two uniform [-0.5, 0.5] LCG draws summed -> triangular PDF in [-1.0, 1.0]
    state.* = state.* *% 1664525 +% 1013904223;
    const r1: f32 = (((state.* >> 8) as f32) / 16777216.0) - 0.5;
    state.* = state.* *% 1664525 +% 1013904223;
    const r2: f32 = (((state.* >> 8) as f32) / 16777216.0) - 0.5;
    return r1 + r2;
}

/// Converts unsigned 8-bit PCM (`0..255`, midpoint `128`) to normalized `f32` (`[-1.0, 1.0]`).
pub fn u8ToF32Slice(src: []const u8, dst: []f32): usize ! AudioError {
    if (dst.len < src.len) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = ((src[i] as f32) - 128.0) / 128.0;
    }
    return src.len;
}

/// Converts normalized `f32` (`[-1.0, 1.0]`) to unsigned 8-bit PCM (`0..255`) with strict clamping.
pub fn f32ToU8Slice(src: []const f32, dst: []u8): usize ! AudioError {
    if (dst.len < src.len) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const c = clampF32(src[i]);
        let v: i32 = ((c * 128.0) + 128.5) as i32;
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        dst[i] = v as u8;
    }
    return src.len;
}

/// Converts packed little-endian signed 16-bit PCM bytes to normalized `f32` (`[-1.0, 1.0]`)
/// with 8-way unrolled multiply-by-reciprocal acceleration.
pub fn s16LeToF32Slice(src_bytes: []const u8, dst: []f32): usize ! AudioError {
    if ((src_bytes.len & 1) != 0) return AudioError.TruncatedData;
    const n: usize = src_bytes.len / 2;
    if (dst.len < n) return AudioError.BufferTooSmall;
    const inv: f32 = 1.0 / 32768.0;
    let i: usize = 0;
    while (i + 8 <= n) : (i += 8) {
        const off: usize = i * 2;
        dst[i + 0] = (@as(i16, @bitCast(readU16Le(src_bytes, off + 0))) as f32) * inv;
        dst[i + 1] = (@as(i16, @bitCast(readU16Le(src_bytes, off + 2))) as f32) * inv;
        dst[i + 2] = (@as(i16, @bitCast(readU16Le(src_bytes, off + 4))) as f32) * inv;
        dst[i + 3] = (@as(i16, @bitCast(readU16Le(src_bytes, off + 6))) as f32) * inv;
        dst[i + 4] = (@as(i16, @bitCast(readU16Le(src_bytes, off + 8))) as f32) * inv;
        dst[i + 5] = (@as(i16, @bitCast(readU16Le(src_bytes, off + 10))) as f32) * inv;
        dst[i + 6] = (@as(i16, @bitCast(readU16Le(src_bytes, off + 12))) as f32) * inv;
        dst[i + 7] = (@as(i16, @bitCast(readU16Le(src_bytes, off + 14))) as f32) * inv;
    }
    while (i < n) : (i += 1) {
        const raw: i16 = @as(i16, @bitCast(readU16Le(src_bytes, i * 2)));
        dst[i] = (raw as f32) * inv;
    }
    return n;
}

/// Converts normalized `f32` (`[-1.0, 1.0]`) to packed little-endian S16LE bytes with optional TPDF dither.
pub fn f32ToS16LeSlice(src: []const f32, dst_bytes: []u8, enable_tpdf_dither: bool): usize ! AudioError {
    const need: usize = src.len * 2;
    if (dst_bytes.len < need) return AudioError.BufferTooSmall;
    let rng: u32 = 0x12345678;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        let scaled: f32 = clampF32(src[i]) * 32767.0;
        if (enable_tpdf_dither) {
            scaled += nextTpdfDither(*rng);
        }
        let q: i32 = if (scaled >= 0.0) ((scaled + 0.5) as i32) else ((scaled - 0.5) as i32);
        if (q > 32767) q = 32767;
        if (q < -32768) q = -32768;
        const s16: i16 = q as i16;
        writeU16Le(dst_bytes, i * 2, @as(u16, @bitCast(s16)));
    }
    return need;
}

/// Converts packed 3-byte signed 24-bit little-endian PCM bytes to normalized `f32` (`[-1.0, 1.0]`).
pub fn s24LeToF32Slice(src_bytes: []const u8, dst: []f32): usize ! AudioError {
    if (src_bytes.len % 3 != 0) return AudioError.TruncatedData;
    const n: usize = src_bytes.len / 3;
    if (dst.len < n) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const off: usize = i * 3;
        const raw24: u32 = (src_bytes[off] as u32)
            | ((src_bytes[off + 1] as u32) << 8)
            | ((src_bytes[off + 2] as u32) << 16);
        const sign_ext: u32 = if ((raw24 & 0x00800000) != 0) (raw24 | 0xFF000000) else raw24;
        const s24: i32 = @as(i32, @bitCast(sign_ext));
        dst[i] = (s24 as f32) / 8388608.0;
    }
    return n;
}

/// Converts normalized `f32` (`[-1.0, 1.0]`) to packed 3-byte signed 24-bit little-endian PCM bytes.
pub fn f32ToS24LeSlice(src: []const f32, dst_bytes: []u8): usize ! AudioError {
    const need: usize = src.len * 3;
    if (dst_bytes.len < need) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const scaled: f64 = (clampF32(src[i]) as f64) * 8388607.0;
        let q: i32 = if (scaled >= 0.0) ((scaled + 0.5) as i32) else ((scaled - 0.5) as i32);
        if (q > 8388607) q = 8388607;
        if (q < -8388608) q = -8388608;
        const u: u32 = @as(u32, @bitCast(q));
        const off: usize = i * 3;
        dst_bytes[off] = (u & 0xFF) as u8;
        dst_bytes[off + 1] = ((u >> 8) & 0xFF) as u8;
        dst_bytes[off + 2] = ((u >> 16) & 0xFF) as u8;
    }
    return need;
}

/// Converts packed signed 32-bit little-endian PCM bytes to normalized `f32` (`[-1.0, 1.0]`).
pub fn s32LeToF32Slice(src_bytes: []const u8, dst: []f32): usize ! AudioError {
    if ((src_bytes.len & 3) != 0) return AudioError.TruncatedData;
    const n: usize = src_bytes.len / 4;
    if (dst.len < n) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const s32: i32 = @as(i32, @bitCast(readU32Le(src_bytes, i * 4)));
        dst[i] = ((s32 as f64) / 2147483648.0) as f32;
    }
    return n;
}

/// Converts normalized `f32` (`[-1.0, 1.0]`) to packed signed 32-bit little-endian PCM bytes.
pub fn f32ToS32LeSlice(src: []const f32, dst_bytes: []u8): usize ! AudioError {
    const need: usize = src.len * 4;
    if (dst_bytes.len < need) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const scaled: f64 = (clampF32(src[i]) as f64) * 2147483647.0;
        let q: i64 = if (scaled >= 0.0) ((scaled + 0.5) as i64) else ((scaled - 0.5) as i64);
        if (q > 2147483647) q = 2147483647;
        if (q < -2147483648) q = -2147483648;
        const s32: i32 = q as i32;
        writeU32Le(dst_bytes, i * 4, @as(u32, @bitCast(s32)));
    }
    return need;
}

/// Converts packed IEEE 32-bit float little-endian bytes to `f32` slice.
pub fn f32LeBytesToF32Slice(src_bytes: []const u8, dst: []f32): usize ! AudioError {
    if ((src_bytes.len & 3) != 0) return AudioError.TruncatedData;
    const n: usize = src_bytes.len / 4;
    if (dst.len < n) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = @as(f32, @bitCast(readU32Le(src_bytes, i * 4)));
    }
    return n;
}

/// Converts `f32` slice to packed IEEE 32-bit float little-endian bytes.
pub fn f32SliceToF32LeBytes(src: []const f32, dst_bytes: []u8): usize ! AudioError {
    const need: usize = src.len * 4;
    if (dst_bytes.len < need) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        writeU32Le(dst_bytes, i * 4, @as(u32, @bitCast(src[i])));
    }
    return need;
}

/// Downmixes interleaved stereo `f32` (`L, R, L, R...`) to mono `f32` (`0.5 * (L + R)`)
/// using 8-way unrolled vector-friendly loops.
pub fn stereoToMonoF32(stereo: []const f32, out_mono: []f32): usize ! AudioError {
    if ((stereo.len & 1) != 0) return AudioError.InvalidParameter;
    const frames: usize = stereo.len / 2;
    if (out_mono.len < frames) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i + 8 <= frames) : (i += 8) {
        const s: usize = i * 2;
        out_mono[i + 0] = (stereo[s + 0] + stereo[s + 1]) * 0.5;
        out_mono[i + 1] = (stereo[s + 2] + stereo[s + 3]) * 0.5;
        out_mono[i + 2] = (stereo[s + 4] + stereo[s + 5]) * 0.5;
        out_mono[i + 3] = (stereo[s + 6] + stereo[s + 7]) * 0.5;
        out_mono[i + 4] = (stereo[s + 8] + stereo[s + 9]) * 0.5;
        out_mono[i + 5] = (stereo[s + 10] + stereo[s + 11]) * 0.5;
        out_mono[i + 6] = (stereo[s + 12] + stereo[s + 13]) * 0.5;
        out_mono[i + 7] = (stereo[s + 14] + stereo[s + 15]) * 0.5;
    }
    while (i < frames) : (i += 1) {
        out_mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5;
    }
    return frames;
}

/// Upmixes mono `f32` to interleaved stereo `f32` (`L = R = M`).
pub fn monoToStereoF32(mono: []const f32, out_stereo: []f32): usize ! AudioError {
    const need: usize = mono.len * 2;
    if (out_stereo.len < need) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < mono.len) : (i += 1) {
        out_stereo[i * 2] = mono[i];
        out_stereo[i * 2 + 1] = mono[i];
    }
    return need;
}

/// Interleaves arbitrary-channel planar `f32` (`[ch0_0..ch0_N, ch1_0..ch1_N, ...]`) into `[f0_c0, f0_c1, ...]`.
pub fn interleaveChannelsF32(
    planar: []const f32,
    channels: usize,
    frames: usize,
    out_interleaved: []f32,
): usize ! AudioError {
    if (channels == 0) return AudioError.InvalidParameter;
    const total: usize = channels * frames;
    if (planar.len < total) return AudioError.TruncatedData;
    if (out_interleaved.len < total) return AudioError.BufferTooSmall;
    let f: usize = 0;
    while (f < frames) : (f += 1) {
        let c: usize = 0;
        while (c < channels) : (c += 1) {
            out_interleaved[f * channels + c] = planar[c * frames + f];
        }
    }
    return total;
}

/// Deinterleaves arbitrary-channel interleaved `f32` (`[f0_c0, f0_c1, ...]`) into planar `[ch0_0..ch0_N, ch1_0..ch1_N, ...]`.
pub fn deinterleaveChannelsF32(
    interleaved: []const f32,
    channels: usize,
    frames: usize,
    out_planar: []f32,
): usize ! AudioError {
    if (channels == 0) return AudioError.InvalidParameter;
    const total: usize = channels * frames;
    if (interleaved.len < total) return AudioError.TruncatedData;
    if (out_planar.len < total) return AudioError.BufferTooSmall;
    let f: usize = 0;
    while (f < frames) : (f += 1) {
        let c: usize = 0;
        while (c < channels) : (c += 1) {
            out_planar[c * frames + f] = interleaved[f * channels + c];
        }
    }
    return total;
}

// ============================================================================
// 5. IMA-ADPCM (Format Tag 0x0011) 89-Step Block Encoder & Decoder
// ============================================================================

const IMA_STEP_TABLE: [89]i32 = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
];

const IMA_INDEX_TABLE: [16]i32 = [
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
];

/// Per-channel IMA-ADPCM predictor & step index state machine.
pub struct ImaAdpcmState {
    predictor: i32 = 0;
    step_index: i32 = 0;

    /// Encodes one 16-bit signed PCM sample into a 4-bit IMA-ADPCM nibble (`0..15`).
    fn encodeSample(*_, sample: i16): u8 {
        let step: i32 = IMA_STEP_TABLE[step_index as usize];
        let diff: i32 = (sample as i32) - predictor;
        let nibble: u8 = 0;
        if (diff < 0) {
            nibble = 8;
            diff = -diff;
        }
        let vpdiff: i32 = step >> 3;
        if (diff >= step) {
            nibble |= 4;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if (diff >= step) {
            nibble |= 2;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if (diff >= step) {
            nibble |= 1;
            vpdiff += step;
        }
        if ((nibble & 8) != 0) {
            predictor -= vpdiff;
        } else {
            predictor += vpdiff;
        }
        if (predictor > 32767) predictor = 32767;
        if (predictor < -32768) predictor = -32768;

        step_index += IMA_INDEX_TABLE[nibble as usize];
        if (step_index < 0) step_index = 0;
        if (step_index > 88) step_index = 88;
        return nibble;
    }

    /// Decodes one 4-bit IMA-ADPCM nibble (`0..15`) into a 16-bit signed PCM sample.
    fn decodeNibble(*_, nibble: u8): i16 {
        const nib: u8 = nibble & 0x0F;
        const step: i32 = IMA_STEP_TABLE[step_index as usize];
        let vpdiff: i32 = step >> 3;
        if ((nib & 4) != 0) vpdiff += step;
        if ((nib & 2) != 0) vpdiff += step >> 1;
        if ((nib & 1) != 0) vpdiff += step >> 2;
        if ((nib & 8) != 0) {
            predictor -= vpdiff;
        } else {
            predictor += vpdiff;
        }
        if (predictor > 32767) predictor = 32767;
        if (predictor < -32768) predictor = -32768;

        step_index += IMA_INDEX_TABLE[nib as usize];
        if (step_index < 0) step_index = 0;
        if (step_index > 88) step_index = 88;
        return predictor as i16;
    }
}

/// Computes the exact number of PCM frames (samples per channel) encoded in one IMA-ADPCM block.
pub fn imaAdpcmBlockSamples(block_align: usize, channels: usize): usize {
    if (channels == 0 || block_align < channels * 4) return 0;
    const data_bytes: usize = block_align - channels * 4;
    if (channels == 1) {
        return data_bytes * 2 + 1;
    }
    // Stereo WAV IMA-ADPCM interleaves 4-byte (8-sample) words per channel
    const words_per_ch: usize = data_bytes / (channels * 4);
    return words_per_ch * 8 + 1;
}

/// Encodes one mono or stereo IMA-ADPCM WAV block (`[predictor_i16, step_index_u8, 0]` header per channel + nibbles).
/// Returns the encoded block length in bytes.
pub fn encodeImaAdpcmBlock(pcm: []const i16, channels: usize, out_block: []u8): usize ! AudioError {
    if (channels != 1 && channels != 2) return AudioError.UnsupportedFormat;
    if (pcm.len == 0 || pcm.len % channels != 0) return AudioError.InvalidParameter;
    const frames: usize = pcm.len / channels;
    if (frames == 0) return AudioError.InvalidParameter;

    if (channels == 1) {
        const rem_samples: usize = frames - 1;
        const nibble_bytes: usize = (rem_samples + 1) / 2;
        const block_len: usize = nibble_bytes + 4;
        if (out_block.len < block_len) return AudioError.BufferTooSmall;

        let st = ImaAdpcmState{ predictor: pcm[0] as i32, step_index: 0 };
        writeU16Le(out_block, 0, @as(u16, @bitCast(pcm[0])));
        out_block[2] = 0;
        out_block[3] = 0;

        let f: usize = 1;
        let boff: usize = 4;
        while (f < frames) {
            const lo: u8 = st.encodeSample(pcm[f]);
            f += 1;
            const hi: u8 = if (f < frames) st.encodeSample(pcm[f]) else 0;
            if (f < frames) f += 1;
            out_block[boff] = lo | (hi << 4);
            boff += 1;
        }
        return block_len;
    }

    // Stereo (channels == 2): 8-byte header + 4-byte (8-sample) interleaved channel words
    const rem_frames: usize = frames - 1;
    const words: usize = (rem_frames + 7) / 8;
    const block_len: usize = words * 8 + 8;
    if (out_block.len < block_len) return AudioError.BufferTooSmall;

    let st_l = ImaAdpcmState{ predictor: pcm[0] as i32, step_index: 0 };
    let st_r = ImaAdpcmState{ predictor: pcm[1] as i32, step_index: 0 };
    writeU16Le(out_block, 0, @as(u16, @bitCast(pcm[0])));
    out_block[2] = 0;
    out_block[3] = 0;
    writeU16Le(out_block, 4, @as(u16, @bitCast(pcm[1])));
    out_block[6] = 0;
    out_block[7] = 0;

    let f: usize = 1;
    let boff: usize = 8;
    let w: usize = 0;
    while (w < words) : (w += 1) {
        // 4 bytes (8 nibbles) for Left channel
        let b_l: usize = 0;
        while (b_l < 4) : (b_l += 1) {
            const idx0: usize = f + b_l * 2;
            const idx1: usize = idx0 + 1;
            const lo_l: u8 = if (idx0 < frames) st_l.encodeSample(pcm[idx0 * 2]) else 0;
            const hi_l: u8 = if (idx1 < frames) st_l.encodeSample(pcm[idx1 * 2]) else 0;
            out_block[boff + b_l] = lo_l | (hi_l << 4);
        }
        // 4 bytes (8 nibbles) for Right channel
        let b_r: usize = 0;
        while (b_r < 4) : (b_r += 1) {
            const idx0: usize = f + b_r * 2;
            const idx1: usize = idx0 + 1;
            const lo_r: u8 = if (idx0 < frames) st_r.encodeSample(pcm[idx0 * 2 + 1]) else 0;
            const hi_r: u8 = if (idx1 < frames) st_r.encodeSample(pcm[idx1 * 2 + 1]) else 0;
            out_block[boff + 4 + b_r] = lo_r | (hi_r << 4);
        }
        f += 8;
        boff += 8;
    }
    return block_len;
}

/// Decodes one mono or stereo IMA-ADPCM WAV block into interleaved `out_pcm`, returning sample count written.
pub fn decodeImaAdpcmBlock(block: []const u8, channels: usize, out_pcm: []i16): usize ! AudioError {
    if (channels != 1 && channels != 2) return AudioError.UnsupportedFormat;
    if (block.len < channels * 4) return AudioError.TruncatedData;

    if (channels == 1) {
        const pred: i16 = @as(i16, @bitCast(readU16Le(block, 0)));
        const idx_u8: u8 = block[2];
        if (idx_u8 > 88) return AudioError.InvalidChunk;
        const data_bytes: usize = block.len - 4;
        const total_samples: usize = data_bytes * 2 + 1;
        if (out_pcm.len < total_samples) return AudioError.BufferTooSmall;

        let st = ImaAdpcmState{ predictor: pred as i32, step_index: idx_u8 as i32 };
        out_pcm[0] = pred;
        let out_i: usize = 1;
        let bi: usize = 4;
        while (bi < block.len) : (bi += 1) {
            const byte = block[bi];
            out_pcm[out_i] = st.decodeNibble(byte & 0x0F);
            out_pcm[out_i + 1] = st.decodeNibble((byte >> 4) & 0x0F);
            out_i += 2;
        }
        return total_samples;
    }

    // Stereo
    const pred_l: i16 = @as(i16, @bitCast(readU16Le(block, 0)));
    const idx_l: u8 = block[2];
    const pred_r: i16 = @as(i16, @bitCast(readU16Le(block, 4)));
    const idx_r: u8 = block[6];
    if (idx_l > 88 || idx_r > 88) return AudioError.InvalidChunk;

    const words: usize = (block.len - 8) / 8;
    const total_frames: usize = words * 8 + 1;
    const total_samples: usize = total_frames * 2;
    if (out_pcm.len < total_samples) return AudioError.BufferTooSmall;

    let st_l = ImaAdpcmState{ predictor: pred_l as i32, step_index: idx_l as i32 };
    let st_r = ImaAdpcmState{ predictor: pred_r as i32, step_index: idx_r as i32 };
    out_pcm[0] = pred_l;
    out_pcm[1] = pred_r;

    let frame_base: usize = 1;
    let boff: usize = 8;
    let w: usize = 0;
    while (w < words) : (w += 1) {
        let b: usize = 0;
        while (b < 4) : (b += 1) {
            const bl = block[boff + b];
            out_pcm[(frame_base + b * 2) * 2] = st_l.decodeNibble(bl & 0x0F);
            out_pcm[(frame_base + b * 2 + 1) * 2] = st_l.decodeNibble((bl >> 4) & 0x0F);
            const br = block[boff + 4 + b];
            out_pcm[(frame_base + b * 2) * 2 + 1] = st_r.decodeNibble(br & 0x0F);
            out_pcm[(frame_base + b * 2 + 1) * 2 + 1] = st_r.decodeNibble((br >> 4) & 0x0F);
        }
        frame_base += 8;
        boff += 8;
    }
    return total_samples;
}

// ============================================================================
// 6. ITU-T G.711 u-law (0x0007) & A-law (0x0006) WAV Container Codecs
// ============================================================================

/// Encodes one 16-bit linear PCM sample into ITU-T G.711 u-law byte.
pub fn wavPcm16ToMulaw(sample: i16): u8 {
    let s: i32 = sample as i32;
    let sign: u8 = 0;
    if (s < 0) {
        sign = 0x80;
        s = -s;
    }
    if (s > 32635) s = 32635;
    s += 132;
    let exp: u8 = 7;
    let exp_mask: i32 = 0x4000;
    while (exp > 0 && (s & exp_mask) == 0) {
        exp -= 1;
        exp_mask >>= 1;
    }
    const mantissa: u8 = ((s >> (@as(u5, @intCast((exp as u32) + 3)))) & 0x0F) as u8;
    return ~(sign | (exp << 4) | mantissa);
}

/// Decodes one ITU-T G.711 u-law byte into a 16-bit linear PCM sample.
pub fn wavMulawToPcm16(u_val: u8): i16 {
    const inv: u8 = ~u_val;
    const sign: i32 = if ((inv & 0x80) != 0) -1 else 1;
    const exp: u5 = @as(u5, @intCast((inv >> 4) & 0x07));
    const mant: i32 = (inv & 0x0F) as i32;
    const mag: i32 = (((mant << 3) + 132) << exp) - 132;
    return (sign * mag) as i16;
}

/// Encodes one 16-bit linear PCM sample into ITU-T G.711 A-law byte.
pub fn wavPcm16ToAlaw(sample: i16): u8 {
    let s: i32 = sample as i32;
    let sign: u8 = 0;
    if (s >= 0) {
        sign = 0x80;
    } else {
        s = -s - 1;
        if (s < 0) s = 0;
    }
    if (s > 32767) s = 32767;
    let out_byte: u8 = 0;
    if (s < 256) {
        out_byte = (s >> 4) as u8;
    } else {
        let exp: u8 = 7;
        let mask: i32 = 0x4000;
        while (exp > 1 && (s & mask) == 0) {
            exp -= 1;
            mask >>= 1;
        }
        const mant: u8 = ((s >> (@as(u5, @intCast((exp as u32) + 3)))) & 0x0F) as u8;
        out_byte = (exp << 4) | mant;
    }
    return (sign | out_byte) ^ 0x55;
}

/// Decodes one ITU-T G.711 A-law byte into a 16-bit linear PCM sample.
pub fn wavAlawToPcm16(a_val: u8): i16 {
    const x: u8 = a_val ^ 0x55;
    const sign: i32 = if ((x & 0x80) != 0) 1 else -1;
    const exp: u8 = (x >> 4) & 0x07;
    const mant: i32 = (x & 0x0F) as i32;
    let mag: i32 = 0;
    if (exp == 0) {
        mag = (mant << 4) + 8;
    } else {
        const shift: u5 = @as(u5, @intCast((exp as u32) + 3));
        mag = ((mant << 1) + 33) << shift;
    }
    return (sign * mag) as i16;
}

/// Encodes interleaved PCM16 samples into a complete RIFF/WAVE G.711 u-law (`0x0007`) container with `fact` chunk.
pub fn encodeWavMulaw(
    samples: []const i16,
    sample_rate: u32,
    channels: u16,
    out: []u8,
): usize ! AudioError {
    if (sample_rate == 0 || channels == 0 || samples.len % (channels as usize) != 0) {
        return AudioError.InvalidParameter;
    }
    let w = WavStreamWriter.init(out, WavStreamConfig{
        sample_format: WavSampleFormat.mulaw,
        sample_rate: sample_rate,
        channels: channels,
    }, WavInfoMetadata{})?;
    if (w.pos + samples.len > out.len) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < samples.len) : (i += 1) {
        out[w.pos + i] = wavPcm16ToMulaw(samples[i]);
    }
    w.pos += samples.len;
    w.data_bytes += samples.len;
    w.total_frames += (samples.len / (channels as usize)) as u64;
    return w.finalize();
}

/// Decodes a RIFF/WAVE G.711 u-law (`0x0007`) container into caller-provided `out_samples`.
pub fn decodeWavMulaw(wav_bytes: []const u8, out_samples: []i16): WavParsedInfo ! AudioError {
    let no_cues: [0]WavCuePoint = [];
    const info = parseWavContainer(wav_bytes, no_cues[0..])?;
    if (info.effective_format_tag != WAV_FORMAT_MULAW) return AudioError.UnsupportedFormat;
    if (out_samples.len < info.data_slice.len) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < info.data_slice.len) : (i += 1) {
        out_samples[i] = wavMulawToPcm16(info.data_slice[i]);
    }
    return info;
}

/// Encodes interleaved PCM16 samples into a complete RIFF/WAVE G.711 A-law (`0x0006`) container with `fact` chunk.
pub fn encodeWavAlaw(
    samples: []const i16,
    sample_rate: u32,
    channels: u16,
    out: []u8,
): usize ! AudioError {
    if (sample_rate == 0 || channels == 0 || samples.len % (channels as usize) != 0) {
        return AudioError.InvalidParameter;
    }
    let w = WavStreamWriter.init(out, WavStreamConfig{
        sample_format: WavSampleFormat.alaw,
        sample_rate: sample_rate,
        channels: channels,
    }, WavInfoMetadata{})?;
    if (w.pos + samples.len > out.len) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < samples.len) : (i += 1) {
        out[w.pos + i] = wavPcm16ToAlaw(samples[i]);
    }
    w.pos += samples.len;
    w.data_bytes += samples.len;
    w.total_frames += (samples.len / (channels as usize)) as u64;
    return w.finalize();
}

/// Decodes a RIFF/WAVE G.711 A-law (`0x0006`) container into caller-provided `out_samples`.
pub fn decodeWavAlaw(wav_bytes: []const u8, out_samples: []i16): WavParsedInfo ! AudioError {
    let no_cues: [0]WavCuePoint = [];
    const info = parseWavContainer(wav_bytes, no_cues[0..])?;
    if (info.effective_format_tag != WAV_FORMAT_ALAW) return AudioError.UnsupportedFormat;
    if (out_samples.len < info.data_slice.len) return AudioError.BufferTooSmall;
    let i: usize = 0;
    while (i < info.data_slice.len) : (i += 1) {
        out_samples[i] = wavAlawToPcm16(info.data_slice[i]);
    }
    return info;
}

// ============================================================================
// Native SIMD `@dot` Audio Energy & 8-Lane Gain Mixing Kernels
// ============================================================================

/// Computes the exact Root-Mean-Square (RMS) amplitude of an `f32` audio slice
/// using hardware SIMD `@dot(samples, samples)` inner product acceleration.
pub fn computeAudioRmsSimdF32(samples: []const f32): f64 {
    if (samples.len == 0) return 0.0;
    const energy: f64 = @as(f64, @dot(samples, samples));
    if (energy <= 0.0 || std.math.isNan(energy) || std.math.isInf(energy)) return 0.0;
    return std.math.sqrt(energy / @as(f64, @floatFromInt(samples.len)));
}

/// Mixes `src * gain` into `dst` in place (`dst[i] += src[i] * gain`) using 8-way unrolled accumulation.
/// Processes `min(dst.len, src.len)` samples and returns the number of mixed samples.
pub fn mixAudioGainSimdF32(dst: *[]f32, src: []const f32, gain: f32): usize {
    const n: usize = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 8 <= n) : (i += 8) {
        dst[i + 0] += src[i + 0] * gain;
        dst[i + 1] += src[i + 1] * gain;
        dst[i + 2] += src[i + 2] * gain;
        dst[i + 3] += src[i + 3] * gain;
        dst[i + 4] += src[i + 4] * gain;
        dst[i + 5] += src[i + 5] * gain;
        dst[i + 6] += src[i + 6] * gain;
        dst[i + 7] += src[i + 7] * gain;
    }
    while (i < n) : (i += 1) {
        dst[i] += src[i] * gain;
    }
    return n;
}
