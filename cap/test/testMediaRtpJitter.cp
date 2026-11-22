// Origin: Pure-CP Stdlib Test Suite (lib/media/rtpJitter.cp) | Created: 2026-11-23
// Production Deep Verification Suite for RFC 3550 RTP/RTCP, G.711 PCMU/PCMA & Adaptive Jitter Buffer/PLC:
//   1. RTP full round-trip (CSRC + header extension + padding) & malformed packet error guards
//   2. `RtpSeqUnwrapper` 16-bit wrap-around (`65534 -> 65535 -> 0 -> 1`) & out-of-order late arrival
//   3. RTCP SR/RR round-trip (incl. signed 24-bit `cumulative_lost`) & RFC 3550 §A.8 jitter/loss stats
//   4. ITU-T G.711 PCMU & PCMA full-range SNR (> 33 dB) & bidirectional PCMU <-> PCMA transcoding
//   5. `AdaptiveJitterBuffer` out-of-order reordering, duplicate dedup & late packet rejection
//   6. Packet gap `.plc_concealment` trigger & `applyPcm16Plc` progressive fade-out to silence
//   7. Adaptive target delay expansion under high jitter & `cap.media` facade parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    RTP_VERSION,
    RTP_MIN_HEADER_LEN,
    RTCP_PT_SR,
    RTCP_PT_RR,
    RTP_PT_PCMU,
    RTP_PT_PCMA,
    RTP_PT_OPUS,
    RtpMediaError,
    RtpHeader,
    parseRtpPacket,
    encodeRtpPacket,
    RtpSeqUnwrapper,
    RtcpSenderReport,
    RtcpReportBlock,
    encodeRtcpSr,
    parseRtcpSr,
    encodeRtcpRr,
    parseRtcpRr,
    RtpRecvStats,
    linear16ToPcmu,
    pcmuToLinear16,
    linear16ToPcma,
    pcmaToLinear16,
    encodePcmuSlice,
    decodePcmuSlice,
    encodePcmaSlice,
    decodePcmaSlice,
    transcodePcmuToPcma,
    transcodePcmaToPcmu,
    JitterPopKind,
    JitterSlot,
    applyPcm16Plc,
    AdaptiveJitterBuffer,
} from "cap:media";

fn computeSnrDb(orig: []const i16, decoded: []const i16): f64 {
    let sig_p: f64 = 0.0;
    let err_p: f64 = 0.0;
    let i: usize = 0;
    while (i < orig.len) : (i += 1) {
        const s: f64 = @as(f64, @floatFromInt(orig[i]));
        const d: f64 = @as(f64, @floatFromInt(decoded[i]));
        const e: f64 = s - d;
        sig_p += s * s;
        err_p += e * e;
    }
    if (err_p <= 1.0e-12) return 100.0;
    return 10.0 * std.math.log10(sig_p / err_p);
}

