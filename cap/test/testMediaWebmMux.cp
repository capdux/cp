// Origin: Pure-CP Stdlib Test Suite (lib/media/webmMux.cp) | Created: 2026-11-23
// Production Deep Verification Suite for WebM/EBML Streaming Muxer/Demuxer, VP8/VP9/AV1 OBU Parsers & RFC 7741 VP8 RTP:
//   1. RFC 8794 EBML VINT 1..8-byte roundtrip, unknown-size sentinel (0x01FFFFFFFFFFFFFF) & 1..4B Element ID codecs
//   2. Multi-track VP9 Video (1280x720) + Opus Audio (48kHz 2ch OpusHead) WebM mux -> demux frame-by-frame roundtrip
//   3. Live streaming unknown-size Segment & Cluster WebM mux -> demux multi-cluster roundtrip
//   4. RFC 6386 VP8 keyframe (0x9D 0x01 0x2A sync, 1280x720 + scale) & interframe header parsing
//   5. VP9 keyframe (profile 0 & profile 2 10-bit, 0x49 0x83 0x42 sync, 1920x1080) & show_existing_frame parsing
//   6. AOMedia AV1 OBU LEB128 + Av1ObuIter (TD + SeqHdr + Frame w/ extension) & 1920x1080 Sequence Header decoding
//   7. RFC 7741 VP8 RTP extended descriptor (X/I/L/T/K), MTU packetization, loss recovery & 16-bit seq wrap-around
//   8. Malformed/truncated/boundary guard matrix & cap.media namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    WEBM_EBML_UNKNOWN_SIZE,
    WEBM_EBML_UNKNOWN_SIZE_RAW,
    WEBM_DEFAULT_TIMECODE_SCALE_NS,
    WEBM_ID_EBML,
    WEBM_ID_SEGMENT,
    WEBM_ID_CLUSTER,
    WEBM_ID_SIMPLE_BLOCK,
    WEBM_TRACK_TYPE_VIDEO,
    WEBM_TRACK_TYPE_AUDIO,
    VP8_KEYFRAME_SYNC_0,
    VP8_KEYFRAME_SYNC_1,
    VP8_KEYFRAME_SYNC_2,
    VP9_KEYFRAME_SYNC_0,
    VP9_KEYFRAME_SYNC_1,
    VP9_KEYFRAME_SYNC_2,
    AV1_OBU_SEQUENCE_HEADER,
    AV1_OBU_TEMPORAL_DELIMITER,
    AV1_OBU_FRAME,
    WebmMuxError,
    ebmlVintWidth,
    encodeEbmlVintFixed,
    encodeEbmlVint,
    decodeEbmlVint,
    encodeEbmlElementId,
    decodeEbmlElementId,
    WebmVideoCodec,
    WebmTrackConfig,
    WebmMuxer,
    WebmDemuxTrackInfo,
    WebmDemuxIter,
    parseVp8FrameHeader,
    parseVp9FrameHeader,
    decodeAv1Leb128,
    encodeAv1Leb128,
    Av1ObuIter,
    parseAv1SequenceHeaderBasic,
    Vp8RtpDescriptor,
    encodeVp8RtpDescriptor,
    decodeVp8RtpDescriptor,
    Vp8RtpPacketSpan,
    packetizeVp8Rtp,
    Vp8DepacketizeOutcome,
    Vp8RtpDepacketizer,
} from "cap:media";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("EBML VINT 1..8-byte roundtrip, unknown-size sentinel 0x01FFFFFFFFFFFFFF & Element IDs") {
    const vals: [8]u64 = [
        0,
        0x7E,
        0x7F,
        0x3FFE,
        0x1FFFFE,
        0x0FFFFFFE,
        0x07FFFFFFFFE,
        0x00FFFFFFFFFFFFFE,
    ];
    const exp_lens: [8]usize = [1, 1, 2, 2, 3, 4, 5, 8];

    let buf: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        expect_eq(ebmlVintWidth(vals[i]) catch unreachable, exp_lens[i]);
        const n = encodeEbmlVint(vals[i], buf[0..]) catch unreachable;
        expect_eq(n, exp_lens[i]);
        const parsed = decodeEbmlVint(buf[0..n]) catch unreachable;
        expect_eq(parsed.value, vals[i]);
        expect_eq(parsed.length, exp_lens[i]);
        expect_true(!parsed.is_unknown);
    }

    // Streaming unknown-size sentinel 0x01FFFFFFFFFFFFFF
    const un = encodeEbmlVint(WEBM_EBML_UNKNOWN_SIZE, buf[0..]) catch unreachable;
    expect_eq(un, 8);
    expect_eq(buf[0], 0x01);
    expect_eq(buf[7], 0xFF);
    const up = decodeEbmlVint(buf[0..8]) catch unreachable;
    expect_true(up.is_unknown);
    expect_eq(up.value, WEBM_EBML_UNKNOWN_SIZE);
    expect_eq(up.length, 8);
    expect_eq(WEBM_EBML_UNKNOWN_SIZE_RAW, 0x01FFFFFFFFFFFFFF);

    // Fixed-width VINT encoding (e.g., 4-byte padded value 42)
    const fn4 = encodeEbmlVintFixed(42, 4, buf[0..]) catch unreachable;
    expect_eq(fn4, 4);
    const fp4 = decodeEbmlVint(buf[0..4]) catch unreachable;
    expect_eq(fp4.value, 42);
    expect_eq(fp4.length, 4);

    // 1..4-byte EBML Element IDs
    const ids: [4]u32 = [WEBM_ID_SIMPLE_BLOCK, 0x4282, 0x2AD7B1, WEBM_ID_EBML];
    const id_lens: [4]usize = [1, 2, 3, 4];
    let j: usize = 0;
    while (j < 4) : (j += 1) {
        const ln = encodeEbmlElementId(ids[j], buf[0..]) catch unreachable;
        expect_eq(ln, id_lens[j]);
        const did = decodeEbmlElementId(buf[0..ln]) catch unreachable;
        expect_eq(did.id, ids[j]);
        expect_eq(did.length, id_lens[j]);
    }
}

