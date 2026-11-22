// Source: cp/cap/test/test_mkv_production_deep.cp (Production deep tests for cap:mkv)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ID_EBML,
    ID_DOC_TYPE,
    ID_SEGMENT,
    ID_INFO,
    ID_TIMECODE_SCALE,
    ID_DURATION,
    ID_MUXING_APP,
    ID_WRITING_APP,
    ID_TRACKS,
    ID_TRACK_ENTRY,
    ID_TRACK_NUMBER,
    ID_TRACK_TYPE,
    ID_CODEC_ID,
    ID_VIDEO,
    ID_PIXEL_WIDTH,
    ID_PIXEL_HEIGHT,
    ID_AUDIO,
    ID_SAMPLING_FREQ,
    ID_CHANNELS,
    ID_CLUSTER,
    MkvTrackType,
    MkvInfo,
    MkvTrack,
    MkvTrackIter,
    MkvHeader,
    isMkv,
    isWebm,
    parseMkvHeader,
} from "cap:media";

fn writeId(buf: []u8, pos: usize, id: u32): usize {
    if (id <= 0xFF) {
        buf[pos] = id as u8;
        return pos + 1;
    }
    if (id <= 0xFFFF) {
        buf[pos] = ((id >> 8) & 0xFF) as u8;
        buf[pos + 1] = (id & 0xFF) as u8;
        return pos + 2;
    }
    if (id <= 0xFFFFFF) {
        buf[pos] = ((id >> 16) & 0xFF) as u8;
        buf[pos + 1] = ((id >> 8) & 0xFF) as u8;
        buf[pos + 2] = (id & 0xFF) as u8;
        return pos + 3;
    }
    buf[pos] = ((id >> 24) & 0xFF) as u8;
    buf[pos + 1] = ((id >> 16) & 0xFF) as u8;
    buf[pos + 2] = ((id >> 8) & 0xFF) as u8;
    buf[pos + 3] = (id & 0xFF) as u8;
    return pos + 4;
}

fn writeSize1(buf: []u8, pos: usize, len: usize): usize {
    buf[pos] = (0x80 as u8) | ((len & 0x7F) as u8);
    return pos + 1;
}

fn writeSize2(buf: []u8, pos: usize, len: usize): usize {
    buf[pos] = (0x40 as u8) | (((len >> 8) & 0x3F) as u8);
    buf[pos + 1] = (len & 0xFF) as u8;
    return pos + 2;
}