test("RTP: CSRC + extension + padding round-trip & malformed packet guards") {
    let hdr = RtpHeader{
        version: RTP_VERSION,
        padding: true,
        extension: true,
        csrc_count: 2,
        marker: true,
        payload_type: RTP_PT_OPUS,
        seq: 4201,
        timestamp: 96000,
        ssrc: 0xDEADBEEF,
        ext_profile: 0xBEDE,
        ext_len: 8,
    };
    hdr.csrc[0] = 0x11112222;
    hdr.csrc[1] = 0x33334444;

    const payload: [5]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0xEE];
    let wire: [128]u8 = [0; 128];
    let wire_s: []u8 = wire[0..];

    const enc_len = encodeRtpPacket(hdr, payload[0..], *wire_s) catch {
        expect_true(false);
        return;
    };
    // 12 + 8 (CSRC) + 4 + 8 (ext) + 5 (payload) = 37 -> padded to 40 (pad_len = 3)
    expect_eq(enc_len, @as(usize, 40));

    const parsed = cap.media.parseRtpPacket(wire[0..enc_len]) catch {
        expect_true(false);
        return;
    };
    expect_eq(parsed.version, RTP_VERSION);
    expect_true(parsed.padding);
    expect_true(parsed.extension);
    expect_true(parsed.marker);
    expect_eq(parsed.csrc_count, @as(u8, 2));
    expect_eq(parsed.csrc[0], @as(u32, 0x11112222));
    expect_eq(parsed.csrc[1], @as(u32, 0x33334444));
    expect_eq(parsed.payload_type, RTP_PT_OPUS);
    expect_eq(parsed.seq, @as(u16, 4201));
    expect_eq(parsed.timestamp, @as(u32, 96000));
    expect_eq(parsed.ssrc, @as(u32, 0xDEADBEEF));
    expect_eq(parsed.ext_profile, @as(u16, 0xBEDE));
    expect_eq(parsed.ext_len, @as(usize, 8));
    expect_eq(parsed.payload_len, @as(usize, 5));
    expect_eq(wire[parsed.payload_offset], @as(u8, 0xAA));
    expect_eq(wire[parsed.payload_offset + 4], @as(u8, 0xEE));

    // Malformed packet guards
    let got_short: bool = false;
    * = parseRtpPacket(wire[0..8]) catch |e| {
        got_short = (e == RtpMediaError.PacketTooShort);
    };
    expect_true(got_short);

    wire[0] = 0x40; // version = 1
    let got_ver: bool = false;
    * = parseRtpPacket(wire[0..enc_len]) catch |e| {
        got_ver = (e == RtpMediaError.UnsupportedVersion);
    };
    expect_true(got_ver);
}

test("RtpSeqUnwrapper: 16-bit wrap-around (65534->65535->0->1) & out-of-order late arrival") {
    let u = RtpSeqUnwrapper.init();
    expect_eq(u.unwrap(65534), @as(i64, 65534));
    expect_eq(u.unwrap(65535), @as(i64, 65535));
    expect_eq(u.unwrap(0), @as(i64, 65536));
    expect_eq(u.unwrap(1), @as(i64, 65537));
    // Out-of-order packet 65535 arriving after wrap must map back to 65535
    expect_eq(u.unwrap(65535), @as(i64, 65535));
    expect_eq(u.unwrap(2), @as(i64, 65538));
}

test("RTCP: SR & RR round-trip (signed 24-bit loss) + RFC 3550 recv jitter/loss stats") {
    // 1. Sender Report round-trip
    const sr = RtcpSenderReport{
        ssrc: 0xCAFEBABE,
        ntp_sec: 3900000123,
        ntp_frac: 0x80000000,
        rtp_timestamp: 480000,
        packet_count: 1500,
        octet_count: 240000,
    };
    let buf: [64]u8 = [0; 64];
    let buf_s: []u8 = buf[0..];
    const sr_len = encodeRtcpSr(sr, *buf_s) catch 0;
    expect_eq(sr_len, @as(usize, 28));
    expect_eq(buf[1], RTCP_PT_SR);

    const sr_dec = parseRtcpSr(buf[0..sr_len]) catch {
        expect_true(false);
        return;
    };
    expect_eq(sr_dec.ssrc, sr.ssrc);
    expect_eq(sr_dec.ntp_sec, sr.ntp_sec);
    expect_eq(sr_dec.packet_count, sr.packet_count);
    expect_eq(sr_dec.octet_count, sr.octet_count);

    // 2. Receiver Report round-trip with negative cumulative_lost (duplicates)
    const blk = RtcpReportBlock{
        ssrc: 0xCAFEBABE,
        fraction_lost: 51,
        cumulative_lost: -3,
        ext_highest_seq: 65540,
        jitter: 128,
        lsr: 0x11223344,
        dlsr: 65536,
    };
    const rr_len = encodeRtcpRr(0x10203040, blk, *buf_s) catch 0;
    expect_eq(rr_len, @as(usize, 32));
    expect_eq(buf[1], RTCP_PT_RR);

    let sender_ssrc: u32 = 0;
    const blk_dec = parseRtcpRr(buf[0..rr_len], *sender_ssrc) catch {
        expect_true(false);
        return;
    };
    expect_eq(sender_ssrc, @as(u32, 0x10203040));
    expect_eq(blk_dec.cumulative_lost, @as(i32, -3));
    expect_eq(blk_dec.fraction_lost, @as(u8, 51));
    expect_eq(blk_dec.jitter, @as(u32, 128));

    // 3. RtpRecvStats: 8 of 10 packets received (20% loss) with alternating jitter
    let st = RtpRecvStats.init(0xCAFEBABE, 8000);
    st.onPacket(10, 1600, 100);
    st.onPacket(11, 1760, 125); // +5ms jitter
    st.onPacket(13, 2080, 155); // seq 12 lost
    st.onPacket(14, 2240, 185);
    st.onPacket(15, 2400, 200);
    st.onPacket(16, 2560, 230);
    st.onPacket(18, 2880, 260); // seq 17 lost
    st.onPacket(19, 3040, 280);

    expect_true(st.jitterMs() > 1.0);
    const rb = st.buildReportBlock(0, 0);
    expect_eq(rb.cumulative_lost, @as(i32, 2)); // expected 10 (10..19), received 8
    expect_true(rb.fraction_lost >= 50 && rb.fraction_lost <= 52); // 2/10 * 256 = 51
}

