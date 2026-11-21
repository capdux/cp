// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc FLAC Audio Metadata Parser)

import std from "std";

pub const FLAC_MAGIC_SIZE: usize = 4;
pub const FLAC_BLOCK_HEADER_SIZE: usize = 4;
pub const FLAC_STREAMINFO_SIZE: usize = 34;

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

fn readU16BE(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readU24BE(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 16) |
        ((buf[off + 1] as u32) << 8) |
        (buf[off + 2] as u32);
}

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
        if (_.done || _.pos + FLAC_BLOCK_HEADER_SIZE > _.data.len) {
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

    let md5: [16]u8 = std.mem.zeroes([16]u8);
    let i: usize = 0;
    while (i < 16) {
        md5[i] = payload[18 + i];
        i += 1;
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