fn writeBytes(buf: []u8, pos: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        buf[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeElemStr(buf: []u8, pos: usize, id: u32, s: []const u8): usize {
    let p = writeId(buf, pos, id);
    p = writeSize1(buf, p, s.len);
    return writeBytes(buf, p, s);
}

fn writeElemU16(buf: []u8, pos: usize, id: u32, val: u16): usize {
    let p = writeId(buf, pos, id);
    p = writeSize1(buf, p, 2);
    buf[p] = ((val >> 8) & 0xFF) as u8;
    buf[p + 1] = (val & 0xFF) as u8;
    return p + 2;
}

fn writeElemU8(buf: []u8, pos: usize, id: u32, val: u8): usize {
    let p = writeId(buf, pos, id);
    p = writeSize1(buf, p, 1);
    buf[p] = val;
    return p + 1;
}

fn writeElemU32(buf: []u8, pos: usize, id: u32, val: u32): usize {
    let p = writeId(buf, pos, id);
    p = writeSize1(buf, p, 4);
    buf[p] = ((val >> 24) & 0xFF) as u8;
    buf[p + 1] = ((val >> 16) & 0xFF) as u8;
    buf[p + 2] = ((val >> 8) & 0xFF) as u8;
    buf[p + 3] = (val & 0xFF) as u8;
    return p + 4;
}

fn writeElemF32(buf: []u8, pos: usize, id: u32, val: f32): usize {
    let p = writeId(buf, pos, id);
    p = writeSize1(buf, p, 4);
    let bits: u32 = 0;
    unsafe {
        bits = @bitCast(val);
    }
    buf[p] = ((bits >> 24) & 0xFF) as u8;
    buf[p + 1] = ((bits >> 16) & 0xFF) as u8;
    buf[p + 2] = ((bits >> 8) & 0xFF) as u8;
    buf[p + 3] = (bits & 0xFF) as u8;
    return p + 4;
}

fn writeElemF64(buf: []u8, pos: usize, id: u32, val: f64): usize {
    let p = writeId(buf, pos, id);
    p = writeSize1(buf, p, 8);
    let bits: u64 = 0;
    unsafe {
        bits = @bitCast(val);
    }
    buf[p] = ((bits >> 56) & 0xFF) as u8;
    buf[p + 1] = ((bits >> 48) & 0xFF) as u8;
    buf[p + 2] = ((bits >> 40) & 0xFF) as u8;
    buf[p + 3] = ((bits >> 32) & 0xFF) as u8;
    buf[p + 4] = ((bits >> 24) & 0xFF) as u8;
    buf[p + 5] = ((bits >> 16) & 0xFF) as u8;
    buf[p + 6] = ((bits >> 8) & 0xFF) as u8;
    buf[p + 7] = (bits & 0xFF) as u8;
    return p + 8;
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Full Matroska container parses Info + Video, Audio, and Subtitle TrackEntries", () => {
    let buf: [512]u8 = std.mem.zeroes([512]u8);
    let p: usize = 0;

    // EBML header with DocType="matroska"
    let ebml_body: [32]u8 = std.mem.zeroes([32]u8);
    const ebml_len = writeElemStr(ebml_body[0..], 0, ID_DOC_TYPE, "matroska");
    p = writeId(buf[0..], p, ID_EBML);
    p = writeSize1(buf[0..], p, ebml_len);
    p = writeBytes(buf[0..], p, ebml_body[0..ebml_len]);

    // Info element: TimecodeScale=1_000_000, Duration=12500.0 (f64), MuxingApp, WritingApp
    let info_body: [96]u8 = std.mem.zeroes([96]u8);
    let ip: usize = 0;
    ip = writeElemU32(info_body[0..], ip, ID_TIMECODE_SCALE, 1_000_000);
    ip = writeElemF64(info_body[0..], ip, ID_DURATION, 12500.0);
    ip = writeElemStr(info_body[0..], ip, ID_MUXING_APP, "libebml");
    ip = writeElemStr(info_body[0..], ip, ID_WRITING_APP, "cap-mkv");

    // Track 1: Video 1920x1080 V_MPEG4/ISO/AVC
    let vid_sub: [32]u8 = std.mem.zeroes([32]u8);
    let vp: usize = 0;
    vp = writeElemU16(vid_sub[0..], vp, ID_PIXEL_WIDTH, 1920);
    vp = writeElemU16(vid_sub[0..], vp, ID_PIXEL_HEIGHT, 1080);

    let t1_body: [64]u8 = std.mem.zeroes([64]u8);
    let t1p: usize = 0;
    t1p = writeElemU8(t1_body[0..], t1p, ID_TRACK_NUMBER, 1);
    t1p = writeElemU8(t1_body[0..], t1p, ID_TRACK_TYPE, 1);
    t1p = writeElemStr(t1_body[0..], t1p, ID_CODEC_ID, "V_MPEG4/ISO/AVC");
    t1p = writeId(t1_body[0..], t1p, ID_VIDEO);
    t1p = writeSize1(t1_body[0..], t1p, vp);
    t1p = writeBytes(t1_body[0..], t1p, vid_sub[0..vp]);

    // Track 2: Audio 48000Hz 2ch A_AAC
    let aud_sub: [32]u8 = std.mem.zeroes([32]u8);
    let ap: usize = 0;
    ap = writeElemU8(aud_sub[0..], ap, ID_CHANNELS, 2);
    ap = writeElemF32(aud_sub[0..], ap, ID_SAMPLING_FREQ, 48000.0);

    let t2_body: [64]u8 = std.mem.zeroes([64]u8);
    let t2p: usize = 0;
    t2p = writeElemU8(t2_body[0..], t2p, ID_TRACK_NUMBER, 2);
    t2p = writeElemU8(t2_body[0..], t2p, ID_TRACK_TYPE, 2);
    t2p = writeElemStr(t2_body[0..], t2p, ID_CODEC_ID, "A_AAC");
    t2p = writeId(t2_body[0..], t2p, ID_AUDIO);
    t2p = writeSize1(t2_body[0..], t2p, ap);
    t2p = writeBytes(t2_body[0..], t2p, aud_sub[0..ap]);

    // Track 3: Subtitle S_TEXT/UTF8
    let t3_body: [48]u8 = std.mem.zeroes([48]u8);
    let t3p: usize = 0;
    t3p = writeElemU8(t3_body[0..], t3p, ID_TRACK_NUMBER, 3);
    t3p = writeElemU8(t3_body[0..], t3p, ID_TRACK_TYPE, 17);
    t3p = writeElemStr(t3_body[0..], t3p, ID_CODEC_ID, "S_TEXT/UTF8");

    // Combine Tracks body
    let tracks_body: [256]u8 = std.mem.zeroes([256]u8);
    let trp: usize = 0;
    trp = writeId(tracks_body[0..], trp, ID_TRACK_ENTRY);
    trp = writeSize1(tracks_body[0..], trp, t1p);
    trp = writeBytes(tracks_body[0..], trp, t1_body[0..t1p]);
    trp = writeId(tracks_body[0..], trp, ID_TRACK_ENTRY);
    trp = writeSize1(tracks_body[0..], trp, t2p);
    trp = writeBytes(tracks_body[0..], trp, t2_body[0..t2p]);
    trp = writeId(tracks_body[0..], trp, ID_TRACK_ENTRY);
    trp = writeSize1(tracks_body[0..], trp, t3p);
    trp = writeBytes(tracks_body[0..], trp, t3_body[0..t3p]);

    // Segment body = Info + Tracks
    let seg_body: [384]u8 = std.mem.zeroes([384]u8);
    let sp: usize = 0;
    sp = writeId(seg_body[0..], sp, ID_INFO);
    sp = writeSize1(seg_body[0..], sp, ip);
    sp = writeBytes(seg_body[0..], sp, info_body[0..ip]);
    sp = writeId(seg_body[0..], sp, ID_TRACKS);
    sp = writeSize2(seg_body[0..], sp, trp);
    sp = writeBytes(seg_body[0..], sp, tracks_body[0..trp]);

    p = writeId(buf[0..], p, ID_SEGMENT);
    p = writeSize2(buf[0..], p, sp);
    p = writeBytes(buf[0..], p, seg_body[0..sp]);

    expect_eq(isMkv(buf[0..p]), true);
    expect_eq(isWebm(buf[0..p]), false);

    const hdr = parseMkvHeader(buf[0..p]) ?? unreachable;
    expect_eq(hdr.has_info, true);
    expect_eq(hdr.has_tracks, true);
    expect_eq(sliceEq(hdr.info.doc_type, "matroska"), true);
    expect_eq(hdr.info.isWebm(), false);
    expect_eq(hdr.info.timecode_scale_ns, 1_000_000);
    expect_eq(hdr.info.durationMs() > 12499.9 && hdr.info.durationMs() < 12500.1, true);
    expect_eq(sliceEq(hdr.info.muxing_app, "libebml"), true);
    expect_eq(sliceEq(hdr.info.writing_app, "cap-mkv"), true);

    let it = hdr.tracks();
    const tr1 = it.next() ?? unreachable;
    expect_eq(tr1.track_number, 1);
    expect_eq(tr1.track_type, MkvTrackType.video);
    expect_eq(sliceEq(tr1.codec_id, "V_MPEG4/ISO/AVC"), true);
    expect_eq(tr1.pixel_width, 1920);
    expect_eq(tr1.pixel_height, 1080);

    const tr2 = it.next() ?? unreachable;
    expect_eq(tr2.track_number, 2);
    expect_eq(tr2.track_type, MkvTrackType.audio);
    expect_eq(sliceEq(tr2.codec_id, "A_AAC"), true);
    expect_eq(tr2.channels, 2);
    expect_eq(tr2.sampling_frequency > 47999.9 && tr2.sampling_frequency < 48000.1, true);

    const tr3 = it.next() ?? unreachable;
    expect_eq(tr3.track_number, 3);
    expect_eq(tr3.track_type, MkvTrackType.subtitle);
    expect_eq(sliceEq(tr3.codec_id, "S_TEXT/UTF8"), true);

    expect_eq(it.next() == null, true);
});

test("2. WebM container with DocType=webm, VP9 + Opus tracks, and custom TimecodeScale", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let p: usize = 0;

    let ebml_body: [16]u8 = std.mem.zeroes([16]u8);
    const ebml_len = writeElemStr(ebml_body[0..], 0, ID_DOC_TYPE, "webm");
    p = writeId(buf[0..], p, ID_EBML);
    p = writeSize1(buf[0..], p, ebml_len);
    p = writeBytes(buf[0..], p, ebml_body[0..ebml_len]);

    // TimecodeScale = 500_000 ns (0.5 ms per tick), Duration = 2000 ticks -> 1000.0 ms
    let info_body: [32]u8 = std.mem.zeroes([32]u8);
    let ip: usize = 0;
    ip = writeElemU32(info_body[0..], ip, ID_TIMECODE_SCALE, 500_000);
    ip = writeElemF32(info_body[0..], ip, ID_DURATION, 2000.0);

    let t1_body: [32]u8 = std.mem.zeroes([32]u8);
    let t1p: usize = 0;
    t1p = writeElemU8(t1_body[0..], t1p, ID_TRACK_NUMBER, 1);
    t1p = writeElemU8(t1_body[0..], t1p, ID_TRACK_TYPE, 1);
    t1p = writeElemStr(t1_body[0..], t1p, ID_CODEC_ID, "V_VP9");

    let t2_body: [32]u8 = std.mem.zeroes([32]u8);
    let t2p: usize = 0;
    t2p = writeElemU8(t2_body[0..], t2p, ID_TRACK_NUMBER, 2);
    t2p = writeElemU8(t2_body[0..], t2p, ID_TRACK_TYPE, 2);
    t2p = writeElemStr(t2_body[0..], t2p, ID_CODEC_ID, "A_OPUS");

    let tracks_body: [96]u8 = std.mem.zeroes([96]u8);
    let trp: usize = 0;
    trp = writeId(tracks_body[0..], trp, ID_TRACK_ENTRY);
    trp = writeSize1(tracks_body[0..], trp, t1p);
    trp = writeBytes(tracks_body[0..], trp, t1_body[0..t1p]);
    trp = writeId(tracks_body[0..], trp, ID_TRACK_ENTRY);
    trp = writeSize1(tracks_body[0..], trp, t2p);
    trp = writeBytes(tracks_body[0..], trp, t2_body[0..t2p]);

    let seg_body: [160]u8 = std.mem.zeroes([160]u8);
    let sp: usize = 0;
    sp = writeId(seg_body[0..], sp, ID_INFO);
    sp = writeSize1(seg_body[0..], sp, ip);
    sp = writeBytes(seg_body[0..], sp, info_body[0..ip]);
    sp = writeId(seg_body[0..], sp, ID_TRACKS);
    sp = writeSize1(seg_body[0..], sp, trp);
    sp = writeBytes(seg_body[0..], sp, tracks_body[0..trp]);

    p = writeId(buf[0..], p, ID_SEGMENT);
    p = writeSize2(buf[0..], p, sp);
    p = writeBytes(buf[0..], p, seg_body[0..sp]);

    expect_eq(isMkv(buf[0..p]), true);
    expect_eq(isWebm(buf[0..p]), true);

    const hdr = parseMkvHeader(buf[0..p]) ?? unreachable;
    expect_eq(hdr.info.isWebm(), true);
    expect_eq(hdr.info.timecode_scale_ns, 500_000);
    expect_eq(hdr.info.durationMs() > 999.9 && hdr.info.durationMs() < 1000.1, true);

    let it = hdr.tracks();
    const v = it.next() ?? unreachable;
    expect_eq(sliceEq(v.codec_id, "V_VP9"), true);
    const a = it.next() ?? unreachable;
    expect_eq(sliceEq(a.codec_id, "A_OPUS"), true);
    expect_eq(it.next() == null, true);
});

