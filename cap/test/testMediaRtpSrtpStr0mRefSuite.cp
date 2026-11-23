// str0m (MIT/Apache-2.0) Reference-Hardened RTP/RTCP & SRTP/SRTCP Regression Suite
// Ports 26 real-world WebRTC media framing & cryptographic edge-case tests from algesten/str0m.

const std = @import("std");
import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    RTP_VERSION,
    RTP_EXT_PROFILE_ONE_BYTE,
    RTP_EXT_PROFILE_TWO_BYTE,
    RTCP_PT_SR,
    RTCP_PT_RR,
    RTCP_PT_RTPFB,
    RTCP_PT_PSFB,
    RTCP_FMT_NACK,
    RTCP_FMT_PLI,
    RTCP_FMT_FIR,
    RtpMediaError,
    RtpHeader,
    parseRtpPacket,
    encodeRtpPacket,
    RtcpSenderReport,
    RtcpReportBlock,
    encodeRtcpSr,
    parseRtcpSr,
    encodeRtcpRr,
    parseRtcpRr,
    RtpExtForm,
    detectRtpExtForm,
    RtpExtEntry,
    RtpExtIter,
    parseRtpHeaderExtensions,
    encodeRtpHeaderExtensions,
    RtpAudioLevel,
    parseRtpExtAudioLevel,
    encodeRtpExtAudioLevel,
    parseRtpExtAbsSendTime,
    encodeRtpExtAbsSendTime,
    parseRtpExtTransportCc,
    encodeRtpExtTransportCc,
    encodeRtpPacketWithExtensions,
    RtcpPacketView,
    RtcpCompoundIter,
    RtcpNackEntry,
    encodeRtcpNack,
    parseRtcpNack,
    expandRtcpNackEntry,
    RtcpPli,
    encodeRtcpPli,
    parseRtcpPli,
    RtcpFirEntry,
    encodeRtcpFir,
    parseRtcpFir,
} from "../../lib/media/index.cp";
import {
    SrtpError,
    SrtpProfile,
    SrtpKeyMaterial,
    SrtpContext,
    estimateSrtpPacketIndex,
} from "../../lib/net/index.cp";

fn makeTestMasterKey(): SrtpKeyMaterial {
    return SrtpKeyMaterial{
        master_key: [0xE1, 0xF9, 0x7A, 0x0D, 0x3E, 0x01, 0x8B, 0xE0, 0xD6, 0x4F, 0xA3, 0x2C, 0x06, 0xDE, 0x41, 0x39],
        master_salt: [0x0E, 0xC6, 0x75, 0xAD, 0x49, 0x8A, 0xFE, 0xEB, 0xB6, 0x96, 0x0B, 0x3A, 0xAB, 0xE6],
    };
}

fn buildMinimalRtp(seq: u16, ts: u32, ssrc: u32, payload: []const u8, out: []u8): usize {
    out[0] = 0x80; // V=2
    out[1] = 111;  // PT=111
    out[2] = @as(u8, @intCast((seq >> 8) & 0xFF));
    out[3] = @as(u8, @intCast(seq & 0xFF));
    out[4] = @as(u8, @intCast((ts >> 24) & 0xFF));
    out[5] = @as(u8, @intCast((ts >> 16) & 0xFF));
    out[6] = @as(u8, @intCast((ts >> 8) & 0xFF));
    out[7] = @as(u8, @intCast(ts & 0xFF));
    out[8] = @as(u8, @intCast((ssrc >> 24) & 0xFF));
    out[9] = @as(u8, @intCast((ssrc >> 16) & 0xFF));
    out[10] = @as(u8, @intCast((ssrc >> 8) & 0xFF));
    out[11] = @as(u8, @intCast(ssrc & 0xFF));
    let i: usize = 0;
    while (i < payload.len) : (i += 1) {
        out[12 + i] = payload[i];
    }
    return 12 + payload.len;
}

// ============================================================================
// Part A: RFC 8285 RTP Header Extensions & Padding Guards
// ============================================================================

test("str0m_ref_01_rtp_ext_form_detection") {
    expect_eq(detectRtpExtForm(0xBEDE), RtpExtForm.one_byte);
    expect_eq(detectRtpExtForm(0x1000), RtpExtForm.two_byte);
    expect_eq(detectRtpExtForm(0x100F), RtpExtForm.two_byte); // app bits ignored
    expect_eq(detectRtpExtForm(0x1010), RtpExtForm.unknown);
    expect_eq(detectRtpExtForm(0xCAFE), RtpExtForm.unknown);
}

