// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ICC Color Profile Parser)

import std from "std";

pub const ICC_HEADER_SIZE: usize = 128;
pub const ICC_TAG_TABLE_OFFSET: usize = 128;
pub const ICC_TAG_ENTRY_SIZE: usize = 12;

pub struct IccHeader {
    profile_size: u32,
    cmm_type: [4]u8,
    version_major: u8,
    version_minor: u8,
    version_bugfix: u8,
    device_class: [4]u8,
    color_space: [4]u8,
    pcs: [4]u8,
    rendering_intent: u32,
    signature: [4]u8,

    fn isRgb(): bool {
        return iccTagEquals(_.color_space, "RGB ");
    }

    fn isCmyk(): bool {
        return iccTagEquals(_.color_space, "CMYK");
    }

    fn isDisplay(): bool {
        return iccTagEquals(_.device_class, "mntr");
    }
}

pub struct IccTagEntry {
    sig: [4]u8,
    offset: u32,
    size: u32,

    fn slice(data: []const u8): ([]const u8)? {
        const end_u64: u64 = (_.offset as u64) + (_.size as u64);
        if (end_u64 > (data.len as u64)) {
            return null;
        }
        return data[(_.offset as usize)..(end_u64 as usize)];
    }
}

fn readU32BE(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn readFourCC(buf: []const u8, off: usize): [4]u8 {
    let tag: [4]u8 = std.mem.zeroes([4]u8);
    tag[0] = buf[off];
    tag[1] = buf[off + 1];
    tag[2] = buf[off + 2];
    tag[3] = buf[off + 3];
    return tag;
}

pub fn iccTagEquals(tag: [4]u8, s: []const u8): bool {
    if (s.len != 4) {
        return false;
    }
    return tag[0] == s[0] && tag[1] == s[1] && tag[2] == s[2] && tag[3] == s[3];
}

pub fn isIccProfile(data: []const u8): bool {
    if (data.len < ICC_HEADER_SIZE) {
        return false;
    }
    return data[36] == 'a' && data[37] == 'c' && data[38] == 's' && data[39] == 'p';
}

pub fn parseIccHeader(data: []const u8): IccHeader? {
    if (!isIccProfile(data)) {
        return null;
    }
    const profile_size = readU32BE(data, 0);
    if ((profile_size as usize) < ICC_HEADER_SIZE || (profile_size as usize) > data.len) {
        return null;
    }

    const version_major = data[8];
    const ver_bcd = data[9];
    const version_minor: u8 = (ver_bcd >> 4) & 0x0F;
    const version_bugfix: u8 = ver_bcd & 0x0F;

    return IccHeader{
        .profile_size = profile_size,
        .cmm_type = readFourCC(data, 4),
        .version_major = version_major,
        .version_minor = version_minor,
        .version_bugfix = version_bugfix,
        .device_class = readFourCC(data, 12),
        .color_space = readFourCC(data, 16),
        .pcs = readFourCC(data, 20),
        .rendering_intent = readU32BE(data, 64),
        .signature = readFourCC(data, 36),
    };
}

pub struct IccTagIter {
    data: []const u8,
    pos: usize,
    remaining: u32,

    fn init(data: []const u8): IccTagIter {
        if (hdr := parseIccHeader(data)) {
            const bound: usize = hdr.profile_size as usize;
            if (bound >= ICC_TAG_TABLE_OFFSET + 4) {
                const tag_count = readU32BE(data, ICC_TAG_TABLE_OFFSET);
                const entries_start = ICC_TAG_TABLE_OFFSET + 4;
                const avail_bytes: u64 = (bound - entries_start) as u64;
                const needed_bytes: u64 = (tag_count as u64) * (ICC_TAG_ENTRY_SIZE as u64);
                if (needed_bytes <= avail_bytes) {
                    return IccTagIter{
                        .data = data[0..bound],
                        .pos = entries_start,
                        .remaining = tag_count,
                    };
                }
            }
        }
        return IccTagIter{
            .data = data[0..0],
            .pos = 0,
            .remaining = 0,
        };
    }

    fn next(*_): IccTagEntry? {
        if (_.remaining == 0 || _.pos + ICC_TAG_ENTRY_SIZE > _.data.len) {
            _.remaining = 0;
            return null;
        }
        const entry = IccTagEntry{
            .sig = readFourCC(_.data, _.pos),
            .offset = readU32BE(_.data, _.pos + 4),
            .size = readU32BE(_.data, _.pos + 8),
        };
        _.pos += ICC_TAG_ENTRY_SIZE;
        _.remaining -= 1;
        return entry;
    }
}

pub fn findIccTag(data: []const u8, sig: []const u8): IccTagEntry? {
    if (sig.len != 4) {
        return null;
    }
    let it = IccTagIter.init(data);
    while (entry := it.next()) {
        if (iccTagEquals(entry.sig, sig)) {
            return entry;
        }
    }
    return null;
}