test("Multi-track VP9 Video + Opus Audio WebM mux -> demux frame-by-frame closed loop") {
    // 19-byte RFC 7845 OpusHead CodecPrivate
    const opus_head: [19]u8 = [
        0x4F, 0x70, 0x75, 0x73, 0x48, 0x65, 0x61, 0x64, // "OpusHead"
        0x01, 0x02, 0x38, 0x01, 0x80, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00,
    ];
    let wire: [4096]u8 = [0; 4096];
    let mux = WebmMuxer.init(wire[0..], WEBM_DEFAULT_TIMECODE_SCALE_NS, false) catch unreachable;
    const tracks: [2]WebmTrackConfig = [
        WebmTrackConfig.video(1, WebmVideoCodec.vp9, 1280, 720),
        WebmTrackConfig.opusAudio(2, 48000.0, 2, opus_head[0..]),
    ];
    * = mux.writeHeader(tracks[0..], "cp-webm-mux", "cap:media") catch unreachable;

    // Interleaved video & audio frames across 2 automatic clusters (jump > 32767ms forces cluster 2)
    * = mux.writeSimpleBlock(1, 0, true, false, false, "vp9-keyframe-0") catch unreachable;
    * = mux.writeSimpleBlock(2, 20, true, false, false, "opus-pkt-20ms") catch unreachable;
    * = mux.writeSimpleBlock(1, 33, false, false, true, "vp9-delta-33ms") catch unreachable;
    * = mux.writeSimpleBlock(1, 35000, true, true, false, "vp9-key-35s-altref") catch unreachable;
    * = mux.writeSimpleBlock(2, 35020, true, false, false, "opus-pkt-35020ms") catch unreachable;
    const total_len = mux.finish() catch unreachable;
    expect_eq(mux.blocks_written, 5);
    expect_eq(mux.clusters_written, 2);

    // Demux back & verify every track field and every frame byte-for-byte
    let out_tracks: [4]WebmDemuxTrackInfo = [WebmDemuxTrackInfo{}; 4];
    let demux = WebmDemuxIter.init(wire[0..total_len], out_tracks[0..]) catch unreachable;
    expect_true(bytesEq(demux.doc_type, "webm"));
    expect_eq(demux.timecode_scale_ns, 1000000);
    expect_true(bytesEq(demux.muxing_app, "cp-webm-mux"));
    expect_true(bytesEq(demux.writing_app, "cap:media"));
    expect_eq(demux.track_count, 2);

    expect_eq(out_tracks[0].track_number, 1);
    expect_eq(out_tracks[0].track_type, WEBM_TRACK_TYPE_VIDEO);
    expect_true(bytesEq(out_tracks[0].codec_id, "V_VP9"));
    expect_eq(out_tracks[0].pixel_width, 1280);
    expect_eq(out_tracks[0].pixel_height, 720);

    expect_eq(out_tracks[1].track_number, 2);
    expect_eq(out_tracks[1].track_type, WEBM_TRACK_TYPE_AUDIO);
    expect_true(bytesEq(out_tracks[1].codec_id, "A_OPUS"));
    expect_eq(out_tracks[1].sampling_frequency, 48000.0);
    expect_eq(out_tracks[1].channels, 2);
    expect_true(bytesEq(out_tracks[1].codec_private, opus_head[0..]));

    const f0 = (demux.nextFrame() catch unreachable) orelse unreachable;
    expect_eq(f0.track_number, 1);
    expect_eq(f0.timestamp_ms, 0);
    expect_true(f0.is_keyframe);
    expect_true(!f0.is_invisible);
    expect_true(bytesEq(f0.payload, "vp9-keyframe-0"));

    const f1 = (demux.nextFrame() catch unreachable) orelse unreachable;
    expect_eq(f1.track_number, 2);
    expect_eq(f1.timestamp_ms, 20);
    expect_true(bytesEq(f1.payload, "opus-pkt-20ms"));

    const f2 = (demux.nextFrame() catch unreachable) orelse unreachable;
    expect_eq(f2.track_number, 1);
    expect_eq(f2.timestamp_ms, 33);
    expect_true(!f2.is_keyframe);
    expect_true(f2.is_discardable);
    expect_true(bytesEq(f2.payload, "vp9-delta-33ms"));

    const f3 = (demux.nextFrame() catch unreachable) orelse unreachable;
    expect_eq(f3.track_number, 1);
    expect_eq(f3.timestamp_ms, 35000);
    expect_true(f3.is_keyframe);
    expect_true(f3.is_invisible);
    expect_true(bytesEq(f3.payload, "vp9-key-35s-altref"));

    const f4 = (demux.nextFrame() catch unreachable) orelse unreachable;
    expect_eq(f4.track_number, 2);
    expect_eq(f4.timestamp_ms, 35020);
    expect_true(bytesEq(f4.payload, "opus-pkt-35020ms"));

    expect_true((demux.nextFrame() catch unreachable) == null);
}