test("str0m_ref_02_rtp_ext_one_byte_roundtrip_mid_audio_ast_twcc") {
    let al_bytes: [1]u8 = [encodeRtpExtAudioLevel(RtpAudioLevel{ voice_activity: true, level_dbov: 42 })];
    let ast_bytes: [3]u8 = [0; 3];
    * = encodeRtpExtAbsSendTime(0x00AB_CDEF, ast_bytes[0..]) catch unreachable;
    let twcc_bytes: [2]u8 = [0; 2];
    * = encodeRtpExtTransportCc(0x1234, twcc_bytes[0..]) catch unreachable;

    const entries: [4]RtpExtEntry = [
        RtpExtEntry{ id: 1, data: "audio0" },
        RtpExtEntry{ id: 2, data: al_bytes[0..] },
        RtpExtEntry{ id: 3, data: ast_bytes[0..] },
        RtpExtEntry{ id: 5, data: twcc_bytes[0..] },
    ];

    let wire: [64]u8 = [0; 64];
    const ext_len = encodeRtpHeaderExtensions(RtpExtForm.one_byte, entries[0..], wire[0..]) catch 0;
    expect_true(ext_len > 0 && (ext_len % 4) == 0);

    let parsed: [8]RtpExtEntry = [RtpExtEntry{}; 8];
    const cnt = parseRtpHeaderExtensions(RTP_EXT_PROFILE_ONE_BYTE, wire[0..ext_len], parsed[0..]) catch 0;
    expect_eq(cnt, 4);

    expect_eq(parsed[0].id, 1);
    expect_true(std.mem.eql(u8, parsed[0].data, "audio0"));

    const al = parseRtpExtAudioLevel(parsed[1].data).?;
    expect_true(al.voice_activity);
    expect_eq(al.level_dbov, 42);

    expect_eq(parseRtpExtAbsSendTime(parsed[2].data).?, 0x00AB_CDEF);
    expect_eq(parseRtpExtTransportCc(parsed[3].data).?, 0x1234);
}

test("str0m_ref_03_rtp_ext_one_byte_skips_zero_padding_and_stops_on_id15") {
    // 0x00 padding before/between entries, ID 1 (len 2 = 0x11), 0x00 pad, ID 15 terminator (0xF0), trailing ignored ID 2
    const raw: [12]u8 = [
        0x00, 0x00,
        0x11, 0xAA, 0xBB,
        0x00,
        0xF0,             // ID=15 terminator!
        0x20, 0xCC,       // MUST NOT be parsed
        0x00, 0x00, 0x00,
    ];
    let parsed: [4]RtpExtEntry = [RtpExtEntry{}; 4];
    const cnt = parseRtpHeaderExtensions(RTP_EXT_PROFILE_ONE_BYTE, raw[0..], parsed[0..]) catch 99;
    expect_eq(cnt, 1);
    expect_eq(parsed[0].id, 1);
    expect_eq(parsed[0].data.len, 2);
    expect_eq(parsed[0].data[0], 0xAA);
    expect_eq(parsed[0].data[1], 0xBB);
}

test("str0m_ref_04_rtp_ext_two_byte_large_id_and_zero_len_payload") {
    const entries: [3]RtpExtEntry = [
        RtpExtEntry{ id: 16, data: "0123456789ABCDEF0123" }, // 20 bytes (> 16, requires two-byte)
        RtpExtEntry{ id: 200, data: "" },                    // 0-byte payload allowed in two-byte
        RtpExtEntry{ id: 255, data: "ok" },
    ];
    let wire: [64]u8 = [0; 64];
    const ext_len = encodeRtpHeaderExtensions(RtpExtForm.two_byte, entries[0..], wire[0..]) catch 0;
    expect_true(ext_len > 0 && (ext_len % 4) == 0);

    let parsed: [8]RtpExtEntry = [RtpExtEntry{}; 8];
    const cnt = parseRtpHeaderExtensions(0x1005, wire[0..ext_len], parsed[0..]) catch 0;
    expect_eq(cnt, 3);
    expect_eq(parsed[0].id, 16);
    expect_true(std.mem.eql(u8, parsed[0].data, "0123456789ABCDEF0123"));
    expect_eq(parsed[1].id, 200);
    expect_eq(parsed[1].data.len, 0);
    expect_eq(parsed[2].id, 255);
    expect_true(std.mem.eql(u8, parsed[2].data, "ok"));
}

test("str0m_ref_05_rtp_ext_validation_rejects_invalid_one_byte_ids_and_truncation") {
    const bad_id: [1]RtpExtEntry = [RtpExtEntry{ id: 15, data: "a" }];
    let wire: [32]u8 = [0; 32];
    let failed_id = false;
    * = encodeRtpHeaderExtensions(RtpExtForm.one_byte, bad_id[0..], wire[0..]) catch { failed_id = true; };
    expect_true(failed_id);

    const bad_empty: [1]RtpExtEntry = [RtpExtEntry{ id: 1, data: "" }];
    let failed_empty = false;
    * = encodeRtpHeaderExtensions(RtpExtForm.one_byte, bad_empty[0..], wire[0..]) catch { failed_empty = true; };
    expect_true(failed_empty);

    // Truncated one-byte element declares len=5 (0x14) but only 2 bytes remain -> stops cleanly
    const trunc: [3]u8 = [0x14, 0x01, 0x02];
    let it = RtpExtIter.init(RTP_EXT_PROFILE_ONE_BYTE, trunc[0..]);
    expect_true(it.next() == null);
}

