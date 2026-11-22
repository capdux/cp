// Origin: Pure-CP Stdlib (Zero-Alloc ISO BMFF / MP4 Box Container Parser) | Created: 2026-11-22

import std from "std";

pub const MP4_BOX_HEADER_SIZE: usize = 8;
pub const MP4_LARGE_HEADER_SIZE: usize = 16;
pub const MP4_UUID_SIZE: usize = 16;

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
        let off: usize = 0;
        while (off + 4 <= _.compatible_brands.len) {
            if (fourccEquals(readFourCC(_.compatible_brands, off), brand)) {
                return true;
            }
            off += 4;
        }
        return false;
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