test("Live streaming unknown-size Segment & Cluster WebM mux -> demux multi-cluster roundtrip") {
    let wire: [2048]u8 = [0; 2048];
    let mux = WebmMuxer.init(wire[0..], WEBM_DEFAULT_TIMECODE_SCALE_NS, true) catch unreachable;
    const tracks: [1]WebmTrackConfig = [
        WebmTrackConfig.video(1, WebmVideoCodec.av1, 1920, 1080),
    ];
    * = mux.writeHeader(tracks[0..], "live-cp", "live-cp") catch unreachable;
    * = mux.startCluster(1000) catch unreachable;
    * = mux.writeSimpleBlock(1, 1000, true, false, false, "av1-gop1-key") catch unreachable;
    * = mux.writeSimpleBlock(1, 1040, false, false, false, "av1-gop1-p1") catch unreachable;
    * = mux.startCluster(2000) catch unreachable;
    * = mux.writeSimpleBlock(1, 2000, true, false, false, "av1-gop2-key") catch unreachable;
    const total_len = mux.finish() catch unreachable;

    let out_tracks: [2]WebmDemuxTrackInfo = [WebmDemuxTrackInfo{}; 2];
    let demux = WebmDemuxIter.init(wire[0..total_len], out_tracks[0..]) catch unreachable;
    expect_eq(demux.track_count, 1);
    expect_true(bytesEq(out_tracks[0].codec_id, "V_AV1"));
    expect_eq(out_tracks[0].pixel_width, 1920);
    expect_eq(out_tracks[0].pixel_height, 1080);

    const a0 = (demux.nextFrame() catch unreachable) orelse unreachable;
    const a1 = (demux.nextFrame() catch unreachable) orelse unreachable;
    const a2 = (demux.nextFrame() catch unreachable) orelse unreachable;
    expect_true((demux.nextFrame() catch unreachable) == null);
    expect_eq(a0.timestamp_ms, 1000);
    expect_true(bytesEq(a0.payload, "av1-gop1-key"));
    expect_eq(a1.timestamp_ms, 1040);
    expect_true(bytesEq(a1.payload, "av1-gop1-p1"));
    expect_eq(a2.timestamp_ms, 2000);
    expect_true(bytesEq(a2.payload, "av1-gop2-key"));
}