test("3. Streaming Segment with unknown size (0x01FFFFFFFFFFFFFF) stops cleanly at Cluster", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let p: usize = 0;

    let ebml_body: [16]u8 = std.mem.zeroes([16]u8);
    const ebml_len = writeElemStr(ebml_body[0..], 0, ID_DOC_TYPE, "webm");
    p = writeId(buf[0..], p, ID_EBML);
    p = writeSize1(buf[0..], p, ebml_len);
    p = writeBytes(buf[0..], p, ebml_body[0..ebml_len]);

    // Segment with 8-byte unknown size 0x01FFFFFFFFFFFFFF
    p = writeId(buf[0..], p, ID_SEGMENT);
    buf[p] = 0x01;
    buf[p + 1] = 0xFF;
    buf[p + 2] = 0xFF;
    buf[p + 3] = 0xFF;
    buf[p + 4] = 0xFF;
    buf[p + 5] = 0xFF;
    buf[p + 6] = 0xFF;
    buf[p + 7] = 0xFF;
    p += 8;

    // Info with default TimecodeScale (omitted => 1_000_000) and MuxingApp="live"
    let info_body: [16]u8 = std.mem.zeroes([16]u8);
    const ip = writeElemStr(info_body[0..], 0, ID_MUXING_APP, "live");
    p = writeId(buf[0..], p, ID_INFO);
    p = writeSize1(buf[0..], p, ip);
    p = writeBytes(buf[0..], p, info_body[0..ip]);

    // Cluster with unknown size immediately after Info
    p = writeId(buf[0..], p, ID_CLUSTER);
    buf[p] = 0xFF; // 1-byte unknown size
    p += 1;
    buf[p] = 0xAA;
    buf[p + 1] = 0xBB;
    p += 2;

    const hdr = parseMkvHeader(buf[0..p]) ?? unreachable;
    expect_eq(hdr.has_info, true);
    expect_eq(hdr.info.timecode_scale_ns, 1_000_000);
    expect_eq(sliceEq(hdr.info.muxing_app, "live"), true);
    expect_eq(hdr.has_tracks, false);
    let it = hdr.tracks();
    expect_eq(it.next() == null, true);
});

