// Origin: Pure-CP Stdlib Test Suite (lib/media/opus.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Upgraded Zero-Heap RFC 6716 Opus Framing/LBRR-FEC/DTX & RFC 7587 WebRTC RTP:
//   1. Exhaustive 32-entry RFC 6716 TOC config table verification (SILK/Hybrid/CELT, bandwidth & duration)
//   2. Code 0, Code 1 (CBR), Code 2 (VBR 1B & 2B frame len >251), Code 3 (CBR/VBR + multi-byte Padding) repack roundtrip
//   3. RFC 6716 §3.2.5 >120ms duration cap rejection (both parseOpusPacketFrames & repackOpusFrames)
//   4. RFC 6716 §4.2.7 SILK/Hybrid bit-level VAD, In-Band FEC (LBRR) & DTX silence detection
//   5. RFC 7587 OpusRtpPacketizer & OpusRtpUnpacker: 48kHz timestamps, post-DTX Marker bit, 20msx2->40ms aggregation & split
//   6. RFC 7587 Opus SDP `a=fmtp` bidirectional parse/format across all 8 parameters
//   7. Malformed/truncated/uneven-CBR boundary guards & cap.media namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    OPUS_SAMPLE_RATE,
    OPUS_MAX_PACKET_DURATION_US,
    OPUS_MAX_FRAME_LEN,
    OPUS_MAX_FRAMES_PER_PACKET,
    OpusError,
    OpusMode,
    OpusBandwidth,
    opusConfigDurationUs,
    parseOpusToc,
    OpusFrameSlice,
    encodeOpusFrameLen,
    decodeOpusFrameLen,
    parseOpusPacketFrames,
    OpusRepackOptions,
    repackOpusFrames,
    diagnoseOpusPacket,
    OpusSdpFmtpConfig,
    parseOpusSdpFmtp,
    formatOpusSdpFmtp,
    OpusRtpPacketizer,
    OpusRtpUnpacker,
} from "cap:media";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("RFC 6716 exhaustive 32-entry TOC config table (SILK / Hybrid / CELT)") {
    let cfg: u8 = 0;
    while (cfg < 32) : (cfg += 1) {
        const pkt: [2]u8 = [cfg << 3, 0x00];
        const toc = parseOpusToc(pkt[0..]) orelse unreachable;
        expect_eq(toc.config, cfg);
        expect_eq(toc.frame_duration_us, opusConfigDurationUs(cfg));
        if (cfg < 12) {
            expect_eq(toc.mode, OpusMode.silk);
        } else if (cfg < 16) {
            expect_eq(toc.mode, OpusMode.hybrid);
        } else {
            expect_eq(toc.mode, OpusMode.celt);
        }
        // Verify 48kHz sample count matches duration
        expect_eq(toc.samplesPerFrame(), (toc.frame_duration_us * 48) / 1000);
    }
    // Spot-check key configs: 8 (SILK WB 10ms), 15 (Hybrid FB 20ms), 28 (CELT FB 2.5ms)
    expect_eq(opusConfigDurationUs(8), 10000);
    expect_eq(opusConfigDurationUs(15), 20000);
    expect_eq(opusConfigDurationUs(28), 2500);
}