test("G.711: PCMU & PCMA full-range SNR > 33 dB & PCMU <-> PCMA transcoding") {
    let pcm_in: [160]i16 = [0; 160];
    let i: usize = 0;
    while (i < 160) : (i += 1) {
        const phase: f64 = 6.283185307179586 * 1000.0 * @as(f64, @floatFromInt(i)) / 8000.0;
        pcm_in[i] = @as(i16, @intFromFloat(16000.0 * std.math.sin(phase)));
    }

    let ulaw: [160]u8 = [0; 160];
    let alaw: [160]u8 = [0; 160];
    let dec_u: [160]i16 = [0; 160];
    let dec_a: [160]i16 = [0; 160];
    let ulaw_s: []u8 = ulaw[0..];
    let alaw_s: []u8 = alaw[0..];
    let dec_u_s: []i16 = dec_u[0..];
    let dec_a_s: []i16 = dec_a[0..];

    expect_eq(encodePcmuSlice(pcm_in[0..], *ulaw_s) catch 0, @as(usize, 160));
    expect_eq(decodePcmuSlice(ulaw[0..], *dec_u_s) catch 0, @as(usize, 160));
    expect_true(computeSnrDb(pcm_in[0..], dec_u[0..]) > 33.0);

    expect_eq(encodePcmaSlice(pcm_in[0..], *alaw_s) catch 0, @as(usize, 160));
    expect_eq(decodePcmaSlice(alaw[0..], *dec_a_s) catch 0, @as(usize, 160));
    expect_true(computeSnrDb(pcm_in[0..], dec_a[0..]) > 33.0);

    // Transcode PCMU -> PCMA -> PCMU and verify fidelity
    let x_alaw: [160]u8 = [0; 160];
    let x_ulaw: [160]u8 = [0; 160];
    let x_pcm: [160]i16 = [0; 160];
    let xa_s: []u8 = x_alaw[0..];
    let xu_s: []u8 = x_ulaw[0..];
    let xp_s: []i16 = x_pcm[0..];
    expect_eq(transcodePcmuToPcma(ulaw[0..], *xa_s) catch 0, @as(usize, 160));
    expect_eq(transcodePcmaToPcmu(x_alaw[0..], *xu_s) catch 0, @as(usize, 160));
    expect_eq(decodePcmuSlice(x_ulaw[0..], *xp_s) catch 0, @as(usize, 160));
    expect_true(computeSnrDb(pcm_in[0..], x_pcm[0..]) > 30.0);

    // Silence & polarity sanity
    expect_true(pcmuToLinear16(linear16ToPcmu(0)) == 0);
    expect_true(pcmaToLinear16(linear16ToPcma(-20000)) < -18000);
}