test("str0m_ref_06_rtp_packet_with_extensions_csrc_and_padding_e2e") {
    let hdr = RtpHeader{
        version: RTP_VERSION,
        padding: true,
        extension: true,
        csrc_count: 2,
        marker: true,
        payload_type: 96,
        seq: 4321,
        timestamp: 90000,
        ssrc: 0xDEAD_BEEF,
    };
    hdr.csrc[0] = 0x1111_2222;
    hdr.csrc[1] = 0x3333_4444;

    const exts: [2]RtpExtEntry = [
        RtpExtEntry{ id: 4, data: "video-main" },
        RtpExtEntry{ id: 7, data: "\x00\x2A" },
    ];
    let pkt: [128]u8 = [0; 128];
    let pkt_slice: []u8 = pkt[0..];
    const n = encodeRtpPacketWithExtensions(hdr, RtpExtForm.one_byte, exts[0..], "VP9DATA", *pkt_slice) catch 0;
    expect_true(n > 0 && (n % 4) == 0);

    const p = parseRtpPacket(pkt[0..n]) catch unreachable;
    expect_true(p.extension);
    expect_true(p.padding);
    expect_true(p.marker);
    expect_eq(p.csrc_count, 2);
    expect_eq(p.csrc[0], 0x1111_2222);
    expect_eq(p.csrc[1], 0x3333_4444);
    expect_eq(p.ext_profile, RTP_EXT_PROFILE_ONE_BYTE);
    expect_true(std.mem.eql(u8, pkt[p.payload_offset .. p.payload_offset + p.payload_len], "VP9DATA"));

    let out_exts: [4]RtpExtEntry = [RtpExtEntry{}; 4];
    const en = parseRtpHeaderExtensions(p.ext_profile, pkt[p.ext_offset .. p.ext_offset + p.ext_len], out_exts[0..]) catch 0;
    expect_eq(en, 2);
    expect_eq(out_exts[0].id, 4);
    expect_true(std.mem.eql(u8, out_exts[0].data, "video-main"));
    expect_eq(parseRtpExtTransportCc(out_exts[1].data).?, 42);
}

test("str0m_ref_07_rtp_padding_malformed_guards") {
    // 1. P=1 with pad_len == 0 -> InvalidPadding
    let p_zero: [16]u8 = [0; 16];
    p_zero[0] = 0xA0; // V=2, P=1
    p_zero[15] = 0;
    let err1 = false;
    * = parseRtpPacket(p_zero[0..]) catch |e| {
        if (e == RtpMediaError.InvalidPadding) err1 = true;
    };
    expect_true(err1);

    // 2. P=1 with pad_len > available payload (5 > 4) -> InvalidPadding
    let p_over: [16]u8 = [0; 16];
    p_over[0] = 0xA0;
    p_over[15] = 5;
    let err2 = false;
    * = parseRtpPacket(p_over[0..]) catch |e| {
        if (e == RtpMediaError.InvalidPadding) err2 = true;
    };
    expect_true(err2);

    // 3. P=1 with zero payload bytes after 12B header -> InvalidPadding
    let p_empty: [12]u8 = [0; 12];
    p_empty[0] = 0xA0;
    let err3 = false;
    * = parseRtpPacket(p_empty[0..]) catch |e| {
        if (e == RtpMediaError.InvalidPadding) err3 = true;
    };
    expect_true(err3);

    // 4. P=1 where entire 4-byte payload is padding (pad_len == 4) -> valid, payload_len == 0
    let p_allpad: [16]u8 = [0; 16];
    p_allpad[0] = 0xA0;
    p_allpad[15] = 4;
    const ok = parseRtpPacket(p_allpad[0..]) catch unreachable;
    expect_eq(ok.payload_len, 0);
}

// ============================================================================
// Part B: Compound RTCP & WebRTC Control Feedback (NACK / PLI / FIR)
// ============================================================================

test("str0m_ref_08_rtcp_nack_encode_parse_and_blp_expansion_wrap") {
    // Ported directly from str0m/src/rtp/rtcp/nack.rs test::nack_entry_iter
    const pid: u16 = 65530;
    // Bit 0 (+1 = 65531), Bit 3 (+4 = 65534), Bit 6 (+7 = 1 wrapped!), Bit 15 (+16 = 10 wrapped!)
    const blp: u16 = 0b1000_0000_0100_1001;
    const entries: [1]RtcpNackEntry = [RtcpNackEntry{ pid: pid, blp: blp }];

    let wire: [32]u8 = [0; 32];
    const n = encodeRtcpNack(0x1122_3344, 0x5566_7788, entries[0..], wire[0..]) catch 0;
    expect_eq(n, 16);
    expect_eq(wire[0], 0x81); // V=2, FMT=1
    expect_eq(wire[1], RTCP_PT_RTPFB);

    let out_e: [4]RtcpNackEntry = [RtcpNackEntry{}; 4];
    const hdr = parseRtcpNack(wire[0..n], out_e[0..]) catch unreachable;
    expect_eq(hdr.sender_ssrc, 0x1122_3344);
    expect_eq(hdr.media_ssrc, 0x5566_7788);
    expect_eq(hdr.entry_count, 1);

    let seqs: [17]u16 = [0; 17];
    const sc = expandRtcpNackEntry(out_e[0], seqs[0..]);
    expect_eq(sc, 5);
    expect_eq(seqs[0], 65530);
    expect_eq(seqs[1], 65531);
    expect_eq(seqs[2], 65534);
    expect_eq(seqs[3], 1);
    expect_eq(seqs[4], 10);
}

