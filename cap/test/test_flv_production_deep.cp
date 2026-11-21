// Source: cp/cap/test/test_flv_production_deep.cp (Production deep tests for cap:flv)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FLV_HEADER_SIZE,
    FLV_TAG_HEADER_SIZE,
    FLV_TAG_AUDIO,
    FLV_TAG_VIDEO,
    FLV_TAG_SCRIPT,
    FlvTagType,
    FlvHeader,
    FlvTag,
    FlvAudioInfo,
    FlvVideoInfo,
    FlvTagIter,
    isFlv,
    parseFlvHeader,
    parseFlvAudioHeader,
    parseFlvVideoHeader,
} from "cap:flv";

fn writeU24BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 16) & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = (v & 0xFF) as u8;
}

fn writeU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeFlvHeader(buf: []u8, has_audio: bool, has_video: bool, data_offset: u32) {
    buf[0] = 'F'; buf[1] = 'L'; buf[2] = 'V';
    buf[3] = 1; // version 1
    let flags: u8 = 0;
    if (has_audio) flags = flags | 0x04;
    if (has_video) flags = flags | 0x01;
    buf[4] = flags;
    writeU32BE(buf, 5, data_offset);
}

fn appendFlvTag(buf: []u8, pos: usize, tag_type_byte: u8, ts_ms: u32, payload: []const u8): usize {
    let p = pos;
    buf[p] = tag_type_byte;
    writeU24BE(buf, p + 1, payload.len as u32);
    writeU24BE(buf, p + 4, ts_ms & 0x00FF_FFFF);
    buf[p + 7] = ((ts_ms >> 24) & 0xFF) as u8;
    writeU24BE(buf, p + 8, 0); // stream_id
    p += FLV_TAG_HEADER_SIZE;

    let i: usize = 0;
    while (i < payload.len) {
        buf[p] = payload[i];
        p += 1;
        i += 1;
    }

    writeU32BE(buf, p, (FLV_TAG_HEADER_SIZE as u32) + (payload.len as u32));
    p += 4;
    return p;
}

test("1. FLV v1 header and Script/Audio/Video multi-tag stream parsing", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    writeFlvHeader(buf[0..], true, true, 9);
    writeU32BE(buf[0..], 9, 0); // PreviousTagSize0 = 0
    let pos: usize = 13;

    // Script tag (4 bytes)
    pos = appendFlvTag(buf[0..], pos, FLV_TAG_SCRIPT, 0, "META");
    // Audio tag (AAC seq hdr: 0xAF, 0x00, 0x12, 0x10)
    const aac_payload: [4]u8 = [0xAF, 0x00, 0x12, 0x10];
    pos = appendFlvTag(buf[0..], pos, FLV_TAG_AUDIO, 33, aac_payload[0..]);
    // Video tag (AVC keyframe NALU, cts=+40ms: 0x17, 0x01, 0x00, 0x00, 0x28)
    const avc_payload: [5]u8 = [0x17, 0x01, 0x00, 0x00, 0x28];
    pos = appendFlvTag(buf[0..], pos, FLV_TAG_VIDEO, 66, avc_payload[0..]);

    expect_eq(isFlv(buf[0..pos]), true);
    const hdr = parseFlvHeader(buf[0..pos]) ?? unreachable;
    expect_eq(hdr.version, 1);
    expect_eq(hdr.has_audio, true);
    expect_eq(hdr.has_video, true);
    expect_eq(hdr.data_offset, 9);

    let it = FlvTagIter.init(buf[0..pos]);

    const t0 = it.next() ?? unreachable;
    expect_eq(t0.tag_type == FlvTagType.script, true);
    expect_eq(t0.timestamp_ms, 0);
    expect_eq(std.mem.eql(u8, t0.payload, "META"), true);
    expect_eq(t0.previous_tag_size, 15);

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.tag_type == FlvTagType.audio, true);
    expect_eq(t1.timestamp_ms, 33);
    expect_eq(t1.payload.len, 4);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.tag_type == FlvTagType.video, true);
    expect_eq(t2.timestamp_ms, 66);
    expect_eq(t2.payload.len, 5);

    expect_eq(it.next() == null, true);
});

test("2. 32-bit extended timestamp combination and filtered tag flag", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    writeFlvHeader(buf[0..], false, true, 9);
    writeU32BE(buf[0..], 9, 0);
    let pos: usize = 13;

    // Timestamp > 24-bit max: 0x02_12_34_56 (34,747,478 ms) + filter bit 0x20
    const ext_ts: u32 = 0x0212_3456;
    pos = appendFlvTag(buf[0..], pos, FLV_TAG_VIDEO | 0x20, ext_ts, "FRAME");

    let it = FlvTagIter.init(buf[0..pos]);
    const tag = it.next() ?? unreachable;
    expect_eq(tag.tag_type == FlvTagType.video, true);
    expect_eq(tag.is_filtered, true);
    expect_eq(tag.timestamp_ms, ext_ts);
    expect_eq(std.mem.eql(u8, tag.payload, "FRAME"), true);
});