test("RFC 6386 VP8 keyframe (0x9D 0x01 0x2A sync, 1280x720 + scale) & interframe header parsing") {
    // Keyframe: is_keyframe=true (bit0=0), version=2, show_frame=true (bit4=1), first_part_size=300
    // tag = 0 | (2 << 1) | (1 << 4) | (300 << 5) = 0x2594
    // width = 1280 (0x0500) | (h_scale=1 << 14) = 0x4500 -> LE: 0x00, 0x45
    // height = 720 (0x02D0) | (v_scale=2 << 14) = 0x82D0 -> LE: 0xD0, 0x82
    const vp8_key: [12]u8 = [
        0x94, 0x25, 0x00,
        VP8_KEYFRAME_SYNC_0, VP8_KEYFRAME_SYNC_1, VP8_KEYFRAME_SYNC_2,
        0x00, 0x45,
        0xD0, 0x82,
        0xAA, 0xBB,
    ];
    const kh = parseVp8FrameHeader(vp8_key[0..]) catch unreachable;
    expect_true(kh.is_keyframe);
    expect_eq(kh.version, 2);
    expect_true(kh.show_frame);
    expect_eq(kh.first_part_size, 300);
    expect_eq(kh.width, 1280);
    expect_eq(kh.height, 720);
    expect_eq(kh.horizontal_scale, 1);
    expect_eq(kh.vertical_scale, 2);

    // Interframe: bit0=1, version=0, show_frame=1, first_part_size=64 -> tag = 1 | 16 | (64<<5) = 0x0811
    const vp8_inter: [4]u8 = [0x11, 0x08, 0x00, 0xFF];
    const ih = parseVp8FrameHeader(vp8_inter[0..]) catch unreachable;
    expect_true(!ih.is_keyframe);
    expect_true(ih.show_frame);
    expect_eq(ih.first_part_size, 64);
    expect_eq(ih.width, 0);
}

test("VP9 keyframe (0x49 0x83 0x42 sync, 1920x1080, profile 0) & show_existing_frame parsing") {
    // Profile 0, show_existing=0, keyframe=0, show_frame=1, error_resilient=1 -> byte 0 = 0x83
    // Sync = 0x49, 0x83, 0x42
    // Color config: color_space=2 (BT.709), color_range=0 -> 0b0100
    // Frame size: width_m1=1919 (0x077F), height_m1=1079 (0x0437)
    const vp9_key: [10]u8 = [
        0x83,
        VP9_KEYFRAME_SYNC_0, VP9_KEYFRAME_SYNC_1, VP9_KEYFRAME_SYNC_2,
        0x40, 0x77, 0xF0, 0x43, 0x70, 0x00,
    ];
    const h0 = parseVp9FrameHeader(vp9_key[0..]) catch unreachable;
    expect_eq(h0.profile, 0);
    expect_true(h0.is_keyframe);
    expect_true(h0.show_frame);
    expect_true(h0.error_resilient_mode);
    expect_eq(h0.bit_depth, 8);
    expect_eq(h0.color_space, 2);
    expect_true(!h0.color_range);
    expect_eq(h0.subsampling_x, 1);
    expect_eq(h0.subsampling_y, 1);
    expect_eq(h0.width, 1920);
    expect_eq(h0.height, 1080);

    // show_existing_frame=1, frame_to_show_map_idx=5 (0b101):
    // bits: 10 (marker) 0 (plow) 0 (phigh) 1 (show_existing) 101 (idx=5) = 0b1000_1101 = 0x8D
    const vp9_existing: [2]u8 = [0x8D, 0x00];
    const he = parseVp9FrameHeader(vp9_existing[0..]) catch unreachable;
    expect_true(he.show_existing_frame);
    expect_eq(he.frame_to_show_map_idx, 5);
}