test("str0m_ref_09_rtcp_nack_multi_fci_and_malformed_rejection") {
    const entries: [3]RtcpNackEntry = [
        RtcpNackEntry{ pid: 100, blp: 0x0001 },
        RtcpNackEntry{ pid: 200, blp: 0x0000 },
        RtcpNackEntry{ pid: 300, blp: 0xFFFF },
    ];
    let wire: [64]u8 = [0; 64];
    const n = encodeRtcpNack(0xAAAA_0001, 0xBBBB_0002, entries[0..], wire[0..]) catch 0;
    expect_eq(n, 24);

    let out_e: [4]RtcpNackEntry = [RtcpNackEntry{}; 4];
    const pkt = parseRtcpNack(wire[0..n], out_e[0..]) catch unreachable;
    expect_eq(pkt.entry_count, 3);
    expect_eq(out_e[2].pid, 300);
    expect_eq(out_e[2].blp, 0xFFFF);

    // Expanding blp=0xFFFF yields all 17 sequence numbers (300..316)
    let seqs: [17]u16 = [0; 17];
    expect_eq(expandRtcpNackEntry(out_e[2], seqs[0..]), 17);
    expect_eq(seqs[16], 316);

    // Wrong FMT (FMT=2 instead of 1) -> InvalidRtcpLength
    wire[0] = 0x82;
    let rejected = false;
    * = parseRtcpNack(wire[0..n], out_e[0..]) catch { rejected = true; };
    expect_true(rejected);
}

test("str0m_ref_10_rtcp_pli_encode_parse_and_validation") {
    let wire: [16]u8 = [0; 16];
    const n = encodeRtcpPli(RtcpPli{ sender_ssrc: 0x1020_3040, media_ssrc: 0x5060_7080 }, wire[0..]) catch 0;
    expect_eq(n, 12);
    expect_eq(wire[0], 0x81); // V=2, FMT=1
    expect_eq(wire[1], RTCP_PT_PSFB);

    const pli = parseRtcpPli(wire[0..n]) catch unreachable;
    expect_eq(pli.sender_ssrc, 0x1020_3040);
    expect_eq(pli.media_ssrc, 0x5060_7080);

    // Truncated 11-byte PLI -> PacketTooShort
    let short_err = false;
    * = parseRtcpPli(wire[0..11]) catch |e| {
        if (e == RtpMediaError.PacketTooShort) short_err = true;
    };
    expect_true(short_err);
}

test("str0m_ref_11_rtcp_fir_multi_entry_roundtrip") {
    const entries: [2]RtcpFirEntry = [
        RtcpFirEntry{ ssrc: 0x1111_1111, seq_nr: 7 },
        RtcpFirEntry{ ssrc: 0x2222_2222, seq_nr: 255 },
    ];
    let wire: [64]u8 = [0; 64];
    const n = encodeRtcpFir(0x9999_8888, entries[0..], wire[0..]) catch 0;
    expect_eq(n, 28); // 12 + 2*8
    expect_eq(wire[0], 0x84); // V=2, FMT=4
    expect_eq(wire[1], RTCP_PT_PSFB);

    let parsed: [4]RtcpFirEntry = [RtcpFirEntry{}; 4];
    const fir = parseRtcpFir(wire[0..n], parsed[0..]) catch unreachable;
    expect_eq(fir.sender_ssrc, 0x9999_8888);
    expect_eq(fir.entry_count, 2);
    expect_eq(parsed[0].ssrc, 0x1111_1111);
    expect_eq(parsed[0].seq_nr, 7);
    expect_eq(parsed[1].ssrc, 0x2222_2222);
    expect_eq(parsed[1].seq_nr, 255);
}

test("str0m_ref_12_rtcp_compound_iterator_sr_rr_nack_pli_fir") {
    let compound: [256]u8 = [0; 256];
    let off: usize = 0;

    // 1. SR (28B)
    let sr_slice: []u8 = compound[off..];
    off += encodeRtcpSr(RtcpSenderReport{ ssrc: 0x1000_0001, packet_count: 50 }, *sr_slice) catch unreachable;
    // 2. RR (32B)
    let rr_slice: []u8 = compound[off..];
    off += encodeRtcpRr(0x2000_0002, RtcpReportBlock{ ssrc: 0x1000_0001, fraction_lost: 12 }, *rr_slice) catch unreachable;
    // 3. NACK (16B)
    const nacks: [1]RtcpNackEntry = [RtcpNackEntry{ pid: 500, blp: 0x0003 }];
    off += encodeRtcpNack(0x2000_0002, 0x1000_0001, nacks[0..], compound[off..]) catch unreachable;
    // 4. PLI (12B)
    off += encodeRtcpPli(RtcpPli{ sender_ssrc: 0x2000_0002, media_ssrc: 0x1000_0001 }, compound[off..]) catch unreachable;
    // 5. FIR (20B)
    const firs: [1]RtcpFirEntry = [RtcpFirEntry{ ssrc: 0x1000_0001, seq_nr: 3 }];
    off += encodeRtcpFir(0x2000_0002, firs[0..], compound[off..]) catch unreachable;

    let it = RtcpCompoundIter.init(compound[0..off]);
    const v0 = it.next().?;
    expect_eq(v0.packet_type, RTCP_PT_SR);
    expect_eq((parseRtcpSr(v0.unpadded) catch unreachable).packet_count, 50);

    const v1 = it.next().?;
    expect_eq(v1.packet_type, RTCP_PT_RR);
    let rr_ssrc: u32 = 0;
    expect_eq((parseRtcpRr(v1.unpadded, &rr_ssrc) catch unreachable).fraction_lost, 12);

    const v2 = it.next().?;
    expect_eq(v2.packet_type, RTCP_PT_RTPFB);
    expect_eq(v2.count_or_fmt, RTCP_FMT_NACK);

    const v3 = it.next().?;
    expect_eq(v3.packet_type, RTCP_PT_PSFB);
    expect_eq(v3.count_or_fmt, RTCP_FMT_PLI);

    const v4 = it.next().?;
    expect_eq(v4.packet_type, RTCP_PT_PSFB);
    expect_eq(v4.count_or_fmt, RTCP_FMT_FIR);

    expect_true(it.next() == null);
    expect_false(it.had_error);
}