test("4. 32-bit and 64-bit IEEE-754 float decoding for Duration and SamplingFrequency", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let p: usize = 0;

    let ebml_body: [16]u8 = std.mem.zeroes([16]u8);
    const ebml_len = writeElemStr(ebml_body[0..], 0, ID_DOC_TYPE, "matroska");
    p = writeId(buf[0..], p, ID_EBML);
    p = writeSize1(buf[0..], p, ebml_len);
    p = writeBytes(buf[0..], p, ebml_body[0..ebml_len]);

    // 32-bit float Duration = 3456.5
    let info_body: [16]u8 = std.mem.zeroes([16]u8);
    const ip = writeElemF32(info_body[0..], 0, ID_DURATION, 3456.5);

    // 64-bit float SamplingFrequency = 44100.0
    let aud_sub: [24]u8 = std.mem.zeroes([24]u8);
    let ap: usize = 0;
    ap = writeElemF64(aud_sub[0..], ap, ID_SAMPLING_FREQ, 44100.0);
    ap = writeElemU8(aud_sub[0..], ap, ID_CHANNELS, 6);

    let t_body: [48]u8 = std.mem.zeroes([48]u8);
    let tp: usize = 0;
    tp = writeElemU8(t_body[0..], tp, ID_TRACK_NUMBER, 1);
    tp = writeElemU8(t_body[0..], tp, ID_TRACK_TYPE, 2);
    tp = writeId(t_body[0..], tp, ID_AUDIO);
    tp = writeSize1(t_body[0..], tp, ap);
    tp = writeBytes(t_body[0..], tp, aud_sub[0..ap]);

    let tracks_body: [64]u8 = std.mem.zeroes([64]u8);
    let trp: usize = 0;
    trp = writeId(tracks_body[0..], trp, ID_TRACK_ENTRY);
    trp = writeSize1(tracks_body[0..], trp, tp);
    trp = writeBytes(tracks_body[0..], trp, t_body[0..tp]);

    let seg_body: [128]u8 = std.mem.zeroes([128]u8);
    let sp: usize = 0;
    sp = writeId(seg_body[0..], sp, ID_INFO);
    sp = writeSize1(seg_body[0..], sp, ip);
    sp = writeBytes(seg_body[0..], sp, info_body[0..ip]);
    sp = writeId(seg_body[0..], sp, ID_TRACKS);
    sp = writeSize1(seg_body[0..], sp, trp);
    sp = writeBytes(seg_body[0..], sp, tracks_body[0..trp]);

    p = writeId(buf[0..], p, ID_SEGMENT);
    p = writeSize1(buf[0..], p, sp);
    p = writeBytes(buf[0..], p, seg_body[0..sp]);

    const hdr = parseMkvHeader(buf[0..p]) ?? unreachable;
    expect_eq(hdr.info.duration_ticks > 3456.4 && hdr.info.duration_ticks < 3456.6, true);
    let it = hdr.tracks();
    const tr = it.next() ?? unreachable;
    expect_eq(tr.channels, 6);
    expect_eq(tr.sampling_frequency > 44099.9 && tr.sampling_frequency < 44100.1, true);
});

