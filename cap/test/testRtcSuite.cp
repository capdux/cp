// cap/test/testRtcSuite.cp
// Comprehensive test & deterministic PRNG fuzz suite for cap:rtc (Sans-IO WebRTC core)

import { test, expect_eq } from "cap:test";
import {
    RTP_VERSION,
    RTP_MIN_HEADER_LEN,
    RTCP_PT_SR,
    RTCP_PT_RR,
    RTCP_PT_SDES,
    RTCP_PT_BYE,
    RTCP_PT_RTPFB,
    RTCP_FMT_NACK,
    RTCP_NACK_PACKET_LEN,
    STUN_MAGIC_COOKIE,
    STUN_BINDING_REQUEST,
    STUN_BINDING_SUCCESS,
    DEMUX_UNKNOWN,
    DEMUX_STUN,
    DEMUX_DTLS,
    DEMUX_RTP,
    DEMUX_RTCP,
    SDP_MEDIA_AUDIO,
    SDP_MEDIA_VIDEO,
    SDP_MEDIA_APPLICATION,
    SDP_SETUP_ACTPASS,
    SDP_SETUP_ACTIVE,
    SDP_SETUP_PASSIVE,
    RtpHeader,
    parseRtpHeader,
    writeRtpHeader,
    RtcpHeader,
    parseRtcpHeader,
    buildRtcpNack,
    parseRtcpNack,
    demuxPacketKind,
    stunCrc32,
    stunFingerprint,
    encodeXorMappedAddr,
    decodeXorMappedAddr,
    StunMessage,
    buildStunBindingRequest,
    buildStunBindingSuccess,
    parseStunHeader,
    SdpMediaLine,
    SdpSession,
    parseSdp,
    formatSdpOffer,
    SeqTracker,
} from "cap:rtc";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn nextPrng(state: *u64): u64 {
    // XorShift64* deterministic PRNG (constants < 2^63)
    let x = state.*;
    x = x ^ (x >> 12);
    x = x ^ (x << 25);
    x = x ^ (x >> 27);
    state.* = x;
    return x *% 0x2545F4914F6CDD1D;
}

fn fillPrngBytes(state: *u64, buf: []u8, len: usize) {
    let i: usize = 0;
    while (i < len) : (i += 1) {
        buf[i] = (nextPrng(state) & 0xFF) as u8;
    }
}