test("str0m_ref_13_rtcp_compound_padding_and_malformed_guards") {
    // Valid PLI with 4 bytes of trailing padding (total 16B, length=3 words)
    let padded: [16]u8 = [0; 16];
    padded[0] = 0xA1; // V=2, P=1, FMT=1
    padded[1] = RTCP_PT_PSFB;
    padded[2] = 0;
    padded[3] = 3; // 4 words = 16 bytes
    padded[15] = 4; // 4 padding bytes

    let it = RtcpCompoundIter.init(padded[0..]);
    const v = it.next().?;
    expect_true(v.padding);
    expect_eq(v.raw.len, 16);
    expect_eq(v.unpadded.len, 12);
    expect_false(it.had_error);

    // Malformed padding: pad_len == 0 -> had_error = true
    padded[15] = 0;
    let it_bad = RtcpCompoundIter.init(padded[0..]);
    expect_true(it_bad.next() == null);
    expect_true(it_bad.had_error);

    // Malformed declared length exceeding buffer -> had_error = true
    padded[0] = 0x81;
    padded[3] = 10; // 44 bytes > 16
    let it_trunc = RtcpCompoundIter.init(padded[0..]);
    expect_true(it_trunc.next() == null);
    expect_true(it_trunc.had_error);
}

// ============================================================================
// Part C: SRTP / SRTCP Hardened Edge Cases (vs str0m/src/rtp/srtp.rs)
// ============================================================================

test("str0m_ref_14_srtp_min_length_and_version_guards") {
    let ctx = SrtpContext.init(makeTestMasterKey(), SrtpProfile.aes128_cm_hmac_sha1_80);
    let out: [128]u8 = [0; 128];

    // 21-byte SRTP packet (< 12B RTP + 10B HMAC-SHA1-80 tag) -> PacketTooShort
    let short_srtp: [21]u8 = [0; 21];
    short_srtp[0] = 0x80;
    let err_short = false;
    * = ctx.unprotectRtp(short_srtp[0..], out[0..]) catch |e| {
        if (e == SrtpError.PacketTooShort) err_short = true;
    };
    expect_true(err_short);

    // RTP version != 2 (V=1 -> 0x40) -> rejected in protectRtp
    let bad_ver: [16]u8 = [0; 16];
    bad_ver[0] = 0x40;
    let err_ver = false;
    * = ctx.protectRtp(bad_ver[0..], out[0..]) catch |e| {
        if (e == SrtpError.PacketTooShort) err_ver = true;
    };
    expect_true(err_ver);
}

test("str0m_ref_15_srtp_extension_header_bounds_guard") {
    let ctx = SrtpContext.init(makeTestMasterKey(), SrtpProfile.aes128_cm_hmac_sha1_80);
    let out: [128]u8 = [0; 128];

    // RTP with X=1 declaring 10 extension words (40 bytes) in a 20-byte packet -> PacketTooShort
    let bad_ext: [20]u8 = [0; 20];
    bad_ext[0] = 0x90; // V=2, X=1
    bad_ext[12] = 0xBE;
    bad_ext[13] = 0xDE;
    bad_ext[14] = 0x00;
    bad_ext[15] = 10; // 40 bytes > remaining 4 bytes
    let rejected = false;
    * = ctx.protectRtp(bad_ext[0..], out[0..]) catch |e| {
        if (e == SrtpError.PacketTooShort) rejected = true;
    };
    expect_true(rejected);
}

test("str0m_ref_16_srtp_roc_wrap_65535_to_0_and_cross_roc_reorder") {
    const key = makeTestMasterKey();
    let tx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);

    let rtp: [64]u8 = [0; 64];
    let srtp_pre: [64]u8 = [0; 64];
    let srtp_cur: [64]u8 = [0; 64];
    let plain_buf: [64]u8 = [0; 64];

    // Send seq 65533, hold seq 65534 for cross-ROC reordering, send 65535, 0, 1, 2
    const r0 = buildMinimalRtp(65533, 1000, 0xCAFE_0001, "pkt-65533", rtp[0..]);
    const s0 = tx.protectRtp(rtp[0..r0], srtp_cur[0..]) catch unreachable;
    * = rx.unprotectRtp(srtp_cur[0..s0], plain_buf[0..]) catch unreachable;
    expect_eq(rx.rxRoc(), 0);

    // Protect seq 65534 (ROC=0) but delay its delivery until AFTER ROC increments to 1
    const r_hold = buildMinimalRtp(65534, 1160, 0xCAFE_0001, "pkt-65534-late", rtp[0..]);
    const s_hold = tx.protectRtp(rtp[0..r_hold], srtp_pre[0..]) catch unreachable;

    const seqs: [4]u16 = [65535, 0, 1, 2];
    let i: usize = 0;
    while (i < seqs.len) : (i += 1) {
        const rl = buildMinimalRtp(seqs[i], 1320 + @as(u32, @intCast(i)) * 160, 0xCAFE_0001, "wrap-ok", rtp[0..]);
        const sl = tx.protectRtp(rtp[0..rl], srtp_cur[0..]) catch unreachable;
        const dl = rx.unprotectRtp(srtp_cur[0..sl], plain_buf[0..]) catch unreachable;
        expect_true(std.mem.eql(u8, plain_buf[12..dl], "wrap-ok"));
    }
    expect_eq(tx.txRoc(), 1);
    expect_eq(rx.rxRoc(), 1);

    // Now deliver delayed seq 65534 (belongs to ROC=0 while rx is at ROC=1, seq=2 -> delta=4 < 64)
    const d_hold = rx.unprotectRtp(srtp_pre[0..s_hold], plain_buf[0..]) catch unreachable;
    expect_true(std.mem.eql(u8, plain_buf[12..d_hold], "pkt-65534-late"));
    expect_eq(rx.rxRoc(), 1);
}

