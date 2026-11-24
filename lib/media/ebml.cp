// Reference-hardened against RFC 8794 / RFC 9559 & ebml-iterable (MIT)
// Origin: Pure-CP Stdlib (Zero-Alloc RFC 8794 EBML Parser, 16-Lane SIMD Primitives & Strict Validator) | Created: 2026-11-22

import std from "std";

type EbmlVec16u8 = @Vector(16, u8);

pub const EBML_ID_HEADER: u32 = 0x1A45DFA3;
pub const EBML_ID_VERSION: u32 = 0x4286;
pub const EBML_ID_DOCTYPE: u32 = 0x4282;
pub const EBML_ID_SEGMENT: u32 = 0x18538067;

pub const EBML_MAX_DEPTH: usize = 64;
pub const EBML_MAX_ELEMENTS: usize = 65536;

pub err EbmlError {
    TruncatedInput,
    InvalidVint,
    InvalidElementId,
    PayloadOutOfBounds,
    MaxDepthExceeded,
    MaxElementsExceeded,
    InvalidIntegerLen,
    InvalidFloatLen,
    UnknownSizeNotAllowed,
}

pub struct EbmlVint {
    value: u64,
    raw_id: u32,
    bytes_read: u8,
    is_unknown_size: bool,
}

pub struct EbmlElement {
    id: u32,
    size: u64,
    is_unknown_size: bool,
    payload: []const u8,
}

// ============================================================================
// 16-Lane @Vector(16, u8) SIMD Primitives + Scalar Differential References
// ============================================================================