test("AOMedia AV1 OBU LEB128, Av1ObuIter & 1920x1080 Sequence Header decoding") {
    // 1. LEB128 roundtrip (1B, 2B, 3B)
    let lbuf: [8]u8 = [0; 8];
    const ln1 = encodeAv1Leb128(300, lbuf[0..]) catch unreachable;
    expect_eq(ln1, 2);
    const ld1 = decodeAv1Leb128(lbuf[0..ln1]) catch unreachable;
    expect_eq(ld1.value, 300);
    expect_eq(ld1.length, 2);

    // 2. Construct multi-OBU AV1 temporal unit:
    //    OBU 1: TEMPORAL_DELIMITER (type=2, has_size=1, size=0) -> [0x12, 0x00]
    //    OBU 2: SEQUENCE_HEADER (type=1, has_size=1, size=8, 1920x1080 payload) -> [0x0A, 0x08, ...]
    //    OBU 3: FRAME (type=6, has_ext=1, has_size=1, tid=2, sid=1 -> ext=0x48, size=3, payload="av1")
    const av1_tu: [18]u8 = [
        0x12, 0x00,
        0x0A, 0x08, 0x00, 0x00, 0x00, 0x42, 0xAB, 0xBF, 0xC3, 0x70,
        0x36, 0x48, 0x03, 0x61, 0x76, 0x31,
    ];
    let it = Av1ObuIter.init(av1_tu[0..]);

    const o1 = (it.next() catch unreachable) orelse unreachable;
    expect_eq(o1.obu_type, AV1_OBU_TEMPORAL_DELIMITER);
    expect_eq(o1.payload.len, 0);

    const o2 = (it.next() catch unreachable) orelse unreachable;
    expect_eq(o2.obu_type, AV1_OBU_SEQUENCE_HEADER);
    expect_eq(o2.payload.len, 8);
    const seq = parseAv1SequenceHeaderBasic(o2.payload) catch unreachable;
    expect_eq(seq.seq_profile, 0);
    expect_true(!seq.still_picture);
    expect_true(!seq.reduced_still_picture_header);
    expect_eq(seq.seq_level_idx_0, 8);
    expect_eq(seq.frame_width_bits_minus_1, 10);
    expect_eq(seq.frame_height_bits_minus_1, 10);
    expect_eq(seq.max_frame_width, 1920);
    expect_eq(seq.max_frame_height, 1080);

    const o3 = (it.next() catch unreachable) orelse unreachable;
    expect_eq(o3.obu_type, AV1_OBU_FRAME);
    expect_true(o3.has_extension);
    expect_eq(o3.temporal_id, 2);
    expect_eq(o3.spatial_id, 1);
    expect_true(bytesEq(o3.payload, "av1"));

    expect_true((it.next() catch unreachable) == null);
}