test("str0m_ref_17_srtp_roc_zero_underflow_rejected_as_packet_too_old") {
    const key = makeTestMasterKey();
    let tx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);

    let rtp: [64]u8 = [0; 64];
    let srtp: [64]u8 = [0; 64];
    let plain_buf: [64]u8 = [0; 64];

    // Establish rx at ROC=0, seq=10
    const r0 = buildMinimalRtp(10, 100, 0x1234_5678, "init", rtp[0..]);
    const s0 = tx.protectRtp(rtp[0..r0], srtp[0..]) catch unreachable;
    * = rx.unprotectRtp(srtp[0..s0], plain_buf[0..]) catch unreachable;

    // A forged packet with seq=65530 while rx is at ROC=0, s_l=10 implies backward wrap below ROC 0
    const r_bad = buildMinimalRtp(65530, 50, 0x1234_5678, "underflow", rtp[0..]);
    let tx_other = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);
    const s_bad = tx_other.protectRtp(rtp[0..r_bad], srtp[0..]) catch unreachable;

    let rejected_old = false;
    * = rx.unprotectRtp(srtp[0..s_bad], plain_buf[0..]) catch |e| {
        if (e == SrtpError.PacketTooOld) rejected_old = true;
    };
    expect_true(rejected_old);
}

test("str0m_ref_18_srtp_large_forward_jump_advances_replay_window") {
    const key = makeTestMasterKey();
    let tx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);

    let rtp: [64]u8 = [0; 64];
    let srtp_old: [64]u8 = [0; 64];
    let srtp: [64]u8 = [0; 64];
    let plain_buf: [64]u8 = [0; 64];

    const r1 = buildMinimalRtp(100, 1000, 0xABCD_0001, "p100", rtp[0..]);
    const s1 = tx.protectRtp(rtp[0..r1], srtp[0..]) catch unreachable;
    * = rx.unprotectRtp(srtp[0..s1], plain_buf[0..]) catch unreachable;

    // Hold packet 101
    const r101 = buildMinimalRtp(101, 1160, 0xABCD_0001, "p101", rtp[0..]);
    const s101 = tx.protectRtp(rtp[0..r101], srtp_old[0..]) catch unreachable;

    // Jump forward by 5000 packets (< 32768)
    const r5100 = buildMinimalRtp(5100, 800000, 0xABCD_0001, "p5100", rtp[0..]);
    const s5100 = tx.protectRtp(rtp[0..r5100], srtp[0..]) catch unreachable;
    const d5100 = rx.unprotectRtp(srtp[0..s5100], plain_buf[0..]) catch unreachable;
    expect_true(std.mem.eql(u8, plain_buf[12..d5100], "p5100"));
    expect_eq(rx.rxRtpHighestIndex(), 5100);

    // Delayed packet 101 is now > 64 behind 5100 -> PacketTooOld
    let too_old = false;
    * = rx.unprotectRtp(srtp_old[0..s101], plain_buf[0..]) catch |e| {
        if (e == SrtpError.PacketTooOld) too_old = true;
    };
    expect_true(too_old);
}

test("str0m_ref_19_srtcp_e0_plaintext_auth_vs_e1_encrypted_dual_path") {
    const key = makeTestMasterKey();
    let tx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);

    // Build a 28-byte RTCP SR
    let rtcp: [28]u8 = [0; 28];
    let rtcp_s: []u8 = rtcp[0..];
    * = encodeRtcpSr(RtcpSenderReport{
        ssrc: 0x3344_5566,
        ntp_sec: 0x1122_3344,
        ntp_frac: 0x5566_7788,
        rtp_timestamp: 90000,
        packet_count: 123,
        octet_count: 45678,
    }, *rtcp_s) catch unreachable;

    let wire: [64]u8 = [0; 64];
    let plain_buf: [64]u8 = [0; 64];

    // 1. E=0 plaintext-authenticated SRTCP
    const n0 = tx.protectRtcpWithOptions(rtcp[0..], false, wire[0..]) catch unreachable;
    expect_eq(n0, 28 + 4 + 10);
    expect_eq(wire[28] & 0x80, 0); // E-bit == 0
    // Bytes 8..28 must remain plaintext on wire
    expect_true(std.mem.eql(u8, wire[8..28], rtcp[8..28]));
    const d0 = rx.unprotectRtcp(wire[0..n0], plain_buf[0..]) catch unreachable;
    expect_eq(d0, 28);
    expect_true(std.mem.eql(u8, plain_buf[0..d0], rtcp[0..]));

    // 2. E=1 encrypted + authenticated SRTCP in the same session
    const n1 = tx.protectRtcpWithOptions(rtcp[0..], true, wire[0..]) catch unreachable;
    expect_eq(n1, 28 + 4 + 10);
    expect_true((wire[28] & 0x80) != 0); // E-bit == 1
    // Bytes 8..28 must be encrypted (differ from plaintext)
    expect_false(std.mem.eql(u8, wire[8..28], rtcp[8..28]));
    const d1 = rx.unprotectRtcp(wire[0..n1], plain_buf[0..]) catch unreachable;
    expect_eq(d1, 28);
    expect_true(std.mem.eql(u8, plain_buf[0..d1], rtcp[0..]));
    expect_eq(tx.txSrtcpIndex(), 2);
}