test("RFC 6716 Code 0 / 1 / 2 (1B & 2B len) / 3 (CBR/VBR + multi-byte Padding) repack roundtrip") {
    // 1. Frame length codec boundary tests (0, 251 -> 1B; 252, 300, 1275 -> 2B)
    let lbuf: [4]u8 = [0; 4];
    expect_eq(encodeOpusFrameLen(251, lbuf[0..]) catch unreachable, 1);
    expect_eq((decodeOpusFrameLen(lbuf[0..1]) catch unreachable).frame_len, 251);
    expect_eq(encodeOpusFrameLen(252, lbuf[0..]) catch unreachable, 2);
    expect_eq((decodeOpusFrameLen(lbuf[0..2]) catch unreachable).frame_len, 252);
    expect_eq(encodeOpusFrameLen(1275, lbuf[0..]) catch unreachable, 2);
    expect_eq((decodeOpusFrameLen(lbuf[0..2]) catch unreachable).frame_len, 1275);

    let out_pkt: [2048]u8 = [0; 2048];
    let parsed_frames: [8]OpusFrameSlice = [OpusFrameSlice{}; 8];

    // 2. Code 0 (single frame, config=31 CELT FB 20ms stereo)
    const f_single: [1]OpusFrameSlice = [OpusFrameSlice{ data: "opus-frame-0" }];
    const n0 = repackOpusFrames(f_single[0..], OpusRepackOptions{ config: 31, is_stereo: true }, out_pkt[0..]) catch unreachable;
    expect_eq(out_pkt[0] & 0x03, 0);
    const p0 = parseOpusPacketFrames(out_pkt[0..n0], parsed_frames[0..]) catch unreachable;
    expect_eq(p0.frame_count, 1);
    expect_eq(p0.total_samples_48k, 960);
    expect_true(bytesEq(parsed_frames[0].data, "opus-frame-0"));

    // 3. Code 1 (2 equal-length CBR frames)
    const f_cbr2: [2]OpusFrameSlice = [
        OpusFrameSlice{ data: "frameA" },
        OpusFrameSlice{ data: "frameB" },
    ];
    const n1 = repackOpusFrames(f_cbr2[0..], OpusRepackOptions{ config: 31, is_stereo: true }, out_pkt[0..]) catch unreachable;
    expect_eq(out_pkt[0] & 0x03, 1);
    const p1 = parseOpusPacketFrames(out_pkt[0..n1], parsed_frames[0..]) catch unreachable;
    expect_eq(p1.frame_count, 2);
    expect_eq(p1.total_samples_48k, 1920);
    expect_true(bytesEq(parsed_frames[0].data, "frameA"));
    expect_true(bytesEq(parsed_frames[1].data, "frameB"));

    // 4. Code 2 (2 unequal frames with 2-byte length > 251!)
    let big_f0: [300]u8 = [0x5A; 300];
    const f_vbr2: [2]OpusFrameSlice = [
        OpusFrameSlice{ data: big_f0[0..] },
        OpusFrameSlice{ data: "tail" },
    ];
    const n2 = repackOpusFrames(f_vbr2[0..], OpusRepackOptions{ config: 31, is_stereo: false }, out_pkt[0..]) catch unreachable;
    expect_eq(out_pkt[0] & 0x03, 2);
    const p2 = parseOpusPacketFrames(out_pkt[0..n2], parsed_frames[0..]) catch unreachable;
    expect_eq(p2.frame_count, 2);
    expect_eq(parsed_frames[0].data.len, 300);
    expect_true(bytesEq(parsed_frames[1].data, "tail"));

    // 5. Code 3 VBR (3 unequal frames) + multi-byte Padding (target_packet_len = 600 -> >254 pad bytes!)
    const f_vbr3: [3]OpusFrameSlice = [
        OpusFrameSlice{ data: "one" },
        OpusFrameSlice{ data: "two-2" },
        OpusFrameSlice{ data: "three-33" },
    ];
    const n3 = repackOpusFrames(f_vbr3[0..], OpusRepackOptions{
        config: 31,
        is_stereo: true,
        target_packet_len: 600,
    }, out_pkt[0..]) catch unreachable;
    expect_eq(n3, 600);
    expect_eq(out_pkt[0] & 0x03, 3);
    const p3 = parseOpusPacketFrames(out_pkt[0..n3], parsed_frames[0..]) catch unreachable;
    expect_eq(p3.frame_count, 3);
    expect_true(p3.toc.is_vbr);
    expect_true(p3.toc.has_padding);
    expect_true(p3.padding_bytes > 500);
    expect_eq(p3.total_duration_us, 60000);
    expect_true(bytesEq(parsed_frames[0].data, "one"));
    expect_true(bytesEq(parsed_frames[1].data, "two-2"));
    expect_true(bytesEq(parsed_frames[2].data, "three-33"));
}

test("RFC 6716 §3.2.5 >120ms duration cap rejection on both parse and repack") {
    // Config 3 = SILK NB 60ms; 3 frames = 180ms > 120ms -> DurationExceeds120Ms
    const over_pkt: [8]u8 = [(3 << 3) | 0x03, 3, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66];
    let frames: [4]OpusFrameSlice = [OpusFrameSlice{}; 4];
    let got_parse_err: bool = false;
    * = parseOpusPacketFrames(over_pkt[0..], frames[0..]) catch |e| {
        if (e == OpusError.DurationExceeds120Ms) got_parse_err = true;
    };
    expect_true(got_parse_err);

    const f3: [3]OpusFrameSlice = [
        OpusFrameSlice{ data: "aa" },
        OpusFrameSlice{ data: "bb" },
        OpusFrameSlice{ data: "cc" },
    ];
    let out: [32]u8 = [0; 32];
    let got_repack_err: bool = false;
    * = repackOpusFrames(f3[0..], OpusRepackOptions{ config: 3, is_stereo: false }, out[0..]) catch |e| {
        if (e == OpusError.DurationExceeds120Ms) got_repack_err = true;
    };
    expect_true(got_repack_err);
}

