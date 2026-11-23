// Reference-hardened against ruuda/claxon (Apache-2.0)
// Origin: Pure-CP Stdlib (RFC 9639 Lossless FLAC Metadata & Audio Decoder + SIMD Kernels) | Created: 2026-11-22
// Features:
//   - Complete Metadata parsing: STREAMINFO, SEEKTABLE, VORBIS_COMMENT, PADDING/PICTURE/CUESHEET skip
//   - Complete Frame decoding: Sync (0x3FFE), UTF-8 frame/sample number, CRC-8 header & CRC-16 frame verification
//   - All 4 Subframe types: CONSTANT, VERBATIM, FIXED (0..4), LPC (1..32 with i64 accumulator) + RICE / RICE2 + wasted bits
//   - All 4 Stereo decorrelation modes: Independent, Left-Side, Right-Side, Mid-Side
//   - Hot-path 4-lane SIMD acceleration (`@Vector(4, i32)` stereo decorrelation & `@Vector(4, i64)` LPC inner product, 0-bit exact)

const std = @import("std");

pub const FLAC_MAGIC_SIZE: usize = 4;
pub const FLAC_BLOCK_HEADER_SIZE: usize = 4;
pub const FLAC_STREAMINFO_SIZE: usize = 34;
pub const FLAC_SEEKPOINT_SIZE: usize = 18;
pub const FLAC_MAX_CHANNELS: usize = 8;
pub const FLAC_MAX_LPC_ORDER: usize = 32;

/// Error set returned by RFC 9639 FLAC metadata, frame, and stream decoding operations.
pub err FlacError {
    InvalidMagic,
    InvalidMetadata,
    InvalidFrameSync,
    InvalidFrameHeader,
    InvalidUtf8Number,
    Crc8Mismatch,
    Crc16Mismatch,
    InvalidSubframeHeader,
    InvalidResidual,
    InvalidLpcOrder,
    UnexpectedEof,
    UnsupportedFormat,
    BufferTooSmall,
}

pub enum FlacBlockType {
    streaminfo,
    padding,
    application,
    seektable,
    vorbis_comment,
    cuesheet,
    picture,
    unknown,
}

pub struct FlacMetadataBlock {
    is_last: bool,
    block_type: FlacBlockType,
    raw_type: u8,
    payload: []const u8,
}

pub struct FlacStreamInfo {
    min_block_size: u16,
    max_block_size: u16,
    min_frame_size: u32,
    max_frame_size: u32,
    sample_rate: u32,
    channels: u8,
    bits_per_sample: u8,
    total_samples: u64,
    md5: [16]u8,

    fn durationMs(): u64 {
        const sr: u64 = _.sample_rate as u64;
        if (sr == 0) {
            return 0;
        }
        const secs: u64 = _.total_samples / sr;
        const rem: u64 = _.total_samples % sr;
        return secs * (1000 as u64) + (rem * (1000 as u64)) / sr;
    }
}

/// Decoded 18-byte `SEEKTABLE` entry.
pub struct FlacSeekPoint {
    sample_number: u64 = 0;
    stream_offset: u64 = 0;
    frame_samples: u16 = 0;
}

/// Decoded `VORBIS_COMMENT` metadata header view (zero-copy).
pub struct FlacVorbisCommentHeader {
    vendor: []const u8 = "";
    comment_count: u32 = 0;
    comments_data: []const u8 = "";
}

/// Decoded FLAC UTF-8 variable-length integer (1..7 bytes, up to 36 bits).
pub struct FlacUtf8Result {
    value: u64 = 0;
    bytes_read: usize = 0;
}

/// Channel assignment mode encoded in a FLAC frame header.
pub enum FlacChannelAssignment {
    independent,
    left_side,
    right_side,
    mid_side,
}

/// Decoded FLAC frame header metadata.
pub struct FlacFrameHeader {
    variable_block_size: bool = false;
    block_size: u32 = 0;
    sample_rate: u32 = 0;
    channels: u8 = 0;
    channel_assignment: FlacChannelAssignment = FlacChannelAssignment.independent;
    bits_per_sample: u8 = 0;
    frame_or_sample_number: u64 = 0;
    header_bytes: usize = 0;
}

/// Decoded single-frame telemetry returned by `decodeFlacFrame`.
pub struct FlacDecodedFrame {
    header: FlacFrameHeader = FlacFrameHeader{},
    bytes_consumed: usize = 0;
    samples_per_channel: usize = 0;
    channels: u8 = 0;
}

/// Full-stream decode summary returned by `decodeFlacStream`.
pub struct FlacDecodeSummary {
    stream_info: FlacStreamInfo = FlacStreamInfo{
        .min_block_size = 0,
        .max_block_size = 0,
        .min_frame_size = 0,
        .max_frame_size = 0,
        .sample_rate = 0,
        .channels = 0,
        .bits_per_sample = 0,
        .total_samples = 0,
        .md5 = [0; 16],
    },
    frames_decoded: usize = 0;
    total_samples_per_channel: usize = 0;
    total_interleaved_samples: usize = 0;
}

// ============================================================================
// 1. Low-Level Binary, CRC-8 (0x07), CRC-16 (0x8005) & UTF-8 Primitives
// ============================================================================

fn readU16BE(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readU24BE(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 16) |
        ((buf[off + 1] as u32) << 8) |
        (buf[off + 2] as u32);
}

fn readU32LE(buf: []const u8, off: usize): u32 {
    return (buf[off + 0] as u32) |
        ((buf[off + 1] as u32) << 8) |
        ((buf[off + 2] as u32) << 16) |
        ((buf[off + 3] as u32) << 24);
}