test("3. parseFlvAudioHeader decodes AAC and MP3 audio metadata", () => {
    // AAC (10), 44100 Hz (3), 16-bit (1), stereo (1) -> (10<<4)|(3<<2)|(1<<1)|1 = 0xAF, packet_type = 1 (raw)
    const aac: [2]u8 = [0xAF, 0x01];
    const a1 = parseFlvAudioHeader(aac[0..]) ?? unreachable;
    expect_eq(a1.sound_format, 10);
    expect_eq(a1.sample_rate_hz, 44100);
    expect_eq(a1.is_16bit, true);
    expect_eq(a1.is_stereo, true);
    expect_eq(a1.aac_packet_type ?? 99, 1);

    // MP3 (2), 22050 Hz (2), 8-bit (0), mono (0) -> (2<<4)|(2<<2) = 0x28
    const mp3: [1]u8 = [0x28];
    const a2 = parseFlvAudioHeader(mp3[0..]) ?? unreachable;
    expect_eq(a2.sound_format, 2);
    expect_eq(a2.sample_rate_hz, 22050);
    expect_eq(a2.is_16bit, false);
    expect_eq(a2.is_stereo, false);
    expect_eq(a2.aac_packet_type == null, true);

    // Truncated AAC header (missing packet_type byte) rejected
    const bad_aac: [1]u8 = [0xAF];
    expect_eq(parseFlvAudioHeader(bad_aac[0..]) == null, true);
    expect_eq(parseFlvAudioHeader(aac[0..0]) == null, true);
});

test("4. parseFlvVideoHeader decodes AVC/HEVC keyframes, inter frames, and signed 24-bit CTS", () => {
    // Keyframe (1), AVC (7) -> 0x17, packet_type=1 (NALU), cts = +80ms (0x000050)
    const avc_pos: [5]u8 = [0x17, 0x01, 0x00, 0x00, 0x50];
    const v1 = parseFlvVideoHeader(avc_pos[0..]) ?? unreachable;
    expect_eq(v1.isKeyframe(), true);
    expect_eq(v1.codec_id, 7);
    expect_eq(v1.avc_packet_type ?? 99, 1);
    expect_eq(v1.composition_time_ms, 80);

    // Inter frame (2), HEVC (12) -> 0x2C, packet_type=1, cts = -40ms (24-bit two's complement: 0xFFFFD8)
    const hevc_neg: [5]u8 = [0x2C, 0x01, 0xFF, 0xFF, 0xD8];
    const v2 = parseFlvVideoHeader(hevc_neg[0..]) ?? unreachable;
    expect_eq(v2.isKeyframe(), false);
    expect_eq(v2.frame_type, 2);
    expect_eq(v2.codec_id, 12);
    expect_eq(v2.composition_time_ms, -40);

    // Truncated AVC header (< 5 bytes) rejected
    expect_eq(parseFlvVideoHeader(avc_pos[0..4]) == null, true);
});

test("5. Malformed magic, bad data_offset, mismatched PreviousTagSize, and truncated tags fail safely", () => {
    let buf: [48]u8 = std.mem.zeroes([48]u8);
    expect_eq(isFlv(buf[0..8]), false);
    expect_eq(parseFlvHeader(buf[0..]) == null, true);

    // data_offset < 9 rejected
    writeFlvHeader(buf[0..], true, false, 8);
    expect_eq(isFlv(buf[0..]), false);

    // data_offset > buffer rejected
    writeFlvHeader(buf[0..], true, false, 100);
    expect_eq(isFlv(buf[0..]), false);

    // Valid header + tag, then corrupt trailing PreviousTagSize
    writeFlvHeader(buf[0..], true, false, 9);
    writeU32BE(buf[0..], 9, 0);
    const end_pos = appendFlvTag(buf[0..], 13, FLV_TAG_SCRIPT, 0, "OK");
    // Corrupt PreviousTagSize (last 4 bytes)
    writeU32BE(buf[0..], end_pos - 4, 999);
    let it_bad_prev = FlvTagIter.init(buf[0..end_pos]);
    expect_eq(it_bad_prev.next() == null, true);

    // Truncated tag buffer (missing trailing PreviousTagSize)
    let it_trunc = FlvTagIter.init(buf[0..(end_pos - 2)]);
    expect_eq(it_trunc.next() == null, true);
});