fn ebmlLoadVec16(data: []const u8, off: usize): EbmlVec16u8 {
    const v: EbmlVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD payload copy (`min(dst.len, src.len)` bytes).
pub fn simdEbmlCopyPayload(dst: []u8, src: []const u8): usize {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v = ebmlLoadVec16(src, i);
        dst[i] = v[0];
        dst[i + 1] = v[1];
        dst[i + 2] = v[2];
        dst[i + 3] = v[3];
        dst[i + 4] = v[4];
        dst[i + 5] = v[5];
        dst[i + 6] = v[6];
        dst[i + 7] = v[7];
        dst[i + 8] = v[8];
        dst[i + 9] = v[9];
        dst[i + 10] = v[10];
        dst[i + 11] = v[11];
        dst[i + 12] = v[12];
        dst[i + 13] = v[13];
        dst[i + 14] = v[14];
        dst[i + 15] = v[15];
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

/// Scalar reference for `simdEbmlCopyPayload` (differential verification).
pub fn scalarEbmlCopyPayload(dst: []u8, src: []const u8): usize {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

/// 16-lane `@Vector(16, u8)` SIMD byte slice equality check for DocType / CodecID / magic matching.
pub fn simdEbmlBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va = ebmlLoadVec16(a, i);
        const vb = ebmlLoadVec16(b, i);
        if (!@reduce(.And, va == vb)) return false;
        i += 16;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Scalar reference for `simdEbmlBytesEqual` (differential verification).
pub fn scalarEbmlBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD resync scanner locating the next EBML Header magic (`1A 45 DF A3`)
/// at or after `start`.
pub fn simdFindEbmlHeaderMagic(data: []const u8, start: usize): usize? {
    if (data.len < 4 || start + 4 > data.len) return null;
    const limit = data.len - 3;
    const m0_v: EbmlVec16u8 = @splat(0x1A);
    let i: usize = start;
    while (i + 16 <= limit) {
        const v = ebmlLoadVec16(data, i);
        if (@reduce(.Or, v == m0_v)) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                const p = i + j;
                if (data[p] == 0x1A && data[p + 1] == 0x45 && data[p + 2] == 0xDF && data[p + 3] == 0xA3) {
                    return p;
                }
            }
        }
        i += 16;
    }
    while (i < limit) : (i += 1) {
        if (data[i] == 0x1A && data[i + 1] == 0x45 && data[i + 2] == 0xDF && data[i + 3] == 0xA3) {
            return i;
        }
    }
    return null;
}

/// Scalar reference for `simdFindEbmlHeaderMagic` (differential verification).
pub fn scalarFindEbmlHeaderMagic(data: []const u8, start: usize): usize? {
    if (data.len < 4 || start + 4 > data.len) return null;
    const limit = data.len - 3;
    let i: usize = start;
    while (i < limit) : (i += 1) {
        if (data[i] == 0x1A && data[i + 1] == 0x45 && data[i + 2] == 0xDF && data[i + 3] == 0xA3) {
            return i;
        }
    }
    return null;
}

// ============================================================================
// RFC 8794 VINT, Element ID, Integer & IEEE 754 Float Decoders
// ============================================================================

fn vintWidth(first_byte: u8): u8 {
    if (first_byte == 0) return 0;
    if ((first_byte & 0x80) != 0) return 1;
    if ((first_byte & 0x40) != 0) return 2;
    if ((first_byte & 0x20) != 0) return 3;
    if ((first_byte & 0x10) != 0) return 4;
    if ((first_byte & 0x08) != 0) return 5;
    if ((first_byte & 0x04) != 0) return 6;
    if ((first_byte & 0x02) != 0) return 7;
    return 8;
}

fn vintDataMask(width: u8): u8 {
    if (width == 1) return 0x7F;
    if (width == 2) return 0x3F;
    if (width == 3) return 0x1F;
    if (width == 4) return 0x0F;
    if (width == 5) return 0x07;
    if (width == 6) return 0x03;
    if (width == 7) return 0x01;
    return 0x00;
}

fn vintAllOnes(width: u8): u64 {
    const bits: u6 = (width * 7) as u6;
    return ((1 as u64) << bits) - (1 as u64);
}

pub fn decodeEbmlId(data: []const u8): EbmlVint? {
    if (data.len == 0) {
        return null;
    }
    const width = vintWidth(data[0]);
    // RFC 8794 §5: Element ID width must be 1..4 bytes; leading 0x00 is forbidden
    if (width == 0 || width > 4 || (width as usize) > data.len) {
        return null;
    }

    const mask = vintDataMask(width);
    let value: u64 = (data[0] & mask) as u64;
    let raw_id: u32 = data[0] as u32;

    let i: usize = 1;
    while (i < (width as usize)) {
        value = (value << 8) | (data[i] as u64);
        raw_id = (raw_id << 8) | (data[i] as u32);
        i += 1;
    }

    // RFC 8794 §5.1: all-zero and all-one VINT_DATA are reserved/invalid IDs
    const all_ones = vintAllOnes(width);
    if (value == 0 || value == all_ones) {
        return null;
    }

    return EbmlVint{
        .value = value,
        .raw_id = raw_id,
        .bytes_read = width,
        .is_unknown_size = false,
    };
}

pub fn decodeEbmlSize(data: []const u8): EbmlVint? {
    if (data.len == 0) {
        return null;
    }
    const width = vintWidth(data[0]);
    if (width == 0 || (width as usize) > data.len) {
        return null;
    }

    const mask = vintDataMask(width);
    let value: u64 = (data[0] & mask) as u64;

    let i: usize = 1;
    while (i < (width as usize)) {
        value = (value << 8) | (data[i] as u64);
        i += 1;
    }

    const all_ones = vintAllOnes(width);
    return EbmlVint{
        .value = value,
        .raw_id = 0,
        .bytes_read = width,
        .is_unknown_size = value == all_ones,
    };
}

pub fn isEbmlHeader(data: []const u8): bool {
    if (data.len < 4) {
        return false;
    }
    return data[0] == 0x1A && data[1] == 0x45 && data[2] == 0xDF && data[3] == 0xA3;
}

pub struct EbmlElementIter {
    data: []const u8,
    pos: usize,

    pub static fn init(data: []const u8): EbmlElementIter {
        return EbmlElementIter{
            .data = data,
            .pos = 0,
        };
    }

    pub fn next(*_): EbmlElement? {
        if (_.pos >= _.data.len) {
            return null;
        }

        const id_opt = decodeEbmlId(_.data[_.pos..]);
        if (id_opt == null) {
            _.pos = _.data.len;
            return null;
        }
        const id_vint = id_opt.?;
        const size_pos = _.pos + (id_vint.bytes_read as usize);

        const size_opt = decodeEbmlSize(_.data[size_pos..]);
        if (size_opt == null) {
            _.pos = _.data.len;
            return null;
        }
        const size_vint = size_opt.?;
        const payload_start = size_pos + (size_vint.bytes_read as usize);

        if (size_vint.is_unknown_size) {
            _.pos = _.data.len;
            return EbmlElement{
                .id = id_vint.raw_id,
                .size = size_vint.value,
                .is_unknown_size = true,
                .payload = _.data[payload_start.._.data.len],
            };
        }

        const remaining: u64 = (_.data.len - payload_start) as u64;
        if (size_vint.value > remaining) {
            _.pos = _.data.len;
            return null;
        }

        const payload_end = payload_start + (size_vint.value as usize);
        _.pos = payload_end;

        return EbmlElement{
            .id = id_vint.raw_id,
            .size = size_vint.value,
            .is_unknown_size = false,
            .payload = _.data[payload_start..payload_end],
        };
    }
}

/// Decodes an RFC 8794 §7.2 big-endian unsigned integer (0..8 bytes; 0 bytes => 0).
pub fn decodeEbmlUint(payload: []const u8): u64? {
    if (payload.len > 8) {
        return null;
    }
    let v: u64 = 0;
    let i: usize = 0;
    while (i < payload.len) {
        v = (v << 8) | (payload[i] as u64);
        i += 1;
    }
    return v;
}

/// Decodes an RFC 8794 §7.1 big-endian two's-complement signed integer (0..8 bytes; 0 bytes => 0).
pub fn decodeEbmlSint(payload: []const u8): i64? {
    if (payload.len == 0) {
        return 0;
    }
    if (payload.len > 8) {
        return null;
    }
    const is_neg = (payload[0] & 0x80) != 0;
    const neg_fill: u64 = @bitCast(@as(i64, -1));
    let acc: u64 = if (is_neg) neg_fill else 0;
    let i: usize = 0;
    while (i < payload.len) : (i += 1) {
        acc = (acc << 8) | (payload[i] as u64);
    }
    return @bitCast(acc);
}

/// Decodes an RFC 8794 §7.3 big-endian IEEE 754 float (0 bytes => 0.0, 4 bytes => f32, 8 bytes => f64).
pub fn decodeEbmlFloat(payload: []const u8): f64? {
    if (payload.len == 0) {
        return 0.0;
    }
    if (payload.len == 4) {
        const bits32: u32 = ((payload[0] as u32) << 24)
            | ((payload[1] as u32) << 16)
            | ((payload[2] as u32) << 8)
            | (payload[3] as u32);
        const f: f32 = @bitCast(bits32);
        return f as f64;
    }
    if (payload.len == 8) {
        const bits64: u64 = ((payload[0] as u64) << 56)
            | ((payload[1] as u64) << 48)
            | ((payload[2] as u64) << 40)
            | ((payload[3] as u64) << 32)
            | ((payload[4] as u64) << 24)
            | ((payload[5] as u64) << 16)
            | ((payload[6] as u64) << 8)
            | (payload[7] as u64);
        const d: f64 = @bitCast(bits64);
        return d;
    }
    return null;
}

fn isKnownMasterElement(id: u32): bool {
    return id == EBML_ID_HEADER
        || id == EBML_ID_SEGMENT
        || id == 0x1549A966 // Info
        || id == 0x1654AE6B // Tracks
        || id == 0xAE       // TrackEntry
        || id == 0xE0       // Video
        || id == 0xE1       // Audio
        || id == 0x1F43B675 // Cluster
        || id == 0xA0;      // BlockGroup
}

fn validateEbmlLevel(
    data: []const u8,
    depth: usize,
    max_depth: usize,
    elem_count: *usize,
    allow_unknown: bool,
): EbmlError!void {
    if (depth > max_depth) return err.MaxDepthExceeded;
    let pos: usize = 0;
    while (pos < data.len) {
        if (elem_count.* >= EBML_MAX_ELEMENTS) return err.MaxElementsExceeded;
        elem_count.* += 1;

        const id_vint = decodeEbmlId(data[pos..]) ?? return err.InvalidElementId;
        const size_pos = pos + (id_vint.bytes_read as usize);
        if (size_pos >= data.len) return err.TruncatedInput;

        const size_vint = decodeEbmlSize(data[size_pos..]) ?? return err.InvalidVint;
        const payload_start = size_pos + (size_vint.bytes_read as usize);

        if (size_vint.is_unknown_size) {
            if (!allow_unknown || !isKnownMasterElement(id_vint.raw_id)) {
                return err.UnknownSizeNotAllowed;
            }
            const sub = data[payload_start..data.len];
            validateEbmlLevel(sub, depth + 1, max_depth, elem_count, true)?;
            return;
        }

        const rem: u64 = (data.len - payload_start) as u64;
        if (size_vint.value > rem) return err.PayloadOutOfBounds;
        const payload_end = payload_start + (size_vint.value as usize);
        const payload = data[payload_start..payload_end];

        if (isKnownMasterElement(id_vint.raw_id)) {
            validateEbmlLevel(payload, depth + 1, max_depth, elem_count, id_vint.raw_id == EBML_ID_SEGMENT)?;
        }
        pos = payload_end;
    }
}

/// Zero-allocation strict RFC 8794 EBML validator with explicit `max_depth` nesting guard.
pub fn validateEbmlStrictWithMaxDepth(data: []const u8, max_depth: usize): EbmlError!void {
    if (data.len < 4) return err.TruncatedInput;
    if (!isEbmlHeader(data)) return err.InvalidElementId;
    let count: usize = 0;
    validateEbmlLevel(data, 0, max_depth, *count, true)?;
}

/// Zero-allocation strict RFC 8794 EBML validator enforcing `EBML_MAX_DEPTH` (64).
pub fn validateEbmlStrict(data: []const u8): EbmlError!void {
    return validateEbmlStrictWithMaxDepth(data, EBML_MAX_DEPTH);
}
