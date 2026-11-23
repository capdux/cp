// Reference-hardened against alfg/mp4-rust (MIT)
// Origin: Pure-CP Stdlib (Zero-Alloc ISO BMFF / MP4 Box Container Parser) | Created: 2026-11-22

import std from "std";

pub const MP4_BOX_HEADER_SIZE: usize = 8;
pub const MP4_LARGE_HEADER_SIZE: usize = 16;
pub const MP4_UUID_SIZE: usize = 16;
pub const MP4_MAX_BOX_DEPTH: usize = 32;

pub error Mp4Error {
    UnexpectedEof,
    InvalidBoxSize,
    MaxDepthExceeded,
    InvalidBoxFormat,
    OutOfMemory,
}

/// 16-lane SIMD `@Vector(16, u8)` payload copy for hot-path MP4 Box / sample data.
pub fn mp4CopyPayloadSimd(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i + 16 <= src.len) : (i += 16) {
        const chunk: @Vector(16, u8) = src[i..][0..16].*;
        dst[i..][0..16].* = chunk;
    }
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Scalar reference payload copy for 0-diff parity testing against `mp4CopyPayloadSimd`.
pub fn mp4CopyPayloadScalar(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane SIMD FourCC brand search across `compatible_brands` (4 brands per iteration).
pub fn mp4HasBrandSimd(compatible_brands: []const u8, brand: []const u8): bool {
    if (brand.len != 4) return false;
    const target_u32: u32 = std.mem.readInt(u32, brand[0..4], .little);
    const target_v: @Vector(4, u32) = @splat(target_u32);
    let off: usize = 0;
    while (off + 16 <= compatible_brands.len) : (off += 16) {
        const bytes_v: @Vector(16, u8) = compatible_brands[off..][0..16].*;
        const words_v: @Vector(4, u32) = @bitCast(bytes_v);
        const eq_mask = words_v == target_v;
        if (@reduce(.Or, eq_mask)) return true;
    }
    while (off + 4 <= compatible_brands.len) : (off += 4) {
        if (compatible_brands[off] == brand[0] &&
            compatible_brands[off + 1] == brand[1] &&
            compatible_brands[off + 2] == brand[2] &&
            compatible_brands[off + 3] == brand[3])
        {
            return true;
        }
    }
    return false;
}

/// Scalar reference FourCC brand search for 0-diff parity testing against `mp4HasBrandSimd`.
pub fn mp4HasBrandScalar(compatible_brands: []const u8, brand: []const u8): bool {
    if (brand.len != 4) return false;
    let off: usize = 0;
    while (off + 4 <= compatible_brands.len) : (off += 4) {
        if (compatible_brands[off] == brand[0] &&
            compatible_brands[off + 1] == brand[1] &&
            compatible_brands[off + 2] == brand[2] &&
            compatible_brands[off + 3] == brand[3])
        {
            return true;
        }
    }
    return false;
}

/// 4-lane SIMD big-endian `u32` array unpack (`@Vector(16, u8)` `@shuffle` -> `@Vector(4, u32)`).
pub fn mp4UnpackU32BESimd(dst: []u32, src: []const u8) {
    const swap_mask: [16]i32 = [ 3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12 ];
    let di: usize = 0;
    let si: usize = 0;
    while (di + 4 <= dst.len && si + 16 <= src.len) {
        const v: @Vector(16, u8) = src[si..][0..16].*;
        const swapped = @shuffle(u8, v, undefined, swap_mask);
        const words: [4]u32 = @bitCast(swapped);
        dst[di] = words[0];
        dst[di + 1] = words[1];
        dst[di + 2] = words[2];
        dst[di + 3] = words[3];
        di += 4;
        si += 16;
    }
    while (di < dst.len && si + 4 <= src.len) {
        dst[di] = readU32BE(src, si);
        di += 1;
        si += 4;
    }
}

/// Scalar reference big-endian `u32` array unpack for 0-diff parity testing against `mp4UnpackU32BESimd`.
pub fn mp4UnpackU32BEScalar(dst: []u32, src: []const u8) {
    let di: usize = 0;
    let si: usize = 0;
    while (di < dst.len && si + 4 <= src.len) {
        dst[di] = readU32BE(src, si);
        di += 1;
        si += 4;
    }
}

pub struct Mp4Box {
    box_type: [4]u8,
    size: u64,
    header_size: u8,
    has_uuid: bool,
    uuid: [16]u8,
    payload: []const u8,

    fn isType(fourcc: []const u8): bool {
        return fourccEquals(_.box_type, fourcc);
    }

    fn isContainer(): bool {
        return fourccEquals(_.box_type, "moov") ||
            fourccEquals(_.box_type, "trak") ||
            fourccEquals(_.box_type, "mdia") ||
            fourccEquals(_.box_type, "minf") ||
            fourccEquals(_.box_type, "stbl") ||
            fourccEquals(_.box_type, "dinf") ||
            fourccEquals(_.box_type, "edts") ||
            fourccEquals(_.box_type, "udta") ||
            fourccEquals(_.box_type, "moof") ||
            fourccEquals(_.box_type, "traf") ||
            fourccEquals(_.box_type, "mvex") ||
            fourccEquals(_.box_type, "meta");
    }

    fn children(): BoxCursor {
        // ISO/IEC 14496-12: 'meta' is a FullBox (4-byte version+flags precedes child boxes)
        if (fourccEquals(_.box_type, "meta")) {
            if (_.payload.len >= 4) {
                return BoxCursor.init(_.payload[4..]);
            }
            return BoxCursor.init(_.payload[0..0]);
        }
        return BoxCursor.init(_.payload);
    }
}

pub struct FtypBox {
    major_brand: [4]u8,
    minor_version: u32,
    compatible_brands: []const u8,

    fn brandCount(): usize {
        return _.compatible_brands.len / 4;
    }

    fn brandAt(index: usize): ([4]u8)? {
        const off = index * 4;
        if (off + 4 > _.compatible_brands.len) {
            return null;
        }
        return readFourCC(_.compatible_brands, off);
    }

    fn hasBrand(brand: []const u8): bool {
        if (brand.len != 4) {
            return false;
        }
        if (fourccEquals(_.major_brand, brand)) {
            return true;
        }
        return mp4HasBrandSimd(_.compatible_brands, brand);
    }
}

pub struct MvhdBox {
    version: u8,
    flags: u32,
    creation_time: u64,
    modification_time: u64,
    timescale: u32,
    duration: u64,
    rate_fixed: u32,
    volume_fixed: u16,
    next_track_id: u32,

    fn durationMs(): u64 {
        const ts: u64 = _.timescale as u64;
        if (ts == 0) {
            return 0;
        }
        const secs: u64 = _.duration / ts;
        const rem: u64 = _.duration % ts;
        return secs * (1000 as u64) + (rem * (1000 as u64)) / ts;
    }

    fn durationSecs(): f64 {
        if (_.timescale == 0) {
            return 0.0;
        }
        return (_.duration as f64) / (_.timescale as f64);
    }
}

pub struct MdhdBox {
    version: u8,
    creation_time: u64,
    modification_time: u64,
    timescale: u32,
    duration: u64,
    language: [3]u8,

    fn durationMs(): u64 {
        const ts: u64 = _.timescale as u64;
        if (ts == 0) {
            return 0;
        }
        const secs: u64 = _.duration / ts;
        const rem: u64 = _.duration % ts;
        return secs * (1000 as u64) + (rem * (1000 as u64)) / ts;
    }
}

pub struct TkhdBox {
    version: u8,
    flags: u32,
    track_id: u32,
    duration: u64,
    width: u16,
    height: u16,
}

fn readU16BE(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readU24BE(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 16) |
        ((buf[off + 1] as u32) << 8) |
        (buf[off + 2] as u32);
}

fn readU32BE(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn readU64BE(buf: []const u8, off: usize): u64 {
    const hi: u64 = readU32BE(buf, off) as u64;
    const lo: u64 = readU32BE(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn readFourCC(buf: []const u8, off: usize): [4]u8 {
    let tag: [4]u8 = std.mem.zeroes([4]u8);
    tag[0] = buf[off];
    tag[1] = buf[off + 1];
    tag[2] = buf[off + 2];
    tag[3] = buf[off + 3];
    return tag;
}

pub fn fourccEquals(tag: [4]u8, s: []const u8): bool {
    if (s.len != 4) {
        return false;
    }
    return tag[0] == s[0] && tag[1] == s[1] && tag[2] == s[2] && tag[3] == s[3];
}

pub struct BoxCursor {
    data: []const u8,
    pos: usize,

    fn init(data: []const u8): BoxCursor {
        return BoxCursor{
            .data = data,
            .pos = 0,
        };
    }

    fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    fn next(*_): Mp4Box? {
        if (_.pos + MP4_BOX_HEADER_SIZE > _.data.len) {
            _.pos = _.data.len;
            return null;
        }

        const start = _.pos;
        const avail: u64 = (_.data.len - start) as u64;
        const size32 = readU32BE(_.data, start);
        const box_type = readFourCC(_.data, start + 4);

        let total_size: u64 = 0;
        let hdr_size: usize = MP4_BOX_HEADER_SIZE;

        if (size32 == 0) {
            // Extends to end of container/stream
            total_size = avail;
        } else if (size32 == 1) {
            // 64-bit largesize follows the 8-byte header
            if (avail < (MP4_LARGE_HEADER_SIZE as u64)) {
                _.pos = _.data.len;
                return null;
            }
            total_size = readU64BE(_.data, start + 8);
            hdr_size = MP4_LARGE_HEADER_SIZE;
            if (total_size < (MP4_LARGE_HEADER_SIZE as u64) || total_size > avail) {
                _.pos = _.data.len;
                return null;
            }
        } else if (size32 < 8) {
            // Invalid box size (2..7 cannot hold even the 8-byte header)
            _.pos = _.data.len;
            return null;
        } else {
            total_size = size32 as u64;
            if (total_size > avail) {
                _.pos = _.data.len;
                return null;
            }
        }

        let has_uuid = false;
        let uuid: [16]u8 = std.mem.zeroes([16]u8);
        if (fourccEquals(box_type, "uuid")) {
            if (total_size < ((hdr_size + MP4_UUID_SIZE) as u64)) {
                _.pos = _.data.len;
                return null;
            }
            has_uuid = true;
            let i: usize = 0;
            while (i < MP4_UUID_SIZE) {
                uuid[i] = _.data[start + hdr_size + i];
                i += 1;
            }
            hdr_size += MP4_UUID_SIZE;
        }

        const end_pos = start + (total_size as usize);
        const payload_start = start + hdr_size;
        _.pos = end_pos;

        return Mp4Box{
            .box_type = box_type,
            .size = total_size,
            .header_size = hdr_size as u8,
            .has_uuid = has_uuid,
            .uuid = uuid,
            .payload = _.data[payload_start..end_pos],
        };
    }
}

pub fn findBox(data: []const u8, fourcc: []const u8): Mp4Box? {
    if (fourcc.len != 4) {
        return null;
    }
    let cur = BoxCursor.init(data);
    while (box := cur.next()) {
        if (fourccEquals(box.box_type, fourcc)) {
            return box;
        }
    }
    return null;
}

pub fn findBoxPath(data: []const u8, path: []const u8): Mp4Box? {
    if (path.len == 0) {
        return null;
    }
    let cur_slice = data;
    let i: usize = 0;
    let found: Mp4Box? = null;

    while (i < path.len) {
        let seg_end = i;
        while (seg_end < path.len && path[seg_end] != '/') {
            seg_end += 1;
        }
        const seg = path[i..seg_end];
        if (seg.len != 4) {
            return null;
        }
        const box = findBox(cur_slice, seg) ?? return null;
        found = box;

        if (seg_end >= path.len) {
            return found;
        }
        // Advance into child payload (handling 'meta' 4-byte FullBox header)
        if (fourccEquals(box.box_type, "meta")) {
            if (box.payload.len < 4) {
                return null;
            }
            cur_slice = box.payload[4..];
        } else {
            cur_slice = box.payload;
        }
        i = seg_end + 1;
    }

    return found;
}

pub fn parseFtyp(payload: []const u8): FtypBox? {
    if (payload.len < 8) {
        return null;
    }
    if (((payload.len - 8) & 3) != 0) {
        return null;
    }
    return FtypBox{
        .major_brand = readFourCC(payload, 0),
        .minor_version = readU32BE(payload, 4),
        .compatible_brands = payload[8..],
    };
}

pub fn isMp4(data: []const u8): bool {
    let cur = BoxCursor.init(data);
    const first = cur.next() ?? return false;
    if (!first.isType("ftyp")) {
        return false;
    }
    return parseFtyp(first.payload) != null;
}

pub fn parseMvhd(payload: []const u8): MvhdBox? {
    if (payload.len < 4) {
        return null;
    }
    const version = payload[0];
    const flags = readU24BE(payload, 1);

    if (version == 0) {
        if (payload.len < 100) {
            return null;
        }
        return MvhdBox{
            .version = 0,
            .flags = flags,
            .creation_time = readU32BE(payload, 4) as u64,
            .modification_time = readU32BE(payload, 8) as u64,
            .timescale = readU32BE(payload, 12),
            .duration = readU32BE(payload, 16) as u64,
            .rate_fixed = readU32BE(payload, 20),
            .volume_fixed = readU16BE(payload, 24),
            .next_track_id = readU32BE(payload, 96),
        };
    } else if (version == 1) {
        if (payload.len < 112) {
            return null;
        }
        return MvhdBox{
            .version = 1,
            .flags = flags,
            .creation_time = readU64BE(payload, 4),
            .modification_time = readU64BE(payload, 12),
            .timescale = readU32BE(payload, 20),
            .duration = readU64BE(payload, 24),
            .rate_fixed = readU32BE(payload, 32),
            .volume_fixed = readU16BE(payload, 36),
            .next_track_id = readU32BE(payload, 108),
        };
    }

    return null;
}

pub fn parseMdhd(payload: []const u8): MdhdBox? {
    if (payload.len < 4) {
        return null;
    }
    const version = payload[0];
    let creation_time: u64 = 0;
    let modification_time: u64 = 0;
    let timescale: u32 = 0;
    let duration: u64 = 0;
    let lang_off: usize = 0;

    if (version == 0) {
        if (payload.len < 24) {
            return null;
        }
        creation_time = readU32BE(payload, 4) as u64;
        modification_time = readU32BE(payload, 8) as u64;
        timescale = readU32BE(payload, 12);
        duration = readU32BE(payload, 16) as u64;
        lang_off = 20;
    } else if (version == 1) {
        if (payload.len < 36) {
            return null;
        }
        creation_time = readU64BE(payload, 4);
        modification_time = readU64BE(payload, 12);
        timescale = readU32BE(payload, 20);
        duration = readU64BE(payload, 24);
        lang_off = 32;
    } else {
        return null;
    }

    const lang_packed = readU16BE(payload, lang_off);
    let language: [3]u8 = std.mem.zeroes([3]u8);
    language[0] = (((lang_packed >> 10) & 0x1F) + 0x60) as u8;
    language[1] = (((lang_packed >> 5) & 0x1F) + 0x60) as u8;
    language[2] = ((lang_packed & 0x1F) + 0x60) as u8;

    return MdhdBox{
        .version = version,
        .creation_time = creation_time,
        .modification_time = modification_time,
        .timescale = timescale,
        .duration = duration,
        .language = language,
    };
}

pub fn parseTkhd(payload: []const u8): TkhdBox? {
    if (payload.len < 4) {
        return null;
    }
    const version = payload[0];
    const flags = readU24BE(payload, 1);

    if (version == 0) {
        if (payload.len < 84) {
            return null;
        }
        return TkhdBox{
            .version = 0,
            .flags = flags,
            .track_id = readU32BE(payload, 12),
            .duration = readU32BE(payload, 20) as u64,
            .width = readU16BE(payload, 76),
            .height = readU16BE(payload, 80),
        };
    } else if (version == 1) {
        if (payload.len < 96) {
            return null;
        }
        return TkhdBox{
            .version = 1,
            .flags = flags,
            .track_id = readU32BE(payload, 20),
            .duration = readU64BE(payload, 28),
            .width = readU16BE(payload, 88),
            .height = readU16BE(payload, 92),
        };
    }

    return null;
}

pub struct HdlrBox {
    version: u8,
    flags: u32,
    handler_type: [4]u8,
    name: []const u8,

    pub fn isVideo(): bool {
        return fourccEquals(_.handler_type, "vide");
    }

    pub fn isAudio(): bool {
        return fourccEquals(_.handler_type, "soun");
    }

    pub fn isSubtitle(): bool {
        return fourccEquals(_.handler_type, "subt");
    }
}

pub fn parseHdlr(payload: []const u8): HdlrBox? {
    if (payload.len < 24) return null;
    return HdlrBox {
        .version = payload[0],
        .flags = readU24BE(payload, 1),
        .handler_type = readFourCC(payload, 8),
        .name = payload[24..],
    };
}

pub struct StszBox {
    version: u8,
    flags: u32,
    sample_size: u32,
    sample_count: u32,
    entry_bytes: []const u8,

    pub fn sampleSizeAt(index: u32): u32? {
        if (index >= _.sample_count) return null;
        if (_.sample_size != 0) return _.sample_size;
        const off = (index as usize) * 4;
        if (off + 4 > _.entry_bytes.len) return null;
        return readU32BE(_.entry_bytes, off);
    }
}

pub fn parseStsz(payload: []const u8): StszBox? {
    if (payload.len < 12) return null;
    const sample_size = readU32BE(payload, 4);
    const sample_count = readU32BE(payload, 8);
    const entries = payload[12..];
    if (sample_size == 0) {
        const need = @mulWithOverflow(sample_count as usize, 4);
        if (need[1] != 0 || need[0] > entries.len) return null;
    }
    return StszBox {
        .version = payload[0],
        .flags = readU24BE(payload, 1),
        .sample_size = sample_size,
        .sample_count = sample_count,
        .entry_bytes = entries,
    };
}

pub struct StcoBox {
    version: u8,
    flags: u32,
    is_co64: bool,
    entry_count: u32,
    entry_bytes: []const u8,

    pub fn chunkOffsetAt(index: u32): u64? {
        if (index >= _.entry_count) return null;
        const elem_sz: usize = if (_.is_co64) 8 else 4;
        const off = (index as usize) * elem_sz;
        if (off + elem_sz > _.entry_bytes.len) return null;
        if (_.is_co64) return readU64BE(_.entry_bytes, off);
        return readU32BE(_.entry_bytes, off) as u64;
    }
}

pub fn parseStco(payload: []const u8): StcoBox? {
    if (payload.len < 8) return null;
    const entry_count = readU32BE(payload, 4);
    const entries = payload[8..];
    const need = @mulWithOverflow(entry_count as usize, 4);
    if (need[1] != 0 || need[0] > entries.len) return null;
    return StcoBox {
        .version = payload[0],
        .flags = readU24BE(payload, 1),
        .is_co64 = false,
        .entry_count = entry_count,
        .entry_bytes = entries,
    };
}

pub fn parseCo64(payload: []const u8): StcoBox? {
    if (payload.len < 8) return null;
    const entry_count = readU32BE(payload, 4);
    const entries = payload[8..];
    const need = @mulWithOverflow(entry_count as usize, 8);
    if (need[1] != 0 || need[0] > entries.len) return null;
    return StcoBox {
        .version = payload[0],
        .flags = readU24BE(payload, 1),
        .is_co64 = true,
        .entry_count = entry_count,
        .entry_bytes = entries,
    };
}

pub struct SttsEntry {
    sample_count: u32,
    sample_delta: u32,
}

pub struct SttsBox {
    version: u8,
    flags: u32,
    entry_count: u32,
    entry_bytes: []const u8,

    pub fn entryAt(index: u32): SttsEntry? {
        if (index >= _.entry_count) return null;
        const off = (index as usize) * 8;
        if (off + 8 > _.entry_bytes.len) return null;
        return SttsEntry {
            .sample_count = readU32BE(_.entry_bytes, off),
            .sample_delta = readU32BE(_.entry_bytes, off + 4),
        };
    }

    pub fn totalDuration(): u64 {
        let total: u64 = 0;
        let i: u32 = 0;
        while (i < _.entry_count) : (i += 1) {
            if (e := _.entryAt(i)) {
                total += (e.sample_count as u64) * (e.sample_delta as u64);
            }
        }
        return total;
    }
}

pub fn parseStts(payload: []const u8): SttsBox? {
    if (payload.len < 8) return null;
    const entry_count = readU32BE(payload, 4);
    const entries = payload[8..];
    const need = @mulWithOverflow(entry_count as usize, 8);
    if (need[1] != 0 || need[0] > entries.len) return null;
    return SttsBox {
        .version = payload[0],
        .flags = readU24BE(payload, 1),
        .entry_count = entry_count,
        .entry_bytes = entries,
    };
}

fn validateBoxSlice(data: []const u8, depth: usize, max_depth: usize): Mp4Error!void {
    if (depth > max_depth) return Mp4Error.MaxDepthExceeded;
    let pos: usize = 0;
    while (pos < data.len) {
        if (data.len - pos < MP4_BOX_HEADER_SIZE) return Mp4Error.UnexpectedEof;
        const avail: u64 = (data.len - pos) as u64;
        const size32 = readU32BE(data, pos);
        const box_type = readFourCC(data, pos + 4);

        let total_size: u64 = 0;
        let hdr_size: usize = MP4_BOX_HEADER_SIZE;
        if (size32 == 0) {
            total_size = avail;
        } else if (size32 == 1) {
            if (avail < (MP4_LARGE_HEADER_SIZE as u64)) return Mp4Error.UnexpectedEof;
            total_size = readU64BE(data, pos + 8);
            hdr_size = MP4_LARGE_HEADER_SIZE;
            if (total_size < (MP4_LARGE_HEADER_SIZE as u64)) return Mp4Error.InvalidBoxSize;
            if (total_size > avail) return Mp4Error.UnexpectedEof;
        } else if (size32 < 8) {
            return Mp4Error.InvalidBoxSize;
        } else {
            total_size = size32 as u64;
            if (total_size > avail) return Mp4Error.UnexpectedEof;
        }

        if (fourccEquals(box_type, "uuid")) {
            if (total_size < ((hdr_size + MP4_UUID_SIZE) as u64)) return Mp4Error.InvalidBoxSize;
            hdr_size += MP4_UUID_SIZE;
        }

        const end_pos = pos + (total_size as usize);
        const payload = data[pos + hdr_size .. end_pos];
        const tmp_box = Mp4Box {
            .box_type = box_type,
            .size = total_size,
            .header_size = hdr_size as u8,
            .has_uuid = false,
            .uuid = std.mem.zeroes([16]u8),
            .payload = payload,
        };
        if (tmp_box.isContainer()) {
            const child_slice = if (fourccEquals(box_type, "meta")) blk: {
                if (payload.len < 4) return Mp4Error.InvalidBoxFormat;
                break :blk payload[4..];
            } else payload;
            try validateBoxSlice(child_slice, depth + 1, max_depth);
        }
        pos = end_pos;
    }
}

/// Zero-heap full ISO BMFF / MP4 box tree validator with recursion depth guard (`MP4_MAX_BOX_DEPTH = 32`).
pub fn validateMp4Structure(data: []const u8): Mp4Error!void {
    try validateMp4StructureWithMaxDepth(data, MP4_MAX_BOX_DEPTH);
}

/// Zero-heap full ISO BMFF / MP4 box tree validator with explicit recursion depth limit.
pub fn validateMp4StructureWithMaxDepth(data: []const u8, max_depth: usize): Mp4Error!void {
    if (data.len == 0) return Mp4Error.UnexpectedEof;
    try validateBoxSlice(data, 0, max_depth);
}