test("RFC 6716 §4.2.7 SILK/Hybrid bit-level VAD, In-Band FEC (LBRR) & DTX silence detection") {
    let scratch: [4]OpusFrameSlice = [OpusFrameSlice{}; 4];

    // 1. SILK WB 20ms mono (config=9, Code 0): 1 subframe -> bit7=VAD, bit6=LBRR
    //    Payload byte 0 = 0xC0 (VAD=1, LBRR=1) + voice bytes
    const silk_fec_pkt: [5]u8 = [(9 << 3), 0xC0, 0x11, 0x22, 0x33];
    const d_fec = diagnoseOpusPacket(silk_fec_pkt[0..], scratch[0..]) catch unreachable;
    expect_eq(d_fec.mode, OpusMode.silk);
    expect_eq(d_fec.bandwidth, OpusBandwidth.wideband);
    expect_true(d_fec.has_vad);
    expect_true(d_fec.has_lbrr_fec);
    expect_true(!d_fec.is_dtx);

    // 2. Hybrid FB 20ms stereo (config=15, stereo=1): Ch0(bit7=VAD,bit6=LBRR), Ch1(bit5=VAD,bit4=LBRR)
    //    0x90 = Ch0 VAD=1 LBRR=0, Ch1 VAD=0 LBRR=1 -> has_vad=true, has_lbrr_fec=true
    const hyb_pkt: [4]u8 = [(15 << 3) | 0x04, 0x90, 0x55, 0x66];
    const d_hyb = diagnoseOpusPacket(hyb_pkt[0..], scratch[0..]) catch unreachable;
    expect_eq(d_hyb.mode, OpusMode.hybrid);
    expect_eq(d_hyb.channels, 2);
    expect_true(d_hyb.has_vad);
    expect_true(d_hyb.has_lbrr_fec);
    expect_true(!d_hyb.is_dtx);

    // 3. TOC-only 1-byte DTX silence packet
    const dtx_pkt: [1]u8 = [(9 << 3)];
    const d_dtx = diagnoseOpusPacket(dtx_pkt[0..], scratch[0..]) catch unreachable;
    expect_true(d_dtx.is_dtx);
    expect_true(!d_dtx.has_vad);
    expect_true(!d_dtx.has_lbrr_fec);
}

test("RFC 7587 OpusRtpPacketizer & OpusRtpUnpacker: 48kHz TS, post-DTX Marker, 20msx2->40ms & split") {
    let scratch: [4]OpusFrameSlice = [OpusFrameSlice{}; 4];
    let pktzr = OpusRtpPacketizer.init(65535, 10000);

    // Packet 0: DTX silence (20ms = 960 samples) -> marker=false, stays in DTX silence
    const p_dtx: [1]u8 = [(9 << 3)];
    const m0 = pktzr.nextPacketMeta(p_dtx[0..], scratch[0..]) catch unreachable;
    expect_eq(m0.seq, 65535);
    expect_eq(m0.timestamp_48k, 10000);
    expect_true(m0.is_dtx);
    expect_true(!m0.marker);
    expect_eq(m0.samples_48k, 960);

    // Packet 1: First active voice packet after DTX -> marker=true, seq wraps 65535 -> 0, ts = 10960
    const p_v1: [5]u8 = [(9 << 3), 0xC0, 0xAA, 0xBB, 0xCC];
    const m1 = pktzr.nextPacketMeta(p_v1[0..], scratch[0..]) catch unreachable;
    expect_eq(m1.seq, 0);
    expect_eq(m1.timestamp_48k, 10960);
    expect_true(m1.marker);
    expect_true(m1.has_lbrr_fec);

    // Packet 2: Subsequent voice packet -> marker=false, seq=1, ts=11920
    const p_v2: [4]u8 = [(9 << 3), 0x80, 0xDD, 0xEE];
    const m2 = pktzr.nextPacketMeta(p_v2[0..], scratch[0..]) catch unreachable;
    expect_eq(m2.seq, 1);
    expect_eq(m2.timestamp_48k, 11920);
    expect_true(!m2.marker);

    // Aggregate 2x20ms packets -> single 40ms packet (1920 samples @ 48kHz)
    let s_a: [2]OpusFrameSlice = [OpusFrameSlice{}; 2];
    let s_b: [2]OpusFrameSlice = [OpusFrameSlice{}; 2];
    let s_all: [4]OpusFrameSlice = [OpusFrameSlice{}; 4];
    let agg_buf: [64]u8 = [0; 64];
    const agg_len = OpusRtpPacketizer.aggregatePackets(p_v1[0..], p_v2[0..], s_a[0..], s_b[0..], s_all[0..], agg_buf[0..]) catch unreachable;

    let unpk = OpusRtpUnpacker.init();
    const diag_agg = unpk.observePacket(agg_buf[0..agg_len], scratch[0..]) catch unreachable;
    expect_eq(diag_agg.frame_count, 2);
    expect_eq(diag_agg.total_duration_us, 40000);
    expect_eq(diag_agg.total_samples_48k, 1920);
    expect_eq(unpk.fec_packets_seen, 1);

    // Split 40ms packet back into 2 standalone Code 0 20ms packets
    let split_slab: [64]u8 = [0; 64];
    let split_spans: [4]OpusFrameSlice = [OpusFrameSlice{}; 4];
    const n_split = OpusRtpUnpacker.splitToSingleFramePackets(agg_buf[0..agg_len], scratch[0..], split_slab[0..], split_spans[0..]) catch unreachable;
    expect_eq(n_split, 2);
    expect_true(bytesEq(split_spans[0].data, p_v1[0..]));
    expect_true(bytesEq(split_spans[1].data, p_v2[0..]));
}

