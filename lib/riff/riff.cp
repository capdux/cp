// Origin: Pure-CP Stdlib (Zero-Alloc RIFF/RIFX Container Chunk Parser) | Created: 2026-11-22

import std from "std";

pub const RIFF_HEADER_SIZE: usize = 12;
pub const RIFF_CHUNK_HEADER_SIZE: usize = 8;

pub struct RiffHeader {
    is_big_endian: bool,
    file_size: u32,
    form_type: [4]u8,

    fn isForm(s: []const u8): bool {
        return fourccEquals(_.form_type, s);
    }
}

pub struct RiffChunk {
    id: [4]u8,
    size: u32,
    data: []const u8,
}

fn readU32LE(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32) |
        ((buf[off + 1] as u32) << 8) |
        ((buf[off + 2] as u32) << 16) |
        ((buf[off + 3] as u32) << 24);
}

fn readU32BE(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn readU32(buf: []const u8, off: usize, is_big_endian: bool): u32 {
    if (is_big_endian) {
        return readU32BE(buf, off);
    }
    return readU32LE(buf, off);
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

pub fn parseRiffHeader(data: []const u8): RiffHeader? {
    if (data.len < RIFF_HEADER_SIZE) {
        return null;
    }
    const magic = readFourCC(data, 0);
    let is_big_endian = false;
    if (fourccEquals(magic, "RIFF")) {
        is_big_endian = false;
    } else if (fourccEquals(magic, "RIFX")) {
        is_big_endian = true;
    } else {
        return null;
    }

    const file_size = readU32(data, 4, is_big_endian);
    if (file_size < 4) {
        return null;
    }
    const total_len: u64 = (8 as u64) + (file_size as u64);
    if (total_len > (data.len as u64)) {
        return null;
    }

    return RiffHeader{
        .is_big_endian = is_big_endian,
        .file_size = file_size,
        .form_type = readFourCC(data, 8),
    };
}

pub fn parseListType(list_payload: []const u8): ([4]u8)? {
    if (list_payload.len < 4) {
        return null;
    }
    return readFourCC(list_payload, 0);
}

pub struct RiffChunkIter {
    buf: []const u8,
    pos: usize,
    is_big_endian: bool,

    fn init(data: []const u8): RiffChunkIter {
        if (hdr := parseRiffHeader(data)) {
            const end_pos: usize = ((8 as u64) + (hdr.file_size as u64)) as usize;
            return RiffChunkIter{
                .buf = data[0..end_pos],
                .pos = RIFF_HEADER_SIZE,
                .is_big_endian = hdr.is_big_endian,
            };
        }
        return RiffChunkIter{
            .buf = data[0..0],
            .pos = 0,
            .is_big_endian = false,
        };
    }

    // Iterate subchunks inside a LIST chunk payload (automatically skips the 4-byte listType)
    fn initSubchunks(list_payload: []const u8, is_big_endian: bool): RiffChunkIter {
        if (list_payload.len < 4) {
            return RiffChunkIter{
                .buf = list_payload[0..0],
                .pos = 0,
                .is_big_endian = is_big_endian,
            };
        }
        return RiffChunkIter{
            .buf = list_payload,
            .pos = 4,
            .is_big_endian = is_big_endian,
        };
    }

    fn next(*_): RiffChunk? {
        if (_.pos + RIFF_CHUNK_HEADER_SIZE > _.buf.len) {
            _.pos = _.buf.len;
            return null;
        }

        const id = readFourCC(_.buf, _.pos);
        const size = readU32(_.buf, _.pos + 4, _.is_big_endian);
        const payload_start = _.pos + RIFF_CHUNK_HEADER_SIZE;
        const payload_end_u64: u64 = (payload_start as u64) + (size as u64);

        if (payload_end_u64 > (_.buf.len as u64)) {
            _.pos = _.buf.len;
            return null;
        }

        const payload_end: usize = payload_end_u64 as usize;
        const pad: u64 = (size & 1) as u64;
        const next_pos_u64: u64 = payload_end_u64 + pad;
        if (next_pos_u64 <= (_.buf.len as u64)) {
            _.pos = next_pos_u64 as usize;
        } else {
            _.pos = payload_end;
        }

        return RiffChunk{
            .id = id,
            .size = size,
            .data = _.buf[payload_start..payload_end],
        };
    }
}

pub fn findRiffChunk(data: []const u8, chunk_id: []const u8): RiffChunk? {
    if (chunk_id.len != 4) {
        return null;
    }
    let it = RiffChunkIter.init(data);
    while (chunk := it.next()) {
        if (fourccEquals(chunk.id, chunk_id)) {
            return chunk;
        }
    }
    return null;
}
