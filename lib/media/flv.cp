// Origin: Pure-CP Stdlib (Zero-Alloc Adobe FLV Container & Tag Parser) | Created: 2026-11-22

import std from "std";

pub const FLV_HEADER_SIZE: usize = 9;
pub const FLV_TAG_HEADER_SIZE: usize = 11;
pub const FLV_TAG_AUDIO: u8 = 8;
pub const FLV_TAG_VIDEO: u8 = 9;
pub const FLV_TAG_SCRIPT: u8 = 18;

pub enum FlvTagType {
    audio,
    video,
    script,
    unknown,
}

pub struct FlvHeader {
    version: u8,
    has_audio: bool,
    has_video: bool,
    data_offset: u32,
}

pub struct FlvTag {
    tag_type: FlvTagType,
    raw_type: u8,
    is_filtered: bool,
    data_size: u32,
    timestamp_ms: u32,
    stream_id: u32,
    payload: []const u8,
    previous_tag_size: u32,
}

pub struct FlvAudioInfo {
    sound_format: u8,
    sample_rate_hz: u32,
    is_16bit: bool,
    is_stereo: bool,
    aac_packet_type: u8?,
}

pub struct FlvVideoInfo {
    frame_type: u8,
    codec_id: u8,
    avc_packet_type: u8?,
    composition_time_ms: i32,

    fn isKeyframe(): bool {
        return _.frame_type == 1;
    }
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

fn mapTagType(raw: u8): FlvTagType {
    if (raw == FLV_TAG_AUDIO) return FlvTagType.audio;
    if (raw == FLV_TAG_VIDEO) return FlvTagType.video;
    if (raw == FLV_TAG_SCRIPT) return FlvTagType.script;
    return FlvTagType.unknown;
}

pub fn isFlv(data: []const u8): bool {
    if (data.len < FLV_HEADER_SIZE) {
        return false;
    }
    if (data[0] != 'F' || data[1] != 'L' || data[2] != 'V') {
        return false;
    }
    const data_offset = readU32BE(data, 5);
    return (data_offset as usize) >= FLV_HEADER_SIZE && (data_offset as usize) <= data.len;
}

pub fn parseFlvHeader(data: []const u8): FlvHeader? {
    if (!isFlv(data)) {
        return null;
    }
    const version = data[3];
    const flags = data[4];
    const data_offset = readU32BE(data, 5);

    return FlvHeader{
        .version = version,
        .has_audio = (flags & 0x04) != 0,
        .has_video = (flags & 0x01) != 0,
        .data_offset = data_offset,
    };
}

pub struct FlvTagIter {
    data: []const u8,
    pos: usize,
    done: bool,

    static fn init(data: []const u8): FlvTagIter {
        if (hdr := parseFlvHeader(data)) {
            const first_tag_off = (hdr.data_offset as usize) + 4; // skip PreviousTagSize0
            if (first_tag_off <= data.len) {
                return FlvTagIter{
                    .data = data,
                    .pos = first_tag_off,
                    .done = false,
                };
            }
        }
        return FlvTagIter{
            .data = data[0..0],
            .pos = 0,
            .done = true,
        };
    }

    fn next(*_): FlvTag? {
        if (_.done || _.pos + FLV_TAG_HEADER_SIZE > _.data.len) {
            _.done = true;
            return null;
        }

        const start = _.pos;
        const b0 = _.data[start];
        const is_filtered = (b0 & 0x20) != 0;
        const raw_type = b0 & 0x1F;
        const data_size = readU24BE(_.data, start + 1);
        const ts_lo = readU24BE(_.data, start + 4);
        const ts_ext: u32 = _.data[start + 7] as u32;
        const timestamp_ms: u32 = (ts_ext << 24) | ts_lo;
        const stream_id = readU24BE(_.data, start + 8);

        const payload_start = start + FLV_TAG_HEADER_SIZE;
        const payload_end_u64: u64 = (payload_start as u64) + (data_size as u64);
        const tag_end_u64: u64 = payload_end_u64 + (4 as u64);

        if (tag_end_u64 > (_.data.len as u64)) {
            _.done = true;
            return null;
        }

        const payload_end: usize = payload_end_u64 as usize;
        const prev_tag_size = readU32BE(_.data, payload_end);
        const expected_prev: u32 = (FLV_TAG_HEADER_SIZE as u32) + data_size;
        if (prev_tag_size != expected_prev) {
            _.done = true;
            return null;
        }

        _.pos = tag_end_u64 as usize;
        return FlvTag{
            .tag_type = mapTagType(raw_type),
            .raw_type = raw_type,
            .is_filtered = is_filtered,
            .data_size = data_size,
            .timestamp_ms = timestamp_ms,
            .stream_id = stream_id,
            .payload = _.data[payload_start..payload_end],
            .previous_tag_size = prev_tag_size,
        };
    }
}

pub fn parseFlvAudioHeader(payload: []const u8): FlvAudioInfo? {
    if (payload.len == 0) {
        return null;
    }
    const b0 = payload[0];
    const sound_format: u8 = (b0 >> 4) & 0x0F;
    const rate_idx: u8 = (b0 >> 2) & 0x03;
    const is_16bit = ((b0 >> 1) & 0x01) != 0;
    const is_stereo = (b0 & 0x01) != 0;

    let sample_rate_hz: u32 = 5512;
    if (rate_idx == 1) sample_rate_hz = 11025;
    else if (rate_idx == 2) sample_rate_hz = 22050;
    else if (rate_idx == 3) sample_rate_hz = 44100;

    let aac_packet_type: u8? = null;
    if (sound_format == 10) {
        if (payload.len < 2) {
            return null;
        }
        aac_packet_type = payload[1];
    }

    return FlvAudioInfo{
        .sound_format = sound_format,
        .sample_rate_hz = sample_rate_hz,
        .is_16bit = is_16bit,
        .is_stereo = is_stereo,
        .aac_packet_type = aac_packet_type,
    };
}

pub fn parseFlvVideoHeader(payload: []const u8): FlvVideoInfo? {
    if (payload.len == 0) {
        return null;
    }
    const b0 = payload[0];
    const frame_type: u8 = (b0 >> 4) & 0x0F;
    const codec_id: u8 = b0 & 0x0F;

    let avc_packet_type: u8? = null;
    let composition_time_ms: i32 = 0;

    if (codec_id == 7 || codec_id == 12) {
        if (payload.len < 5) {
            return null;
        }
        avc_packet_type = payload[1];
        const raw24 = readU24BE(payload, 2);
        if ((raw24 & 0x80_0000) != 0) {
            composition_time_ms = @bitCast(raw24 | 0xFF00_0000);
        } else {
            composition_time_ms = raw24 as i32;
        }
    }

    return FlvVideoInfo{
        .frame_type = frame_type,
        .codec_id = codec_id,
        .avc_packet_type = avc_packet_type,
        .composition_time_ms = composition_time_ms,
    };
}