fn readU64BE(buf: []const u8, off: usize): u64 {
    return ((buf[off + 0] as u64) << 56) |
        ((buf[off + 1] as u64) << 48) |
        ((buf[off + 2] as u64) << 40) |
        ((buf[off + 3] as u64) << 32) |
        ((buf[off + 4] as u64) << 24) |
        ((buf[off + 5] as u64) << 16) |
        ((buf[off + 6] as u64) << 8) |
        (buf[off + 7] as u64);
}

/// Computes RFC 9639 FLAC Frame Header CRC-8 (polynomial `x^8 + x^2 + x + 1` = `0x07`, init `0`).
pub fn flacCrc8(data: []const u8): u8 {
    let crc: u8 = 0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= data[i];
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((crc & 0x80) != 0) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/// Computes RFC 9639 FLAC Frame Footer CRC-16 (polynomial `x^16 + x^15 + x^2 + 1` = `0x8005`, init `0`).
pub fn flacCrc16(data: []const u8): u16 {
    let crc: u16 = 0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= (data[i] as u16) << 8;
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((crc & 0x8000) != 0) {
                crc = (crc << 1) ^ 0x8005;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/// Decodes a FLAC UTF-8 variable-length integer (1..7 bytes, up to 36 bits) at `data[pos..]`.
pub fn decodeFlacUtf8(data: []const u8, pos: usize): FlacUtf8Result ! FlacError {
    if (pos >= data.len) return FlacError.UnexpectedEof;
    const first: u8 = data[pos];
    if ((first & 0x80) == 0) {
        return FlacUtf8Result{ value: first as u64, bytes_read: 1 };
    }
    if ((first & 0xC0) == 0x80 || first == 0xFF) {
        return FlacError.InvalidUtf8Number;
    }

    let leading_ones: usize = 0;
    let mask_mark: u8 = 0x80;
    let mask_data: u8 = 0x7F;
    while ((first & mask_mark) != 0) {
        leading_ones += 1;
        mask_mark >>= 1;
        mask_data >>= 1;
    }
    if (leading_ones < 2 || leading_ones > 7) return FlacError.InvalidUtf8Number;
    if (leading_ones > data.len - pos) return FlacError.UnexpectedEof;

    let val: u64 = (first & mask_data) as u64;
    let i: usize = 1;
    while (i < leading_ones) : (i += 1) {
        const cont: u8 = data[pos + i];
        if ((cont & 0xC0) != 0x80) return FlacError.InvalidUtf8Number;
        val = (val << 6) | ((cont & 0x3F) as u64);
    }
    return FlacUtf8Result{ value: val, bytes_read: leading_ones };
}

fn extendSignU32(val: u32, bits: u8): i32 {
    if (bits == 0) return 0;
    if (bits >= 32) return @bitCast(val);
    const shift: u5 = @intCast(32 - @as(u32, bits));
    const shifted_u: u32 = val << shift;
    const shifted_i: i32 = @bitCast(shifted_u);
    return shifted_i >> shift;
}

fn riceToSigned(val: u32): i32 {
    const half: i32 = @intCast(val >> 1);
    if ((val & 1) != 0) {
        return -1 - half;
    }
    return half;
}

// ============================================================================
// 2. Hot-Path 0-Bit Exact SIMD Kernels (Stereo Decorrelation & LPC Prediction)
// ============================================================================

/// Scalar reference Left-Side stereo decorrelation: `right[i] = left[i] -% side[i]`.
pub fn decorrelateLeftSideScalar(left: []const i32, side: *[]i32) {
    const n: usize = @min(left.len, side.len);
    let i: usize = 0;
    while (i < n) : (i += 1) {
        side[i] = left[i] -% side[i];
    }
}

/// 4-lane `@Vector(4, i32)` SIMD Left-Side stereo decorrelation (0-bit exact with scalar).
pub fn decorrelateLeftSideSimd(left: []const i32, side: *[]i32) {
    const n: usize = @min(left.len, side.len);
    let i: usize = 0;
    while (i + 4 <= n) {
        const vl: @Vector(4, i32) = left[i..i + 4][0..4].*;
        const vs: @Vector(4, i32) = side[i..i + 4][0..4].*;
        const vr: @Vector(4, i32) = vl -% vs;
        side[i..i + 4][0..4].* = vr;
        i += 4;
    }
    while (i < n) : (i += 1) {
        side[i] = left[i] -% side[i];
    }
}

/// Scalar reference Right-Side stereo decorrelation: `left[i] = side[i] +% right[i]`.
pub fn decorrelateRightSideScalar(side: *[]i32, right: []const i32) {
    const n: usize = @min(side.len, right.len);
    let i: usize = 0;
    while (i < n) : (i += 1) {
        side[i] = side[i] +% right[i];
    }
}

/// 4-lane `@Vector(4, i32)` SIMD Right-Side stereo decorrelation (0-bit exact with scalar).
pub fn decorrelateRightSideSimd(side: *[]i32, right: []const i32) {
    const n: usize = @min(side.len, right.len);
    let i: usize = 0;
    while (i + 4 <= n) {
        const vs: @Vector(4, i32) = side[i..i + 4][0..4].*;
        const vr: @Vector(4, i32) = right[i..i + 4][0..4].*;
        const vl: @Vector(4, i32) = vs +% vr;
        side[i..i + 4][0..4].* = vl;
        i += 4;
    }
    while (i < n) : (i += 1) {
        side[i] = side[i] +% right[i];
    }
}

/// Scalar reference Mid-Side stereo decorrelation (RFC 9639 odd-side exact rounding).
pub fn decorrelateMidSideScalar(mid: *[]i32, side: *[]i32) {
    const n: usize = @min(mid.len, side.len);
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const s: i32 = side[i];
        const m2: i32 = (mid[i] *% 2) | (s & 1);
        mid[i] = (m2 +% s) >> 1;
        side[i] = (m2 -% s) >> 1;
    }
}

/// 4-lane `@Vector(4, i32)` SIMD Mid-Side stereo decorrelation (0-bit exact with scalar).
pub fn decorrelateMidSideSimd(mid: *[]i32, side: *[]i32) {
    const n: usize = @min(mid.len, side.len);
    const v1: @Vector(4, i32) = @splat(1);
    const v2: @Vector(4, i32) = @splat(2);
    const vsh1: @Vector(4, u5) = @splat(1);
    let i: usize = 0;
    while (i + 4 <= n) {
        const vm: @Vector(4, i32) = mid[i..i + 4][0..4].*;
        const vs: @Vector(4, i32) = side[i..i + 4][0..4].*;
        const m2: @Vector(4, i32) = (vm *% v2) | (vs & v1);
        const vl: @Vector(4, i32) = (m2 +% vs) >> vsh1;
        const vr: @Vector(4, i32) = (m2 -% vs) >> vsh1;
        mid[i..i + 4][0..4].* = vl;
        side[i..i + 4][0..4].* = vr;
        i += 4;
    }
    while (i < n) : (i += 1) {
        const s: i32 = side[i];
        const m2: i32 = (mid[i] *% 2) | (s & 1);
        mid[i] = (m2 +% s) >> 1;
        side[i] = (m2 -% s) >> 1;
    }
}

/// Scalar reference LPC linear predictor restoration (`i64` accumulator, 0-bit exact).
/// `coeffs` holds the `order` LPC coefficients in FLAC order (`coeffs[0]` multiplies `samples[i - 1]`).
pub fn lpcPredictScalar(
    samples: *[]i32,
    coeffs: []const i32,
    shift: u6,
) {
    const order: usize = coeffs.len;
    if (order == 0 || samples.len <= order) return;
    let i: usize = order;
    while (i < samples.len) : (i += 1) {
        let sum: i64 = 0;
        let k: usize = 0;
        while (k < order) : (k += 1) {
            sum +%= @as(i64, coeffs[k]) *% @as(i64, samples[i - 1 - k]);
        }
        const pred: i32 = @truncate(sum >> shift);
        samples[i] = samples[i] +% pred;
    }
}

/// 4-lane `@Vector(4, i64)` SIMD LPC predictor restoration (0-bit exact with `lpcPredictScalar`).
/// Reverses coefficients once into a stack array so history `samples[i - order .. i]` is loaded contiguously.
pub fn lpcPredictSimd(
    samples: *[]i32,
    coeffs: []const i32,
    shift: u6,
) {
    const order: usize = coeffs.len;
    if (order == 0 || order > FLAC_MAX_LPC_ORDER || samples.len <= order) return;

    let rev_c: [FLAC_MAX_LPC_ORDER]i64 = [0; FLAC_MAX_LPC_ORDER];
    let r: usize = 0;
    while (r < order) : (r += 1) {
        rev_c[r] = @as(i64, coeffs[order - 1 - r]);
    }

    let i: usize = order;
    while (i < samples.len) : (i += 1) {
        const base: usize = i - order;
        let vacc: @Vector(4, i64) = @splat(0);
        let k: usize = 0;
        while (k + 4 <= order) {
            const vc: @Vector(4, i64) = rev_c[k..k + 4][0..4].*;
            const vs64: @Vector(4, i64) = [
                @as(i64, samples[base + k + 0]),
                @as(i64, samples[base + k + 1]),
                @as(i64, samples[base + k + 2]),
                @as(i64, samples[base + k + 3]),
            ];
            vacc +%= vc *% vs64;
            k += 4;
        }
        let sum: i64 = @reduce(.Add, vacc);
        while (k < order) : (k += 1) {
            sum +%= rev_c[k] *% @as(i64, samples[base + k]);
        }
        const pred: i32 = @truncate(sum >> shift);
        samples[i] = samples[i] +% pred;
    }
}

fn predictFixedInPlace(order: u8, samples: *[]i32) {
    const ord: usize = @intCast(order);
    if (ord == 0 || samples.len <= ord) return;
    let i: usize = ord;
    if (ord == 1) {
        while (i < samples.len) : (i += 1) {
            samples[i] = samples[i] +% samples[i - 1];
        }
    } else if (ord == 2) {
        while (i < samples.len) : (i += 1) {
            const p: i32 = (2 *% samples[i - 1]) -% samples[i - 2];
            samples[i] = samples[i] +% p;
        }
    } else if (ord == 3) {
        while (i < samples.len) : (i += 1) {
            const p: i32 = (3 *% samples[i - 1]) -% (3 *% samples[i - 2]) +% samples[i - 3];
            samples[i] = samples[i] +% p;
        }
    } else if (ord == 4) {
        while (i < samples.len) : (i += 1) {
            const p: i32 = (4 *% samples[i - 1]) -% (6 *% samples[i - 2]) +% (4 *% samples[i - 3]) -% samples[i - 4];
            samples[i] = samples[i] +% p;
        }
    }
}

// ============================================================================
// 3. Metadata Block Iterator & Parsers (100% Backward Compatible)
// ============================================================================

fn classifyBlockType(raw_type: u8): FlacBlockType {
    if (raw_type == 0) return FlacBlockType.streaminfo;
    if (raw_type == 1) return FlacBlockType.padding;
    if (raw_type == 2) return FlacBlockType.application;
    if (raw_type == 3) return FlacBlockType.seektable;
    if (raw_type == 4) return FlacBlockType.vorbis_comment;
    if (raw_type == 5) return FlacBlockType.cuesheet;
    if (raw_type == 6) return FlacBlockType.picture;
    return FlacBlockType.unknown;
}

pub fn isFlac(data: []const u8): bool {
    if (data.len < FLAC_MAGIC_SIZE) {
        return false;
    }
    return data[0] == 'f' && data[1] == 'L' && data[2] == 'a' && data[3] == 'C';
}

pub struct FlacBlockIter {
    data: []const u8,
    pos: usize,
    done: bool,

    fn init(data: []const u8): FlacBlockIter {
        if (!isFlac(data)) {
            return FlacBlockIter{
                .data = data[0..0],
                .pos = 0,
                .done = true,
            };
        }
        return FlacBlockIter{
            .data = data,
            .pos = FLAC_MAGIC_SIZE,
            .done = false,
        };
    }

    fn next(*_): FlacMetadataBlock? {
        if (_.done || _.pos > _.data.len || _.data.len - _.pos < FLAC_BLOCK_HEADER_SIZE) {
            _.done = true;
            return null;
        }

        const hdr0 = _.data[_.pos];
        const is_last = (hdr0 & 0x80) != 0;
        const raw_type = hdr0 & 0x7F;
        // RFC 9639: block type 127 is invalid/forbidden
        if (raw_type == 127) {
            _.done = true;
            return null;
        }

        const payload_len: usize = readU24BE(_.data, _.pos + 1) as usize;
        const payload_start = _.pos + FLAC_BLOCK_HEADER_SIZE;
        if (payload_len > _.data.len - payload_start) {
            _.done = true;
            return null;
        }
        const payload_end = payload_start + payload_len;

        _.pos = payload_end;
        if (is_last) {
            _.done = true;
        }

        return FlacMetadataBlock{
            .is_last = is_last,
            .block_type = classifyBlockType(raw_type),
            .raw_type = raw_type,
            .payload = _.data[payload_start..payload_end],
        };
    }
}

pub fn parseStreamInfoPayload(payload: []const u8): FlacStreamInfo? {
    if (payload.len != FLAC_STREAMINFO_SIZE) {
        return null;
    }

    const min_block_size = readU16BE(payload, 0);
    const max_block_size = readU16BE(payload, 2);
    const min_frame_size = readU24BE(payload, 4);
    const max_frame_size = readU24BE(payload, 7);

    const b10 = payload[10];
    const b11 = payload[11];
    const b12 = payload[12];
    const b13 = payload[13];
    const b14 = payload[14];
    const b15 = payload[15];
    const b16 = payload[16];
    const b17 = payload[17];

    const sample_rate: u32 = ((b10 as u32) << 12) |
        ((b11 as u32) << 4) |
        (((b12 >> 4) & 0x0F) as u32);
    const channels: u8 = (((b12 >> 1) & 0x07) + 1) as u8;
    const bits_per_sample: u8 = ((((b12 & 0x01) << 4) | ((b13 >> 4) & 0x0F)) + 1) as u8;
    const total_samples: u64 = (((b13 & 0x0F) as u64) << 32) |
        ((b14 as u64) << 24) |
        ((b15 as u64) << 16) |
        ((b16 as u64) << 8) |
        (b17 as u64);

    let md5: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        md5[i] = payload[18 + i];
    }

    return FlacStreamInfo{
        .min_block_size = min_block_size,
        .max_block_size = max_block_size,
        .min_frame_size = min_frame_size,
        .max_frame_size = max_frame_size,
        .sample_rate = sample_rate,
        .channels = channels,
        .bits_per_sample = bits_per_sample,
        .total_samples = total_samples,
        .md5 = md5,
    };
}

pub fn parseFlacStreamInfo(data: []const u8): FlacStreamInfo? {
    let it = FlacBlockIter.init(data);
    const first = it.next() ?? return null;
    if (first.block_type != FlacBlockType.streaminfo) {
        return null;
    }
    return parseStreamInfoPayload(first.payload);
}

/// Parses a `SEEKTABLE` metadata payload into caller-owned `out_points` (18 bytes per entry).
pub fn parseSeekTable(payload: []const u8, out_points: *[]FlacSeekPoint): usize ! FlacError {
    if ((payload.len % FLAC_SEEKPOINT_SIZE) != 0) return FlacError.InvalidMetadata;
    const total: usize = payload.len / FLAC_SEEKPOINT_SIZE;
    let written: usize = 0;
    let i: usize = 0;
    while (i < total && written < out_points.len) : (i += 1) {
        const off: usize = i * FLAC_SEEKPOINT_SIZE;
        out_points[written] = FlacSeekPoint{
            sample_number: readU64BE(payload, off),
            stream_offset: readU64BE(payload, off + 8),
            frame_samples: readU16BE(payload, off + 16),
        };
        written += 1;
    }
    return written;
}

/// Parses a `VORBIS_COMMENT` metadata payload with overflow-safe bounds checks.
pub fn parseVorbisComment(payload: []const u8): FlacVorbisCommentHeader ! FlacError {
    if (payload.len < 8) return FlacError.InvalidMetadata;
    const vendor_len: usize = readU32LE(payload, 0) as usize;
    if (vendor_len > payload.len - 8) return FlacError.InvalidMetadata;
    const vendor: []const u8 = payload[4..vendor_len + 4];
    const count_off: usize = vendor_len + 4;
    const comment_count: u32 = readU32LE(payload, count_off);
    return FlacVorbisCommentHeader{
        vendor: vendor,
        comment_count: comment_count,
        comments_data: payload[count_off + 4..payload.len],
    };
}

/// Validates all metadata blocks starting from `fLaC` magic and returns the byte offset
/// of the first audio frame.
pub fn findAudioDataOffset(data: []const u8): usize ! FlacError {
    if (!isFlac(data)) return FlacError.InvalidMagic;
    let pos: usize = FLAC_MAGIC_SIZE;
    let saw_streaminfo: bool = false;
    while (true) {
        if (pos > data.len || data.len - pos < FLAC_BLOCK_HEADER_SIZE) {
            return FlacError.UnexpectedEof;
        }
        const hdr0: u8 = data[pos];
        const is_last: bool = (hdr0 & 0x80) != 0;
        const raw_type: u8 = hdr0 & 0x7F;
        if (raw_type == 127) return FlacError.InvalidMetadata;
        const plen: usize = readU24BE(data, pos + 1) as usize;
        const pstart: usize = pos + FLAC_BLOCK_HEADER_SIZE;
        if (plen > data.len - pstart) return FlacError.UnexpectedEof;
        if (!saw_streaminfo) {
            if (raw_type != 0 || plen != FLAC_STREAMINFO_SIZE) return FlacError.InvalidMetadata;
            saw_streaminfo = true;
        }
        pos = pstart + plen;
        if (is_last) return pos;
    }
}

// ============================================================================
// 4. Bitstream Reader, Frame Header Decoder & Subframe Decoders
// ============================================================================

struct FlacBitReader {
    data: []const u8 = "",
    byte_pos: usize = 0;
    bit_pos: u8 = 0;

    static fn init(data: []const u8, start_byte: usize): FlacBitReader {
        return FlacBitReader{
            data: data,
            byte_pos: start_byte,
            bit_pos: 0,
        };
    }

    fn readBit(*_): u8 ! FlacError {
        if (_.byte_pos >= _.data.len) return FlacError.UnexpectedEof;
        const shift: u3 = @intCast(7 - @as(u8, _.bit_pos));
        const bit: u8 = (_.data[_.byte_pos] >> shift) & 1;
        _.bit_pos += 1;
        if (_.bit_pos == 8) {
            _.bit_pos = 0;
            _.byte_pos += 1;
        }
        return bit;
    }

    fn readBits(*_, n_in: u8): u32 ! FlacError {
        if (n_in == 0) return 0;
        if (n_in > 32) return FlacError.InvalidResidual;
        let rem: u8 = n_in;
        let acc: u32 = 0;
        while (rem > 0) {
            if (_.byte_pos >= _.data.len) return FlacError.UnexpectedEof;
            const eight_u8: u8 = 8;
            const avail: u8 = eight_u8 - _.bit_pos;
            const take: u8 = if (rem < avail) rem else avail;
            const shift_r: u3 = @intCast(avail - take);
            const mask: u8 = if (take == 8) 0xFF else ((@as(u8, 1) << @as(u3, @intCast(take))) - 1);
            const chunk: u32 = @as(u32, (_.data[_.byte_pos] >> shift_r) & mask);
            if (take == 32) {
                acc = chunk;
            } else {
                acc = (acc << @as(u5, @intCast(take))) | chunk;
            }
            _.bit_pos += take;
            if (_.bit_pos == 8) {
                _.bit_pos = 0;
                _.byte_pos += 1;
            }
            rem -= take;
        }
        return acc;
    }

    fn readSignedBits(*_, n: u8): i32 ! FlacError {
        const raw: u32 = _.readBits(n)?;
        return extendSignU32(raw, n);
    }

    fn readUnary(*_): u32 ! FlacError {
        let count: u32 = 0;
        while (true) {
            if (_.byte_pos >= _.data.len) return FlacError.UnexpectedEof;
            const b: u8 = _.data[_.byte_pos];
            // Fast skip full zero bytes when byte-aligned
            if (_.bit_pos == 0 && b == 0) {
                count += 8;
                _.byte_pos += 1;
                if (count > 65536) return FlacError.InvalidResidual;
                continue;
            }
            const bit: u8 = _.readBit()?;
            if (bit == 1) return count;
            count += 1;
            if (count > 65536) return FlacError.InvalidResidual;
        }
    }

    fn alignToByte(*_): void ! FlacError {
        if (_.bit_pos == 0) return;
        const eight_u8: u8 = 8;
        const pad_bits: u8 = eight_u8 - _.bit_pos;
        const pad: u32 = _.readBits(pad_bits)?;
        if (pad != 0) return FlacError.InvalidResidual;
    }
}

/// Decodes and CRC-8-verifies a FLAC frame header at `data[offset..]`.
pub fn decodeFrameHeader(
    data: []const u8,
    offset: usize,
    stream_info: FlacStreamInfo,
): FlacFrameHeader ! FlacError {
    if (offset > data.len || data.len - offset < 6) return FlacError.UnexpectedEof;

    const sync_word: u16 = readU16BE(data, offset);
    if ((sync_word & 0xFFFC) != 0xFFF8) return FlacError.InvalidFrameSync;
    if ((sync_word & 0x0002) != 0) return FlacError.InvalidFrameHeader;
    const variable_bs: bool = (sync_word & 0x0001) != 0;

    const bs_sr: u8 = data[offset + 2];
    const bs_code: u8 = (bs_sr >> 4) & 0x0F;
    const sr_code: u8 = bs_sr & 0x0F;

    let block_size: u32 = 0;
    let read_bs_8: bool = false;
    let read_bs_16: bool = false;
    if (bs_code == 0) {
        return FlacError.InvalidFrameHeader;
    } else if (bs_code == 1) {
        block_size = 192;
    } else if (bs_code >= 2 && bs_code <= 5) {
        block_size = @as(u32, 576) << @as(u5, @intCast(bs_code - 2));
    } else if (bs_code == 6) {
        read_bs_8 = true;
    } else if (bs_code == 7) {
        read_bs_16 = true;
    } else {
        block_size = @as(u32, 256) << @as(u5, @intCast(bs_code - 8));
    }

    let sample_rate: u32 = 0;
    let read_sr_8k: bool = false;
    let read_sr_16: bool = false;
    let read_sr_16d: bool = false;
    if (sr_code == 0) {
        sample_rate = stream_info.sample_rate;
    } else if (sr_code == 1) {
        sample_rate = 88200;
    } else if (sr_code == 2) {
        sample_rate = 176400;
    } else if (sr_code == 3) {
        sample_rate = 192000;
    } else if (sr_code == 4) {
        sample_rate = 8000;
    } else if (sr_code == 5) {
        sample_rate = 16000;
    } else if (sr_code == 6) {
        sample_rate = 22050;
    } else if (sr_code == 7) {
        sample_rate = 24000;
    } else if (sr_code == 8) {
        sample_rate = 32000;
    } else if (sr_code == 9) {
        sample_rate = 44100;
    } else if (sr_code == 10) {
        sample_rate = 48000;
    } else if (sr_code == 11) {
        sample_rate = 96000;
    } else if (sr_code == 12) {
        read_sr_8k = true;
    } else if (sr_code == 13) {
        read_sr_16 = true;
    } else if (sr_code == 14) {
        read_sr_16d = true;
    } else {
        return FlacError.InvalidFrameHeader;
    }

    const ch_bps: u8 = data[offset + 3];
    if ((ch_bps & 0x01) != 0) return FlacError.InvalidFrameHeader;
    const ch_code: u8 = (ch_bps >> 4) & 0x0F;
    const bps_code: u8 = (ch_bps >> 1) & 0x07;

    let channels: u8 = 0;
    let ch_assign: FlacChannelAssignment = FlacChannelAssignment.independent;
    if (ch_code <= 7) {
        channels = ch_code + 1;
        ch_assign = FlacChannelAssignment.independent;
    } else if (ch_code == 8) {
        channels = 2;
        ch_assign = FlacChannelAssignment.left_side;
    } else if (ch_code == 9) {
        channels = 2;
        ch_assign = FlacChannelAssignment.right_side;
    } else if (ch_code == 10) {
        channels = 2;
        ch_assign = FlacChannelAssignment.mid_side;
    } else {
        return FlacError.InvalidFrameHeader;
    }

    let bps: u8 = 0;
    if (bps_code == 0) {
        bps = stream_info.bits_per_sample;
    } else if (bps_code == 1) {
        bps = 8;
    } else if (bps_code == 2) {
        bps = 12;
    } else if (bps_code == 3) {
        return FlacError.InvalidFrameHeader;
    } else if (bps_code == 4) {
        bps = 16;
    } else if (bps_code == 5) {
        bps = 20;
    } else if (bps_code == 6) {
        bps = 24;
    } else {
        bps = 32;
    }
    if (bps < 4 || bps > 32) return FlacError.InvalidFrameHeader;

    let pos: usize = offset + 4;
    const utf8_res: FlacUtf8Result = decodeFlacUtf8(data, pos)?;
    pos += utf8_res.bytes_read;

    if (read_bs_8) {
        if (pos >= data.len) return FlacError.UnexpectedEof;
        block_size = @as(u32, data[pos]) + 1;
        pos += 1;
    } else if (read_bs_16) {
        if (data.len - pos < 2) return FlacError.UnexpectedEof;
        block_size = @as(u32, readU16BE(data, pos)) + 1;
        pos += 2;
    }

    if (read_sr_8k) {
        if (pos >= data.len) return FlacError.UnexpectedEof;
        sample_rate = @as(u32, data[pos]) * 1000;
        pos += 1;
    } else if (read_sr_16) {
        if (data.len - pos < 2) return FlacError.UnexpectedEof;
        sample_rate = @as(u32, readU16BE(data, pos));
        pos += 2;
    } else if (read_sr_16d) {
        if (data.len - pos < 2) return FlacError.UnexpectedEof;
        sample_rate = @as(u32, readU16BE(data, pos)) * 10;
        pos += 2;
    }

    if (pos >= data.len) return FlacError.UnexpectedEof;
    const stored_crc8: u8 = data[pos];
    const calc_crc8: u8 = flacCrc8(data[offset..pos]);
    if (stored_crc8 != calc_crc8) return FlacError.Crc8Mismatch;
    pos += 1;

    return FlacFrameHeader{
        variable_block_size: variable_bs,
        block_size: block_size,
        sample_rate: sample_rate,
        channels: channels,
        channel_assignment: ch_assign,
        bits_per_sample: bps,
        frame_or_sample_number: utf8_res.value,
        header_bytes: pos - offset,
    };
}

fn decodeResidualPartitions(
    br: *FlacBitReader,
    block_size: usize,
    predictor_order: usize,
    out_buf: *[]i32,
): void ! FlacError {
    const method: u32 = br.readBits(2)?;
    if (method > 1) return FlacError.InvalidResidual;
    const is_rice2: bool = method == 1;
    const param_bits: u8 = if (is_rice2) 5 else 4;
    const escape_code: u32 = if (is_rice2) 31 else 15;

    const partition_order: u32 = br.readBits(4)?;
    if (partition_order > 15) return FlacError.InvalidResidual;
    const num_partitions: usize = @as(usize, 1) << @as(u5, @intCast(partition_order));
    if (num_partitions == 0 || (block_size % num_partitions) != 0) {
        return FlacError.InvalidResidual;
    }
    const samples_per_part: usize = block_size / num_partitions;
    if (predictor_order > samples_per_part) return FlacError.InvalidResidual;

    let sample_idx: usize = predictor_order;
    let p: usize = 0;
    while (p < num_partitions) : (p += 1) {
        const count: usize = if (p == 0) (samples_per_part - predictor_order) else samples_per_part;
        const rice_param: u32 = br.readBits(param_bits)?;
        if (rice_param == escape_code) {
            const raw_bits: u8 = @intCast(br.readBits(5)?);
            if (raw_bits > 32) return FlacError.InvalidResidual;
            let k: usize = 0;
            while (k < count) : (k += 1) {
                out_buf[sample_idx] = if (raw_bits == 0) 0 else br.readSignedBits(raw_bits)?;
                sample_idx += 1;
            }
        } else {
            const rp: u8 = @intCast(rice_param);
            let k: usize = 0;
            while (k < count) : (k += 1) {
                const q: u32 = br.readUnary()?;
                const r: u32 = br.readBits(rp)?;
                const shift_rp: u5 = @intCast(rp);
                const max_u32: u32 = 0xFFFF_FFFF;
                if (rp > 0 && q > (max_u32 >> shift_rp)) return FlacError.InvalidResidual;
                const folded: u32 = (q << shift_rp) | r;
                out_buf[sample_idx] = riceToSigned(folded);
                sample_idx += 1;
            }
        }
    }
}

fn decodeSubframe(
    br: *FlacBitReader,
    block_size: usize,
    raw_bps: u8,
    out_samples: *[]i32,
): void ! FlacError {
    if (out_samples.len < block_size) return FlacError.BufferTooSmall;
    const pad_bit: u8 = br.readBit()?;
    if (pad_bit != 0) return FlacError.InvalidSubframeHeader;

    const sf_code: u32 = br.readBits(6)?;
    const has_wasted: u8 = br.readBit()?;
    let wasted_bits: u8 = 0;
    if (has_wasted == 1) {
        const w: u32 = br.readUnary()? + 1;
        if (w > 31 || w >= @as(u32, raw_bps)) return FlacError.InvalidSubframeHeader;
        wasted_bits = @intCast(w);
    }
    const eff_bps: u8 = raw_bps - wasted_bits;
    if (eff_bps == 0 || eff_bps > 32) return FlacError.InvalidSubframeHeader;

    if (sf_code == 0) {
        // CONSTANT
        const val: i32 = br.readSignedBits(eff_bps)?;
        let i: usize = 0;
        while (i < block_size) : (i += 1) out_samples[i] = val;
    } else if (sf_code == 1) {
        // VERBATIM
        let i: usize = 0;
        while (i < block_size) : (i += 1) {
            out_samples[i] = br.readSignedBits(eff_bps)?;
        }
    } else if (sf_code >= 8 && sf_code <= 12) {
        // FIXED (order 0..4)
        const order: u8 = @intCast(sf_code - 8);
        const ord: usize = @intCast(order);
        if (ord > block_size) return FlacError.InvalidSubframeHeader;
        let i: usize = 0;
        while (i < ord) : (i += 1) {
            out_samples[i] = br.readSignedBits(eff_bps)?;
        }
        decodeResidualPartitions(br, block_size, ord, out_samples)?;
        let sub_slice: []i32 = out_samples[0..block_size];
        predictFixedInPlace(order, sub_slice);
    } else if (sf_code >= 32 && sf_code <= 63) {
        // LPC (order 1..32)
        const order: usize = @intCast((sf_code - 32) + 1);
        if (order > block_size || order > FLAC_MAX_LPC_ORDER) return FlacError.InvalidLpcOrder;
        let i: usize = 0;
        while (i < order) : (i += 1) {
            out_samples[i] = br.readSignedBits(eff_bps)?;
        }
        const prec_raw: u32 = br.readBits(4)?;
        if (prec_raw == 15) return FlacError.InvalidSubframeHeader;
        const coeff_prec: u8 = @intCast(prec_raw + 1);
        const lpc_shift_signed: i32 = br.readSignedBits(5)?;
        if (lpc_shift_signed < 0 || lpc_shift_signed > 31) return FlacError.InvalidSubframeHeader;
        const lpc_shift: u6 = @intCast(lpc_shift_signed);

        let coeffs: [FLAC_MAX_LPC_ORDER]i32 = [0; FLAC_MAX_LPC_ORDER];
        let c: usize = 0;
        while (c < order) : (c += 1) {
            coeffs[c] = br.readSignedBits(coeff_prec)?;
        }
        decodeResidualPartitions(br, block_size, order, out_samples)?;
        let sub_slice: []i32 = out_samples[0..block_size];
        lpcPredictSimd(sub_slice, coeffs[0..order], lpc_shift);
    } else {
        return FlacError.InvalidSubframeHeader;
    }

    if (wasted_bits > 0) {
        const wsh: u5 = @intCast(wasted_bits);
        let i: usize = 0;
        while (i < block_size) : (i += 1) {
            out_samples[i] = out_samples[i] << wsh;
        }
    }
}

/// Decodes a single complete FLAC audio frame at `data[offset..]`, verifying both
/// header CRC-8 and trailing frame CRC-16, applying SIMD stereo decorrelation,
/// and writing interleaved PCM samples into `out_interleaved`.
/// `channel_scratch` must have length `>= header.channels * header.block_size`.
pub fn decodeFlacFrame(
    data: []const u8,
    offset: usize,
    stream_info: FlacStreamInfo,
    channel_scratch: *[]i32,
    out_interleaved: *[]i32,
): FlacDecodedFrame ! FlacError {
    const hdr: FlacFrameHeader = decodeFrameHeader(data, offset, stream_info)?;
    const bs: usize = @intCast(hdr.block_size);
    const ch: usize = @intCast(hdr.channels);
    const total_frame_samples: usize = bs * ch;
    if (channel_scratch.len < total_frame_samples || out_interleaved.len < total_frame_samples) {
        return FlacError.BufferTooSmall;
    }

    let br: FlacBitReader = FlacBitReader.init(data, offset + hdr.header_bytes);
    let c: usize = 0;
    while (c < ch) : (c += 1) {
        let sub_bps: u8 = hdr.bits_per_sample;
        if (hdr.channel_assignment == FlacChannelAssignment.left_side && c == 1) {
            sub_bps += 1;
        } else if (hdr.channel_assignment == FlacChannelAssignment.right_side && c == 0) {
            sub_bps += 1;
        } else if (hdr.channel_assignment == FlacChannelAssignment.mid_side && c == 1) {
            sub_bps += 1;
        }
        if (sub_bps > 32) return FlacError.UnsupportedFormat;
        let ch_buf: []i32 = channel_scratch[c * bs..(c + 1) * bs];
        decodeSubframe(&br, bs, sub_bps, ch_buf)?;
    }

    br.alignToByte()?;
    if (br.byte_pos > data.len || data.len - br.byte_pos < 2) return FlacError.UnexpectedEof;

    const stored_crc16: u16 = readU16BE(data, br.byte_pos);
    const calc_crc16: u16 = flacCrc16(data[offset..br.byte_pos]);
    if (stored_crc16 != calc_crc16) return FlacError.Crc16Mismatch;
    const frame_end: usize = br.byte_pos + 2;

    // Apply SIMD stereo channel decorrelation
    if (hdr.channel_assignment == FlacChannelAssignment.left_side) {
        let side_s: []i32 = channel_scratch[bs..2 * bs];
        decorrelateLeftSideSimd(channel_scratch[0..bs], side_s);
    } else if (hdr.channel_assignment == FlacChannelAssignment.right_side) {
        let side_s: []i32 = channel_scratch[0..bs];
        decorrelateRightSideSimd(side_s, channel_scratch[bs..2 * bs]);
    } else if (hdr.channel_assignment == FlacChannelAssignment.mid_side) {
        let mid_s: []i32 = channel_scratch[0..bs];
        let side_s: []i32 = channel_scratch[bs..2 * bs];
        decorrelateMidSideSimd(mid_s, side_s);
    }

    // Interleave decoded channels into `out_interleaved`
    if (ch == 1) {
        let i: usize = 0;
        while (i < bs) : (i += 1) {
            out_interleaved[i] = channel_scratch[i];
        }
    } else if (ch == 2) {
        let i: usize = 0;
        while (i < bs) : (i += 1) {
            out_interleaved[i * 2 + 0] = channel_scratch[i];
            out_interleaved[i * 2 + 1] = channel_scratch[bs + i];
        }
    } else {
        let i: usize = 0;
        while (i < bs) : (i += 1) {
            let ci: usize = 0;
            while (ci < ch) : (ci += 1) {
                out_interleaved[i * ch + ci] = channel_scratch[ci * bs + i];
            }
        }
    }

    return FlacDecodedFrame{
        header: hdr,
        bytes_consumed: frame_end - offset,
        samples_per_channel: bs,
        channels: hdr.channels,
    };
}

/// Decodes a complete FLAC stream (`fLaC` magic + metadata blocks + all audio frames)
/// into caller-owned `out_interleaved` with zero dynamic heap allocation.
pub fn decodeFlacStream(
    data: []const u8,
    channel_scratch: *[]i32,
    out_interleaved: *[]i32,
): FlacDecodeSummary ! FlacError {
    const si_opt: FlacStreamInfo? = parseFlacStreamInfo(data);
    if (si_opt == null) return FlacError.InvalidMetadata;
    const si: FlacStreamInfo = si_opt ?? return FlacError.InvalidMetadata;

    let pos: usize = findAudioDataOffset(data)?;
    let frames: usize = 0;
    let total_spc: usize = 0;
    let total_interleaved: usize = 0;

    while (pos < data.len) {
        let out_tail: []i32 = out_interleaved[total_interleaved..out_interleaved.len];
        const fr: FlacDecodedFrame = decodeFlacFrame(data, pos, si, channel_scratch, out_tail)?;
        pos += fr.bytes_consumed;
        frames += 1;
        total_spc += fr.samples_per_channel;
        total_interleaved += fr.samples_per_channel * @as(usize, fr.channels);
    }

    return FlacDecodeSummary{
        stream_info: si,
        frames_decoded: frames,
        total_samples_per_channel: total_spc,
        total_interleaved_samples: total_interleaved,
    };
}