test("AdaptiveJitterBuffer: reordering, duplicate dedup & late packet rejection") {
    let slots: [8]JitterSlot = [JitterSlot{}; 8];
    let pool: [128]u8 = [0; 128];
    let slots_s: []JitterSlot = slots[0..];
    let pool_s: []u8 = pool[0..];

    let jb = cap.media.AdaptiveJitterBuffer.init(16, 8000, 20, 200, *slots_s, *pool_s) catch {
        expect_true(false);
        return;
    };

    const p0: [4]u8 = [10, 10, 10, 10];
    const p1: [4]u8 = [11, 11, 11, 11];
    const p2: [4]u8 = [12, 12, 12, 12];
    const p3: [4]u8 = [13, 13, 13, 13];

    // Push out-of-order: 102, 100, 101, 100(dup), 103
    expect_true(jb.pushPacket(RtpHeader{ seq: 102, timestamp: 320 }, p2[0..], 100) catch false);
    expect_true(jb.pushPacket(RtpHeader{ seq: 100, timestamp: 0 }, p0[0..], 102) catch false);
    expect_true(jb.pushPacket(RtpHeader{ seq: 101, timestamp: 160 }, p1[0..], 104) catch false);
    expect_true(!(jb.pushPacket(RtpHeader{ seq: 100, timestamp: 0 }, p0[0..], 105) catch true)); // dup
    expect_true(jb.pushPacket(RtpHeader{ seq: 103, timestamp: 480 }, p3[0..], 106) catch false);

    let out: [16]u8 = [0; 16];
    let out_s: []u8 = out[0..];

    // Before target_delay_ms -> pre-buffering returns empty_underrun
    const pre = jb.popNextFrame(110, *out_s) catch {
        expect_true(false);
        return;
    };
    expect_true(pre.kind == JitterPopKind.empty_underrun);

    // After delay -> pops 100, 101, 102, 103 in exact order
    const r0 = jb.popNextFrame(200, *out_s) catch { expect_true(false); return; };
    expect_true(r0.kind == JitterPopKind.audio_packet);
    expect_eq(r0.ext_seq, @as(i64, 100));
    expect_eq(out[0], @as(u8, 10));

    const r1 = jb.popNextFrame(220, *out_s) catch { expect_true(false); return; };
    expect_eq(r1.ext_seq, @as(i64, 101));
    expect_eq(out[0], @as(u8, 11));

    const r2 = jb.popNextFrame(240, *out_s) catch { expect_true(false); return; };
    expect_eq(r2.ext_seq, @as(i64, 102));
    expect_eq(out[0], @as(u8, 12));

    const r3 = jb.popNextFrame(260, *out_s) catch { expect_true(false); return; };
    expect_eq(r3.ext_seq, @as(i64, 103));
    expect_eq(out[0], @as(u8, 13));

    // Late packet 99 arriving after playout started must be dropped
    expect_true(!(jb.pushPacket(RtpHeader{ seq: 99, timestamp: 0 }, p0[0..], 270) catch true));
    const st = jb.stats();
    expect_eq(st.duplicates_dropped, @as(u64, 1));
    expect_eq(st.late_dropped, @as(u64, 1));
    expect_eq(st.packets_popped, @as(u64, 4));
}