test("RFC 7741 VP8 RTP extended descriptor, MTU packetization, loss recovery & 16-bit seq wrap") {
    // 1. Full extended descriptor (X=1, N=1, S=1, PID=2, 15-bit PictureID=0x1234, TL0PICIDX=0x55, TID=2, Y=1, KEYIDX=19)
    const full_desc = Vp8RtpDescriptor{
        x: true,
        n: true,
        s: true,
        pid: 2,
        has_pic_id: true,
        is_15bit_pic_id: true,
        pic_id: 0x1234,
        has_tl0picidx: true,
        tl0picidx: 0x55,
        has_tid: true,
        has_keyidx: true,
        tid: 2,
        y: true,
        keyidx: 19,
    };
    let dbuf: [8]u8 = [0; 8];
    const dlen = encodeVp8RtpDescriptor(full_desc, dbuf[0..]) catch unreachable;
    expect_eq(dlen, 6);
    const dp = decodeVp8RtpDescriptor(dbuf[0..dlen]) catch unreachable;
    expect_eq(dp.header_len, 6);
    expect_true(dp.desc.x);
    expect_true(dp.desc.n);
    expect_true(dp.desc.s);
    expect_eq(dp.desc.pid, 2);
    expect_true(dp.desc.has_pic_id);
    expect_true(dp.desc.is_15bit_pic_id);
    expect_eq(dp.desc.pic_id, 0x1234);
    expect_true(dp.desc.has_tl0picidx);
    expect_eq(dp.desc.tl0picidx, 0x55);
    expect_eq(dp.desc.tid, 2);
    expect_true(dp.desc.y);
    expect_eq(dp.desc.keyidx, 19);

    // 2. Packetize 3,500-byte VP8 frame at MTU=1200 -> 3 packets across 16-bit seq wrap (65535 -> 0 -> 1)
    let vp8_frame: [3500]u8 = [0; 3500];
    let fi: usize = 0;
    while (fi < 3500) : (fi += 1) {
        vp8_frame[fi] = ((fi * 7 + 13) & 0xFF) as u8;
    }
    const base_d = Vp8RtpDescriptor{
        x: true,
        has_pic_id: true,
        is_15bit_pic_id: false,
        pic_id: 77,
    };
    let rtp_slab: [4096]u8 = [0; 4096];
    let spans: [8]Vp8RtpPacketSpan = [Vp8RtpPacketSpan{}; 8];
    const npkts = packetizeVp8Rtp(vp8_frame[0..], 1200, base_d, rtp_slab[0..], spans[0..]) catch unreachable;
    expect_eq(npkts, 3);

    // Verify S=1 on packet 0, S=0 on packets 1..2
    const p0_d = decodeVp8RtpDescriptor(spans[0].slice(rtp_slab[0..])) catch unreachable;
    const p1_d = decodeVp8RtpDescriptor(spans[1].slice(rtp_slab[0..])) catch unreachable;
    expect_true(p0_d.desc.s);
    expect_true(!p1_d.desc.s);
    expect_eq(p0_d.desc.pic_id, 77);

    // Reassemble cleanly across 16-bit sequence wrap (65535, 0, 1)
    let out_frame: [4096]u8 = [0; 4096];
    let depkt = Vp8RtpDepacketizer.init(out_frame[0..]) catch unreachable;
    expect_eq(depkt.pushRtpPacket(65535, false, spans[0].slice(rtp_slab[0..])) catch unreachable, Vp8DepacketizeOutcome.in_progress);
    expect_eq(depkt.pushRtpPacket(0, false, spans[1].slice(rtp_slab[0..])) catch unreachable, Vp8DepacketizeOutcome.in_progress);
    expect_eq(depkt.pushRtpPacket(1, true, spans[2].slice(rtp_slab[0..])) catch unreachable, Vp8DepacketizeOutcome.frame_ready);
    expect_eq(depkt.frames_completed, 1);
    expect_eq(depkt.frames_dropped, 0);
    expect_true(bytesEq(depkt.frame(), vp8_frame[0..]));

    // 3. Simulate mid-frame packet loss (packet 1 dropped: seq 2 -> seq 4) -> dropped_incomplete, then recover on next frame!
    expect_eq(depkt.pushRtpPacket(2, false, spans[0].slice(rtp_slab[0..])) catch unreachable, Vp8DepacketizeOutcome.in_progress);
    expect_eq(depkt.pushRtpPacket(4, true, spans[2].slice(rtp_slab[0..])) catch unreachable, Vp8DepacketizeOutcome.dropped_incomplete);
    expect_eq(depkt.frames_dropped, 1);

    // Next frame arrives intact (seq 5, 6, 7) -> recovers seamlessly
    expect_eq(depkt.pushRtpPacket(5, false, spans[0].slice(rtp_slab[0..])) catch unreachable, Vp8DepacketizeOutcome.in_progress);
    expect_eq(depkt.pushRtpPacket(6, false, spans[1].slice(rtp_slab[0..])) catch unreachable, Vp8DepacketizeOutcome.in_progress);
    expect_eq(depkt.pushRtpPacket(7, true, spans[2].slice(rtp_slab[0..])) catch unreachable, Vp8DepacketizeOutcome.frame_ready);
    expect_eq(depkt.frames_completed, 2);
    expect_true(bytesEq(depkt.frame(), vp8_frame[0..]));
}