test("5. Malformed EBML protection: non-EBML magic, wrong DocType, zero VINT, and out-of-bounds size", () => {
    // Empty and non-EBML magic
    let bad: [16]u8 = std.mem.zeroes([16]u8);
    expect_eq(isMkv(bad[0..0]), false);
    expect_eq(isMkv(bad[0..16]), false);
    expect_eq(parseMkvHeader(bad[0..16]) == null, true);

    // Valid EBML magic but unknown DocType ("custom")
    let wrong_dt: [32]u8 = std.mem.zeroes([32]u8);
    let dt_body: [16]u8 = std.mem.zeroes([16]u8);
    const dt_len = writeElemStr(dt_body[0..], 0, ID_DOC_TYPE, "custom");
    let wp = writeId(wrong_dt[0..], 0, ID_EBML);
    wp = writeSize1(wrong_dt[0..], wp, dt_len);
    wp = writeBytes(wrong_dt[0..], wp, dt_body[0..dt_len]);
    expect_eq(isMkv(wrong_dt[0..wp]), false);
    expect_eq(parseMkvHeader(wrong_dt[0..wp]) == null, true);

    // Out-of-bounds VINT size (EBML claims 60 bytes on a 10-byte buffer)
    let oob: [10]u8 = std.mem.zeroes([10]u8);
    let op = writeId(oob[0..], 0, ID_EBML);
    op = writeSize1(oob[0..], op, 60);
    expect_eq(isMkv(oob[0..]), false);
    expect_eq(parseMkvHeader(oob[0..]) == null, true);

    // Zero byte VINT (0x00) in size position rejected
    let zero_vint: [10]u8 = std.mem.zeroes([10]u8);
    * = writeId(zero_vint[0..], 0, ID_EBML);
    zero_vint[4] = 0x00;
    expect_eq(isMkv(zero_vint[0..]), false);
});