test("PLC: gap triggers plc_concealment & applyPcm16Plc progressively fades to silence") {
    let slots: [8]JitterSlot = [JitterSlot{}; 8];
    let pool: [64]u8 = [0; 64];
    let slots_s: []JitterSlot = slots[0..];
    let pool_s: []u8 = pool[0..];

    let jb = AdaptiveJitterBuffer.init(8, 8000, 20, 100, *slots_s, *pool_s) catch {
        expect_true(false);
        return;
    };
    const p: [4]u8 = [1, 2, 3, 4];
    * = jb.pushPacket(RtpHeader{ seq: 200, timestamp: 0 }, p[0..], 0) catch false;
    * = jb.pushPacket(RtpHeader{ seq: 202, timestamp: 320 }, p[0..], 40) catch false; // 201 lost

    let out: [8]u8 = [0; 8];
    let out_s: []u8 = out[0..];
    const pop0 = jb.popNextFrame(100, *out_s) catch { expect_true(false); return; };
    expect_true(pop0.kind == JitterPopKind.audio_packet);
    expect_eq(pop0.ext_seq, @as(i64, 200));

    const pop1 = jb.popNextFrame(120, *out_s) catch { expect_true(false); return; };
    expect_true(pop1.kind == JitterPopKind.plc_concealment);
    expect_eq(pop1.ext_seq, @as(i64, 201));

    const pop2 = jb.popNextFrame(140, *out_s) catch { expect_true(false); return; };
    expect_true(pop2.kind == JitterPopKind.audio_packet);
    expect_eq(pop2.ext_seq, @as(i64, 202));
    expect_eq(jb.stats().plc_concealed, @as(u64, 1));

    // Verify applyPcm16Plc progressive attenuation -> complete mute at loss 6
    const prev: [8]i16 = [10000; 8];
    let f1: [8]i16 = [0; 8];
    let f3: [8]i16 = [0; 8];
    let f6: [8]i16 = [0; 8];
    let f1_s: []i16 = f1[0..];
    let f3_s: []i16 = f3[0..];
    let f6_s: []i16 = f6[0..];

    * = applyPcm16Plc(prev[0..], 1, *f1_s) catch 0;
    * = applyPcm16Plc(prev[0..], 3, *f3_s) catch 0;
    * = applyPcm16Plc(prev[0..], 6, *f6_s) catch 0;

    expect_true(f1[0] > f3[0] && f3[0] > 0);
    expect_true(f1[0] > f1[7]); // Smooth intra-frame ramp
    expect_eq(f6[0], @as(i16, 0));
    expect_eq(f6[7], @as(i16, 0));
}

test("AdaptiveJitterBuffer: high-jitter target_delay_ms auto-expansion & config guards") {
    expect_eq(RTP_MIN_HEADER_LEN, @as(usize, 12));
    expect_eq(RTP_PT_PCMU, @as(u8, 0));
    expect_eq(RTP_PT_PCMA, @as(u8, 8));

    let slots: [16]JitterSlot = [JitterSlot{}; 16];
    let pool: [256]u8 = [0; 256];
    let slots_s: []JitterSlot = slots[0..];
    let pool_s: []u8 = pool[0..];

    let jb = AdaptiveJitterBuffer.init(16, 8000, 20, 200, *slots_s, *pool_s) catch {
        expect_true(false);
        return;
    };
    const p: [4]u8 = [0x55; 4];

    // Feed bursty high-jitter arrivals (20ms RTP steps arriving at wildly varying intervals)
    let t_ms: u64 = 0;
    let seq: u16 = 0;
    while (seq < 12) : (seq += 1) {
        const step: u64 = if ((seq % 2) == 0) 5 else 65;
        t_ms += step;
        * = jb.pushPacket(RtpHeader{ seq: seq, timestamp: @as(u32, seq) * 160 }, p[0..], t_ms) catch false;
    }

    const st = jb.stats();
    expect_true(st.jitter_ms > 10.0);
    expect_true(st.target_delay_ms > 20 && st.target_delay_ms <= 200);

    // Invalid config guard (min_delay > max_delay)
    let got_bad_cfg: bool = false;
    * = AdaptiveJitterBuffer.init(16, 8000, 300, 100, *slots_s, *pool_s) catch |e| {
        got_bad_cfg = (e == RtpMediaError.InvalidConfig);
    };
    expect_true(got_bad_cfg);
}