fn main() {
    test("1. RTP header round-trip (basic, CSRC, extension, padding & zero-copy payload)", () => {
        // Basic 12-byte RTP + 4-byte payload
        let hdr = RtpHeader{
            version: RTP_VERSION,
            marker: true,
            payload_type: 111,
            seq: 0x1234,
            timestamp: 0xAABBCCDD,
            ssrc: 0x11223344,
        };
        let pkt: [64]u8 = [0; 64];
        const written = writeRtpHeader(hdr, pkt[0..]) catch unreachable;
        expect_eq(written, 12 as usize);
        pkt[12] = 0xDE;
        pkt[13] = 0xAD;
        pkt[14] = 0xBE;
        pkt[15] = 0xEF;

        const parsed = parseRtpHeader(pkt[0..16]) catch unreachable;
        expect_eq(parsed.version, 2 as u8);
        expect_eq(parsed.marker, true);
        expect_eq(parsed.payload_type, 111 as u8);
        expect_eq(parsed.seq, 0x1234 as u16);
        expect_eq(parsed.timestamp, 0xAABBCCDD as u32);
        expect_eq(parsed.ssrc, 0x11223344 as u32);
        expect_eq(parsed.header_len, 12 as usize);
        expect_eq(parsed.payload_len, 4 as usize);
        const pay = parsed.payloadSlice(pkt[0..16]);
        expect_eq(pay.len, 4 as usize);
        expect_eq(pay[0], 0xDE as u8);
        expect_eq(pay[3], 0xEF as u8);

        // RTP with 2 CSRCs, 8-byte extension, 5-byte payload, 3-byte padding
        let hdr2 = RtpHeader{
            version: RTP_VERSION,
            padding: true,
            extension: true,
            csrc_count: 2,
            marker: false,
            payload_type: 96,
            seq: 65535,
            timestamp: 90000,
            ssrc: 0xCAFEBABE,
            ext_profile: 0xBEDE,
            ext_len: 8,
        };
        hdr2.csrc[0] = 0x01020304;
        hdr2.csrc[1] = 0x05060708;

        let pkt2: [64]u8 = [0; 64];
        const w2 = writeRtpHeader(hdr2, pkt2[0..]) catch unreachable;
        // 12 + 2*4 + 4 + 8 = 32 bytes header
        expect_eq(w2, 32 as usize);
        // 5-byte payload + 3-byte padding (last byte = 3)
        pkt2[32] = 0x41;
        pkt2[33] = 0x42;
        pkt2[34] = 0x43;
        pkt2[35] = 0x44;
        pkt2[36] = 0x45;
        pkt2[37] = 0x00;
        pkt2[38] = 0x00;
        pkt2[39] = 0x03;

        const p2 = parseRtpHeader(pkt2[0..40]) catch unreachable;
        expect_eq(p2.csrc_count, 2 as u8);
        expect_eq(p2.csrcAt(0), 0x01020304 as u32);
        expect_eq(p2.csrcAt(1), 0x05060708 as u32);
        expect_eq(p2.extension, true);
        expect_eq(p2.ext_profile, 0xBEDE as u16);
        expect_eq(p2.ext_len, 8 as usize);
        expect_eq(p2.header_len, 32 as usize);
        expect_eq(p2.padding, true);
        expect_eq(p2.padding_len, 3 as u8);
        expect_eq(p2.payload_len, 5 as usize);
        expect_eq(sliceEq(p2.payloadSlice(pkt2[0..40]), "ABCDE"), true);
    });

    test("2. RTP malformed & out-of-bounds rejection", () => {
        // Truncated (< 12 bytes)
        let short_pkt: [8]u8 = [0x80, 0x60, 0, 1, 0, 0, 0, 0];
        let failed = false;
        * = parseRtpHeader(short_pkt[0..]) catch { failed = true; };
        expect_eq(failed, true);

        // Wrong version (V=1)
        let bad_ver: [12]u8 = [0x40, 0x60, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1];
        failed = false;
        * = parseRtpHeader(bad_ver[0..]) catch { failed = true; };
        expect_eq(failed, true);

        // CSRC count claims 4 (16 bytes) but packet is only 16 bytes total
        let bad_cc: [16]u8 = [0x84, 0x60, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0];
        failed = false;
        * = parseRtpHeader(bad_cc[0..]) catch { failed = true; };
        expect_eq(failed, true);

        // Extension bit set, claims 10 words (40 bytes) in a 16-byte packet
        let bad_ext: [16]u8 = [0x90, 0x60, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0xBE, 0xDE, 0x00, 0x0A];
        failed = false;
        * = parseRtpHeader(bad_ext[0..]) catch { failed = true; };
        expect_eq(failed, true);

        // Padding bit set with zero padding length
        let bad_pad0: [14]u8 = [0xA0, 0x60, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0x11, 0x00];
        failed = false;
        * = parseRtpHeader(bad_pad0[0..]) catch { failed = true; };
        expect_eq(failed, true);

        // Padding bit set with padding > remaining bytes
        let bad_pad_big: [14]u8 = [0xA0, 0x60, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0x11, 0x05];
        failed = false;
        * = parseRtpHeader(bad_pad_big[0..]) catch { failed = true; };
        expect_eq(failed, true);

        // writeRtpHeader buffer too small
        let tiny: [8]u8 = [0; 8];
        failed = false;
        * = writeRtpHeader(RtpHeader{}, tiny[0..]) catch { failed = true; };
        expect_eq(failed, true);
    });

    test("3. RTCP SR/RR/SDES/BYE parsing & Generic NACK round-trip", () => {
        // RTCP NACK build & parse round-trip
        let nack_buf: [32]u8 = [0; 32];
        const n = buildRtcpNack(0x10203040, 0x50607080, 1024, 0x00A5, nack_buf[0..]) catch unreachable;
        expect_eq(n, RTCP_NACK_PACKET_LEN);

        const hdr = parseRtcpHeader(nack_buf[0..n]) catch unreachable;
        expect_eq(hdr.version, 2 as u8);
        expect_eq(hdr.packet_type, RTCP_PT_RTPFB);
        expect_eq(hdr.count_or_fmt, RTCP_FMT_NACK);
        expect_eq(hdr.isNack(), true);
        expect_eq(hdr.byte_len, 16 as usize);
        expect_eq(hdr.sender_ssrc, 0x10203040 as u32);
        expect_eq(hdr.media_ssrc, 0x50607080 as u32);

        const fci = parseRtcpNack(nack_buf[0..n]) catch unreachable;
        expect_eq(fci.sender_ssrc, 0x10203040 as u32);
        expect_eq(fci.media_ssrc, 0x50607080 as u32);
        expect_eq(fci.base_seq, 1024 as u16);
        expect_eq(fci.bitmask, 0x00A5 as u16);

        // RTCP SR (PT=200), RR (PT=201), SDES (PT=202), BYE (PT=203)
        let sr_pkt: [8]u8 = [0x80, RTCP_PT_SR, 0x00, 0x01, 0xAA, 0xBB, 0xCC, 0xDD];
        const sr = parseRtcpHeader(sr_pkt[0..]) catch unreachable;
        expect_eq(sr.isSr(), true);
        expect_eq(sr.sender_ssrc, 0xAABBCCDD as u32);

        let rr_pkt: [8]u8 = [0x81, RTCP_PT_RR, 0x00, 0x01, 0x01, 0x02, 0x03, 0x04];
        const rr = parseRtcpHeader(rr_pkt[0..]) catch unreachable;
        expect_eq(rr.isRr(), true);
        expect_eq(rr.count_or_fmt, 1 as u8);

        let sdes_pkt: [4]u8 = [0x80, RTCP_PT_SDES, 0x00, 0x00];
        const sdes = parseRtcpHeader(sdes_pkt[0..]) catch unreachable;
        expect_eq(sdes.isSdes(), true);

        let bye_pkt: [8]u8 = [0x81, RTCP_PT_BYE, 0x00, 0x01, 0xDE, 0xAD, 0xBE, 0xEF];
        const bye = parseRtcpHeader(bye_pkt[0..]) catch unreachable;
        expect_eq(bye.isBye(), true);

        // Malformed RTCP: length_words exceeds packet buffer
        let bad_rtcp: [8]u8 = [0x80, RTCP_PT_SR, 0x00, 0x05, 0, 0, 0, 0];
        let failed = false;
        * = parseRtcpHeader(bad_rtcp[0..]) catch { failed = true; };
        expect_eq(failed, true);

        // RFC 7983 demux checks
        expect_eq(demuxPacketKind(sr_pkt[0..]), DEMUX_RTCP);
        let rtp_probe: [2]u8 = [0x80, 0x6F];
        expect_eq(demuxPacketKind(rtp_probe[0..]), DEMUX_RTP);
        let dtls_probe: [2]u8 = [22, 0xFE];
        expect_eq(demuxPacketKind(dtls_probe[0..]), DEMUX_DTLS);
        let stun_probe: [2]u8 = [0x00, 0x01];
        expect_eq(demuxPacketKind(stun_probe[0..]), DEMUX_STUN);
        expect_eq(demuxPacketKind(sr_pkt[0..0]), DEMUX_UNKNOWN);
    });

    test("4. STUN Binding Request/Success, XOR-MAPPED-ADDRESS IPv4 & CRC-32 FINGERPRINT", () => {
        // Standard IEEE CRC-32 check vector: crc32("123456789") == 0xCBF43926
        expect_eq(stunCrc32("123456789"), 0xCBF43926 as u32);

        const txn: [12]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C];
        let req_buf: [64]u8 = [0; 64];
        const req_len = buildStunBindingRequest(txn, req_buf[0..]) catch unreachable;
        expect_eq(req_len, 28 as usize);

        const req = parseStunHeader(req_buf[0..req_len]) catch unreachable;
        expect_eq(req.isBindingRequest(), true);
        expect_eq(req.magic_cookie, STUN_MAGIC_COOKIE);
        expect_eq(req.txn_id[0], 0x01 as u8);
        expect_eq(req.txn_id[11], 0x0C as u8);
        expect_eq(req.has_fingerprint, true);
        expect_eq(req.fingerprint_valid, true);

        // Tamper one byte -> FINGERPRINT must detect corruption
        req_buf[10] = req_buf[10] ^ 0xFF;
        const tampered = parseStunHeader(req_buf[0..req_len]) catch unreachable;
        expect_eq(tampered.has_fingerprint, true);
        expect_eq(tampered.fingerprint_valid, false);

        // Direct XOR-MAPPED-ADDRESS IPv4 round-trip (192.0.2.1:32853 from RFC 5769 style)
        const ip: [4]u8 = [192, 0, 2, 1];
        const port: u16 = 32853;
        let attr_buf: [8]u8 = [0; 8];
        const alen = encodeXorMappedAddr(ip, port, attr_buf[0..]) catch unreachable;
        expect_eq(alen, 8 as usize);
        const decoded = decodeXorMappedAddr(attr_buf[0..alen]) catch unreachable;
        expect_eq(decoded.ip4[0], 192 as u8);
        expect_eq(decoded.ip4[1], 0 as u8);
        expect_eq(decoded.ip4[2], 2 as u8);
        expect_eq(decoded.ip4[3], 1 as u8);
        expect_eq(decoded.port, 32853 as u16);

        // Full STUN Binding Success Response with XOR-MAPPED-ADDRESS + FINGERPRINT
        let resp_buf: [64]u8 = [0; 64];
        const resp_len = buildStunBindingSuccess(txn, [203, 0, 113, 42], 54321, resp_buf[0..]) catch unreachable;
        expect_eq(resp_len, 40 as usize);
        const resp = parseStunHeader(resp_buf[0..resp_len]) catch unreachable;
        expect_eq(resp.isBindingSuccess(), true);
        expect_eq(resp.has_xor_mapped, true);
        expect_eq(resp.xor_mapped_ip4[0], 203 as u8);
        expect_eq(resp.xor_mapped_ip4[1], 0 as u8);
        expect_eq(resp.xor_mapped_ip4[2], 113 as u8);
        expect_eq(resp.xor_mapped_ip4[3], 42 as u8);
        expect_eq(resp.xor_mapped_port, 54321 as u16);
        expect_eq(resp.has_fingerprint, true);
        expect_eq(resp.fingerprint_valid, true);

        // Malformed STUN: bad magic cookie
        resp_buf[4] = 0x00;
        let failed = false;
        * = parseStunHeader(resp_buf[0..resp_len]) catch { failed = true; };
        expect_eq(failed, true);
    });

    test("5. SDP Offer formatting & parsing round-trip (RFC 4566 / RFC 8843)", () => {
        let sess = SdpSession{
            session_id: 9876543210,
            setup: SDP_SETUP_ACTPASS,
        };
        sess.setUfrag("str0mUfrag");
        sess.setPwd("str0mSecretIcePassword123456");
        sess.setFingerprint("sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33");

        let audio = SdpMediaLine{
            kind: SDP_MEDIA_AUDIO,
            port: 9,
            setup: SDP_SETUP_ACTPASS,
        };
        audio.setMid("0");
        * = audio.addPayloadType(111);
        * = audio.addPayloadType(0);
        * = sess.addMedia(audio) catch unreachable;

        let video = SdpMediaLine{
            kind: SDP_MEDIA_VIDEO,
            port: 9,
            setup: SDP_SETUP_ACTIVE,
        };
        video.setMid("1");
        * = video.addPayloadType(96);
        * = video.addPayloadType(98);
        * = sess.addMedia(video) catch unreachable;

        let sdp_buf: [2048]u8 = [0; 2048];
        const sdp_len = formatSdpOffer(sess, sdp_buf[0..]) catch unreachable;
        expect_eq(sdp_len > 100, true);

        const parsed = parseSdp(sdp_buf[0..sdp_len]) catch unreachable;
        expect_eq(parsed.version, 0 as u8);
        expect_eq(parsed.session_id, 9876543210 as u64);
        expect_eq(parsed.media_count, 2 as usize);
        expect_eq(sliceEq(parsed.ufrag(), "str0mUfrag"), true);
        expect_eq(sliceEq(parsed.pwd(), "str0mSecretIcePassword123456"), true);
        expect_eq(sliceEq(parsed.fingerprint(), "sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33"), true);

        expect_eq(parsed.media[0].kind, SDP_MEDIA_AUDIO);
        expect_eq(parsed.media[0].port, 9 as u16);
        expect_eq(sliceEq(parsed.media[0].mid(), "0"), true);
        expect_eq(parsed.media[0].payload_type_count, 2 as usize);
        expect_eq(parsed.media[0].payload_types[0], 111 as u8);
        expect_eq(parsed.media[0].payload_types[1], 0 as u8);
        expect_eq(parsed.media[0].setup, SDP_SETUP_ACTPASS);

        expect_eq(parsed.media[1].kind, SDP_MEDIA_VIDEO);
        expect_eq(sliceEq(parsed.media[1].mid(), "1"), true);
        expect_eq(parsed.media[1].payload_types[0], 96 as u8);
        expect_eq(parsed.media[1].payload_types[1], 98 as u8);
        expect_eq(parsed.media[1].setup, SDP_SETUP_ACTIVE);

        // Reject invalid SDP (missing v=0)
        let failed = false;
        * = parseSdp("o=- 1 2 IN IP4 127.0.0.1\r\ns=-\r\n") catch { failed = true; };
        expect_eq(failed, true);
    });

    test("6. SeqTracker 16-bit wrap-around, reordering, NACK bitmask & loss stats", () => {
        let tr = SeqTracker.init();

        // Start near 16-bit wrap boundary: 65534, 65535, skip 0 & 2, receive 1 & 3
        const r0 = tr.observe(65534);
        expect_eq(r0.is_new, true);
        expect_eq(r0.gap, 0 as u16);

        const r1 = tr.observe(65535);
        expect_eq(r1.is_new, true);
        expect_eq(r1.extended_seq, 65535 as u64);

        // Wrap across 65535 -> 1 (seq 0 missed!)
        const r2 = tr.observe(1);
        expect_eq(r2.is_new, true);
        expect_eq(r2.gap, 1 as u16);
        expect_eq(tr.cycles, 1 as u32);
        expect_eq(r2.extended_seq, 65537 as u64);

        // Advance to 3 (seq 2 missed!)
        const r3 = tr.observe(3);
        expect_eq(r3.is_new, true);
        expect_eq(r3.gap, 1 as u16);

        // Missing: 0 and 2 => NACK base_seq = 0, bitmask bit 1 (0+2) => 0x0002
        const nack1 = tr.buildNack();
        expect_eq(nack1.has_nack, true);
        expect_eq(nack1.base_seq, 0 as u16);
        expect_eq(nack1.bitmask, 0x0002 as u16);

        // Expected 6 packets (65534..3), received 4, lost 2 => 333 permille
        expect_eq(tr.expectedPackets(), 6 as u64);
        expect_eq(tr.lostPackets(), 2 as u64);
        expect_eq(tr.lossRatePermille(), 333 as u32);

        // Build actual RTCP NACK packet from SeqTracker output
        let wire: [16]u8 = [0; 16];
        const wn = buildRtcpNack(0xAAAA0001, 0xBBBB0002, nack1.base_seq, nack1.bitmask, wire[0..]) catch unreachable;
        const parsed_nack = parseRtcpNack(wire[0..wn]) catch unreachable;
        expect_eq(parsed_nack.base_seq, 0 as u16);
        expect_eq(parsed_nack.bitmask, 0x0002 as u16);

        // Out-of-order retransmission of seq 0 arrives
        const r_reord = tr.observe(0);
        expect_eq(r_reord.is_new, true);
        expect_eq(r_reord.is_reordered, true);
        expect_eq(r_reord.extended_seq, 65536 as u64);

        // Duplicate seq 0 arrives
        const r_dup = tr.observe(0);
        expect_eq(r_dup.is_duplicate, true);
        expect_eq(tr.packets_duplicate, 1 as u64);

        // Now only seq 2 is missing => NACK base_seq = 2, bitmask = 0
        const nack2 = tr.buildNack();
        expect_eq(nack2.has_nack, true);
        expect_eq(nack2.base_seq, 2 as u16);
        expect_eq(nack2.bitmask, 0 as u16);

        // Retransmitted seq 2 arrives -> 0 loss, no NACK needed!
        * = tr.observe(2);
        const nack3 = tr.buildNack();
        expect_eq(nack3.has_nack, false);
        expect_eq(tr.lostPackets(), 0 as u64);
        expect_eq(tr.lossRatePermille(), 0 as u32);
    });

    test("7. Deterministic PRNG Fuzz Robustness Suite (2,000 cases: RTP/RTCP/STUN/SDP)", () => {
        let rng: u64 = 0x5720_4D52_5443_2026;
        let buf: [256]u8 = [0; 256];
        let total_cases: usize = 0;
        let valid_count: usize = 0;

        // 500 RTP fuzz cases (mix of raw random + V=2-biased mutations)
        let i: usize = 0;
        let fuzz_len: usize = 0;
        while (i < 500) : (i += 1) {
            fuzz_len = (nextPrng(*rng) % 128) as usize;
            fillPrngBytes(*rng, buf[0..], fuzz_len);
            if (fuzz_len > 0 && (i & 1) == 0) {
                buf[0] = (buf[0] & 0x3F) | 0x80;
            }
            let rtp_ok = true;
            const rtp_hdr = parseRtpHeader(buf[0..fuzz_len]) catch {
                rtp_ok = false;
                RtpHeader{}
            };
            if (rtp_ok) {
                expect_eq(rtp_hdr.version, 2 as u8);
                expect_eq(rtp_hdr.header_len + rtp_hdr.payload_len + usize(rtp_hdr.padding_len), fuzz_len);
                valid_count += 1;
            }
            total_cases += 1;
        }

        // 500 RTCP fuzz cases
        i = 0;
        while (i < 500) : (i += 1) {
            fuzz_len = (nextPrng(*rng) % 128) as usize;
            fillPrngBytes(*rng, buf[0..], fuzz_len);
            if (fuzz_len >= 2 && (i & 1) == 0) {
                buf[0] = (buf[0] & 0x3F) | 0x80;
                buf[1] = (200 + (i % 7)) as u8;
            }
            let rtcp_ok = true;
            const rtcp_hdr = parseRtcpHeader(buf[0..fuzz_len]) catch {
                rtcp_ok = false;
                RtcpHeader{}
            };
            if (rtcp_ok) {
                expect_eq(rtcp_hdr.version, 2 as u8);
                expect_eq(rtcp_hdr.byte_len <= fuzz_len, true);
                valid_count += 1;
            }
            total_cases += 1;
        }

        // 500 STUN fuzz cases (mix of raw random + magic-cookie-biased mutations)
        i = 0;
        while (i < 500) : (i += 1) {
            fuzz_len = (nextPrng(*rng) % 128) as usize;
            fillPrngBytes(*rng, buf[0..], fuzz_len);
            if (fuzz_len >= 20 && (i & 1) == 0) {
                buf[0] = buf[0] & 0x3F;
                buf[4] = 0x21;
                buf[5] = 0x12;
                buf[6] = 0xA4;
                buf[7] = 0x42;
            }
            let stun_ok = true;
            const stun_msg = parseStunHeader(buf[0..fuzz_len]) catch {
                stun_ok = false;
                StunMessage{}
            };
            if (stun_ok) {
                expect_eq(stun_msg.magic_cookie, STUN_MAGIC_COOKIE);
                expect_eq(stun_msg.total_len <= fuzz_len, true);
                valid_count += 1;
            }
            total_cases += 1;
        }

        // 500 SDP fuzz cases
        i = 0;
        while (i < 500) : (i += 1) {
            fuzz_len = (nextPrng(*rng) % 192) as usize;
            fillPrngBytes(*rng, buf[0..], fuzz_len);
            if (fuzz_len >= 4 && (i & 1) == 0) {
                buf[0] = 0x76; // 'v'
                buf[1] = 0x3D; // '='
                buf[2] = 0x30; // '0'
                buf[3] = 0x0A; // '\n'
            }
            let sdp_ok = true;
            const sdp_sess = parseSdp(buf[0..fuzz_len]) catch {
                sdp_ok = false;
                SdpSession{}
            };
            if (sdp_ok) {
                expect_eq(sdp_sess.version, 0 as u8);
                expect_eq(sdp_sess.media_count <= 4, true);
                valid_count += 1;
            }
            total_cases += 1;
        }

        expect_eq(total_cases, 2000 as usize);
        expect_eq(valid_count > 0, true);
    });
}