test("RFC 7587 Opus SDP `a=fmtp` bidirectional parse & format across all 8 parameters") {
    const line = "a=fmtp:111 minptime=10;maxplaybackrate=48000;maxaveragebitrate=64000;stereo=1;sprop-stereo=1;cbr=0;useinbandfec=1;usedtx=1";
    const cfg = parseOpusSdpFmtp(line) catch unreachable;
    expect_eq(cfg.payload_type, 111);
    expect_eq(cfg.minptime, 10);
    expect_eq(cfg.maxplaybackrate, 48000);
    expect_eq(cfg.maxaveragebitrate, 64000);
    expect_true(cfg.has_stereo && cfg.stereo);
    expect_true(cfg.has_sprop_stereo && cfg.sprop_stereo);
    expect_true(cfg.has_cbr && !cfg.cbr);
    expect_true(cfg.has_useinbandfec && cfg.useinbandfec);
    expect_true(cfg.has_usedtx && cfg.usedtx);

    let out: [256]u8 = [0; 256];
    const n = formatOpusSdpFmtp(cfg, out[0..]) catch unreachable;
    expect_true(bytesEq(out[0..n], line));
}

test("Opus malformed/truncated/uneven-CBR boundary guards & cap.media namespace parity") {
    let frames: [4]OpusFrameSlice = [OpusFrameSlice{}; 4];

    // 1. EmptyPacket
    const empty: [0]u8 = [];
    let got_empty: bool = false;
    * = parseOpusPacketFrames(empty[0..], frames[0..]) catch |e| {
        if (e == OpusError.EmptyPacket) got_empty = true;
    };
    expect_true(got_empty);

    // 2. UnevenCbrFrames in Code 1 (odd remaining bytes)
    const odd_c1: [4]u8 = [(31 << 3) | 0x01, 0x11, 0x22, 0x33];
    let got_uneven: bool = false;
    * = parseOpusPacketFrames(odd_c1[0..], frames[0..]) catch |e| {
        if (e == OpusError.UnevenCbrFrames) got_uneven = true;
    };
    expect_true(got_uneven);

    // 3. InvalidPadding (padding exceeds packet size)
    const bad_pad: [4]u8 = [(31 << 3) | 0x03, 0x41, 10, 0x00]; // declares 10B padding in 4B packet
    let got_pad: bool = false;
    * = parseOpusPacketFrames(bad_pad[0..], frames[0..]) catch |e| {
        if (e == OpusError.InvalidPadding) got_pad = true;
    };
    expect_true(got_pad);

    // 4. InvalidFmtp
    let got_fmtp: bool = false;
    * = parseOpusSdpFmtp("a=fmtp:111 minptime=abc") catch |e| {
        if (e == OpusError.InvalidFmtp) got_fmtp = true;
    };
    expect_true(got_fmtp);

    // 5. cap.media namespace parity
    expect_eq(cap.media.OPUS_SAMPLE_RATE, OPUS_SAMPLE_RATE);
    expect_eq(cap.media.OPUS_MAX_PACKET_DURATION_US, OPUS_MAX_PACKET_DURATION_US);
    expect_eq(cap.media.OPUS_MAX_FRAME_LEN, OPUS_MAX_FRAME_LEN);
    expect_eq(cap.media.OPUS_MAX_FRAMES_PER_PACKET, OPUS_MAX_FRAMES_PER_PACKET);
}