test("WebM/VP8/VP9/AV1/RTP malformed boundary guard matrix & cap.media namespace parity") {
    // 1. InvalidVint (leading 0x00)
    const zero_vint: [2]u8 = [0x00, 0x01];
    let got_bad_vint: bool = false;
    * = decodeEbmlVint(zero_vint[0..]) catch |e| {
        if (e == WebmMuxError.InvalidVint) got_bad_vint = true;
    };
    expect_true(got_bad_vint);

    // 2. InvalidEbmlHeader (wrong magic)
    const bad_ebml: [8]u8 = [0x11, 0x22, 0x33, 0x44, 0x80, 0x00, 0x00, 0x00];
    let tr_buf: [2]WebmDemuxTrackInfo = [WebmDemuxTrackInfo{}; 2];
    let got_bad_ebml: bool = false;
    * = WebmDemuxIter.init(bad_ebml[0..], tr_buf[0..]) catch |e| {
        if (e == WebmMuxError.InvalidEbmlHeader) got_bad_ebml = true;
    };
    expect_true(got_bad_ebml);

    // 3. InvalidVp8Frame (bad keyframe sync code)
    const bad_vp8: [10]u8 = [0x10, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0x00, 0x01, 0x00, 0x01];
    let got_bad_vp8: bool = false;
    * = parseVp8FrameHeader(bad_vp8[0..]) catch |e| {
        if (e == WebmMuxError.InvalidVp8Frame) got_bad_vp8 = true;
    };
    expect_true(got_bad_vp8);

    // 4. InvalidVp9Frame (wrong frame_marker != 0b10)
    const bad_vp9: [4]u8 = [0x00, 0x49, 0x83, 0x42];
    let got_bad_vp9: bool = false;
    * = parseVp9FrameHeader(bad_vp9[0..]) catch |e| {
        if (e == WebmMuxError.InvalidVp9Frame) got_bad_vp9 = true;
    };
    expect_true(got_bad_vp9);

    // 5. InvalidAv1Obu (obu_forbidden_bit = 1)
    const bad_obu: [3]u8 = [0x8A, 0x01, 0x00];
    let oit = Av1ObuIter.init(bad_obu[0..]);
    let got_bad_obu: bool = false;
    * = oit.next() catch |e| {
        if (e == WebmMuxError.InvalidAv1Obu) got_bad_obu = true;
    };
    expect_true(got_bad_obu);

    // 6. MtuTooSmall in packetizeVp8Rtp
    let tiny_buf: [32]u8 = [0; 32];
    let tiny_spans: [2]Vp8RtpPacketSpan = [Vp8RtpPacketSpan{}; 2];
    let got_mtu_err: bool = false;
    * = packetizeVp8Rtp("frame", 1, Vp8RtpDescriptor{}, tiny_buf[0..], tiny_spans[0..]) catch |e| {
        if (e == WebmMuxError.MtuTooSmall) got_mtu_err = true;
    };
    expect_true(got_mtu_err);

    // 7. cap.media namespace parity
    expect_eq(cap.media.WEBM_EBML_UNKNOWN_SIZE_RAW, 0x01FFFFFFFFFFFFFF);
    expect_eq(cap.media.AV1_OBU_SEQUENCE_HEADER, 1);
    expect_eq(cap.media.VP8_KEYFRAME_SYNC_0, 0x9D);
    expect_eq(cap.media.VP9_KEYFRAME_SYNC_0, 0x49);
}