test("str0m_ref_20_srtcp_e0_tamper_detected_by_hmac") {
    const key = makeTestMasterKey();
    let tx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);

    let rtcp: [12]u8 = [0; 12];
    * = encodeRtcpPli(RtcpPli{ sender_ssrc: 0xAAAA_BBBB, media_ssrc: 0xCCCC_DDDD }, rtcp[0..]) catch unreachable;

    let wire: [64]u8 = [0; 64];
    let plain_buf: [64]u8 = [0; 64];
    const n = tx.protectRtcpWithOptions(rtcp[0..], false, wire[0..]) catch unreachable;

    // Flip 1 bit in E=0 plaintext body -> must fail AuthTagMismatch
    wire[10] ^= 0x01;
    let auth_fail = false;
    * = rx.unprotectRtcp(wire[0..n], plain_buf[0..]) catch |e| {
        if (e == SrtpError.AuthTagMismatch) auth_fail = true;
    };
    expect_true(auth_fail);

    // Flip 1 bit in E-flag -> must also fail AuthTagMismatch (E-bit is covered by HMAC)
    wire[10] ^= 0x01;
    wire[12] ^= 0x80;
    let ebit_fail = false;
    * = rx.unprotectRtcp(wire[0..n], plain_buf[0..]) catch |e| {
        if (e == SrtpError.AuthTagMismatch) ebit_fail = true;
    };
    expect_true(ebit_fail);
}

test("str0m_ref_21_srtcp_min_length_and_version_guards") {
    let ctx = SrtpContext.init(makeTestMasterKey(), SrtpProfile.aes128_cm_hmac_sha1_80);
    let out: [64]u8 = [0; 64];

    // 21 bytes (< 8B RTCP + 4B index + 10B auth tag = 22B) -> PacketTooShort
    let short_srtcp: [21]u8 = [0; 21];
    short_srtcp[0] = 0x80;
    let err_short = false;
    * = ctx.unprotectRtcp(short_srtcp[0..], out[0..]) catch |e| {
        if (e == SrtpError.PacketTooShort) err_short = true;
    };
    expect_true(err_short);

    // Invalid RTCP version (V=0) -> PacketTooShort
    let bad_ver: [24]u8 = [0; 24];
    bad_ver[0] = 0x00;
    let err_ver = false;
    * = ctx.unprotectRtcp(bad_ver[0..], out[0..]) catch |e| {
        if (e == SrtpError.PacketTooShort) err_ver = true;
    };
    expect_true(err_ver);
}

test("str0m_ref_22_srtcp_independent_64bit_replay_window") {
    const key = makeTestMasterKey();
    let tx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);

    let pli: [12]u8 = [0; 12];
    * = encodeRtcpPli(RtcpPli{ sender_ssrc: 0x0102_0304, media_ssrc: 0x0506_0708 }, pli[0..]) catch unreachable;

    let wire0: [32]u8 = [0; 32];
    let wire1: [32]u8 = [0; 32];
    let wire_tmp: [32]u8 = [0; 32];
    let plain_buf: [32]u8 = [0; 32];

    const n0 = tx.protectRtcp(pli[0..], wire0[0..]) catch unreachable; // idx 0
    const n1 = tx.protectRtcp(pli[0..], wire1[0..]) catch unreachable; // idx 1

    // Deliver idx 1 first, then out-of-order idx 0 -> both accepted
    * = rx.unprotectRtcp(wire1[0..n1], plain_buf[0..]) catch unreachable;
    * = rx.unprotectRtcp(wire0[0..n0], plain_buf[0..]) catch unreachable;

    // Replay idx 0 -> ReplayDetected
    let replayed = false;
    * = rx.unprotectRtcp(wire0[0..n0], plain_buf[0..]) catch |e| {
        if (e == SrtpError.ReplayDetected) replayed = true;
    };
    expect_true(replayed);

    // Advance SRTCP index by 70 packets (idx 2..71)
    let k: usize = 0;
    while (k < 70) : (k += 1) {
        const nk = tx.protectRtcp(pli[0..], wire_tmp[0..]) catch unreachable;
        if (k == 69) {
            * = rx.unprotectRtcp(wire_tmp[0..nk], plain_buf[0..]) catch unreachable;
        }
    }
    expect_eq(rx.rxRtcpHighestIndex(), 71);

    // Re-sending idx 1 (delta = 70 >= 64) -> PacketTooOld
    let old_err = false;
    * = rx.unprotectRtcp(wire1[0..n1], plain_buf[0..]) catch |e| {
        if (e == SrtpError.PacketTooOld) old_err = true;
    };
    expect_true(old_err);
}

test("str0m_ref_23_srtp_hmac_sha1_32_short_tag_profile") {
    const key = makeTestMasterKey();
    let tx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_32);
    let rx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_32);

    let rtp: [64]u8 = [0; 64];
    let srtp: [64]u8 = [0; 64];
    let plain_buf: [64]u8 = [0; 64];

    const rlen = buildMinimalRtp(777, 123456, 0xFACE_B00C, "audio-32tag", rtp[0..]);
    const slen = tx.protectRtp(rtp[0..rlen], srtp[0..]) catch unreachable;
    expect_eq(slen, rlen + 4); // 4-byte tag

    const dlen = rx.unprotectRtp(srtp[0..slen], plain_buf[0..]) catch unreachable;
    expect_eq(dlen, rlen);
    expect_true(std.mem.eql(u8, plain_buf[0..dlen], rtp[0..rlen]));

    // 15-byte packet (< 12B + 4B = 16B) -> PacketTooShort
    let err_short = false;
    * = rx.unprotectRtp(srtp[0..15], plain_buf[0..]) catch |e| {
        if (e == SrtpError.PacketTooShort) err_short = true;
    };
    expect_true(err_short);
}

test("str0m_ref_24_srtp_with_rfc8285_extensions_end_to_end") {
    const key = makeTestMasterKey();
    let tx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);

    let hdr = RtpHeader{
        version: RTP_VERSION,
        extension: true,
        marker: true,
        payload_type: 111,
        seq: 1001,
        timestamp: 48000,
        ssrc: 0x0BAD_F00D,
    };
    const exts: [1]RtpExtEntry = [RtpExtEntry{ id: 1, data: "mid-audio" }];
    let rtp: [128]u8 = [0; 128];
    let rtp_slice: []u8 = rtp[0..];
    const rlen = encodeRtpPacketWithExtensions(hdr, RtpExtForm.one_byte, exts[0..], "OPUSFRAME", *rtp_slice) catch unreachable;

    let srtp: [160]u8 = [0; 160];
    let plain_buf: [128]u8 = [0; 128];
    const slen = tx.protectRtp(rtp[0..rlen], srtp[0..]) catch unreachable;
    // RFC 3711: RTP header & RFC 8285 extensions stay in clear (authenticated), only payload encrypted
    const parsed_hdr = parseRtpPacket(rtp[0..rlen]) catch unreachable;
    expect_true(std.mem.eql(u8, srtp[0..parsed_hdr.header_len], rtp[0..parsed_hdr.header_len]));
    expect_false(std.mem.eql(u8, srtp[parsed_hdr.payload_offset .. parsed_hdr.payload_offset + parsed_hdr.payload_len], "OPUSFRAME"));

    const dlen = rx.unprotectRtp(srtp[0..slen], plain_buf[0..]) catch unreachable;
    expect_eq(dlen, rlen);
    const dec_hdr = parseRtpPacket(plain_buf[0..dlen]) catch unreachable;
    expect_true(std.mem.eql(u8, plain_buf[dec_hdr.payload_offset .. dec_hdr.payload_offset + dec_hdr.payload_len], "OPUSFRAME"));
}

test("str0m_ref_25_srtcp_compound_nack_pli_roundtrip") {
    const key = makeTestMasterKey();
    let tx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(key, SrtpProfile.aes128_cm_hmac_sha1_80);

    let comp: [64]u8 = [0; 64];
    const nacks: [1]RtcpNackEntry = [RtcpNackEntry{ pid: 900, blp: 0x0005 }];
    const n1 = encodeRtcpNack(0x1111_0001, 0x2222_0002, nacks[0..], comp[0..]) catch unreachable;
    const n2 = encodeRtcpPli(RtcpPli{ sender_ssrc: 0x1111_0001, media_ssrc: 0x2222_0002 }, comp[n1..]) catch unreachable;
    const total = n1 + n2;

    let wire: [96]u8 = [0; 96];
    let plain_buf: [64]u8 = [0; 64];
    const slen = tx.protectRtcp(comp[0..total], wire[0..]) catch unreachable;
    const dlen = rx.unprotectRtcp(wire[0..slen], plain_buf[0..]) catch unreachable;
    expect_eq(dlen, total);

    let it = RtcpCompoundIter.init(plain_buf[0..dlen]);
    const p0 = it.next().?;
    expect_eq(p0.packet_type, RTCP_PT_RTPFB);
    let out_n: [2]RtcpNackEntry = [RtcpNackEntry{}; 2];
    const np = parseRtcpNack(p0.unpadded, out_n[0..]) catch unreachable;
    expect_eq(np.entry_count, 1);
    expect_eq(out_n[0].pid, 900);

    const p1 = it.next().?;
    expect_eq(p1.packet_type, RTCP_PT_PSFB);
    const pli = parseRtcpPli(p1.unpadded) catch unreachable;
    expect_eq(pli.media_ssrc, 0x2222_0002);
    expect_true(it.next() == null);
}

test("str0m_ref_26_estimate_srtp_packet_index_boundaries") {
    // Normal forward
    expect_eq(estimateSrtpPacketIndex(0, 100, 101), 101);
    // Forward wrap 65535 -> 0 increments ROC from 0 to 1
    expect_eq(estimateSrtpPacketIndex(0, 65535, 0), 65536);
    // Backward wrap across ROC boundary (roc=2, s_l=5, seq=65530 -> roc=1)
    expect_eq(estimateSrtpPacketIndex(2, 5, 65530), (@as(u64, 1) << 16) | 65530);
    // Forward wrap at roc=5
    expect_eq(estimateSrtpPacketIndex(5, 65000, 10), (@as(u64, 6) << 16) | 10);
}
