// Origin: Pure-CP Stdlib Test Suite (lib/net/webrtcIce.cp) | Created: 2026-11-23
// Reference-hardened against algesten/str0m (MIT/Apache-2.0): crates/is/src/stun.rs, candidate.rs & src/sdp/parser.rs
// 25+ ported real-world Chrome/Firefox/Safari/FFmpeg STUN, ICE & SDP boundary tests:
//   1. STUN IPv4 & IPv6 XOR-MAPPED-ADDRESS roundtrip + 4 decode_xor malformed guards
//   2. STUN HMAC-SHA1 MESSAGE-INTEGRITY + FINGERPRINT length-recomputation, tamper & wrong-key guards
//   3. STUN ERROR-CODE (487 Role Conflict / 401) reason parsing & RFC 8445 §7.3.1.1 tie-breaker resolution
//   4. ICE Candidates: srflx/relay raddr/rport, tcp active/passive/so, ssltcp, IPv6, mDNS `.local`, generation & ufrag
//   5. Real Chrome, Firefox, FFmpeg WHIP & Safari DataChannel SDP offers (BUNDLE, mid, setup, extmap, rtcp-fb, ssrc, fmtp)
//   6. str0m SDP edge guards: missing a=setup (Firefox), empty a=group:BUNDLE, fmtp horizontal whitespace, PT > 127 rejection

import { test, expect_eq, expect_true } from "cap:test";
import {
    STUN_MAGIC_COOKIE,
    WebRtcIceError,
    StunMsgType,
    StunIpv4Addr,
    StunIpv6Addr,
    encodeStunBindingRequestAuth,
    encodeStunBindingResponse,
    encodeStunBindingResponseV6,
    encodeStunBindingError,
    parseStunPacket,
    verifyStunMessageIntegrity,
    IceRole,
    IceCandidateKind,
    IceTransportProto,
    IceTcpType,
    IceHostKind,
    IceRoleResolution,
    resolveIceRoleConflict,
    parseIceCandidateAttr,
    SdpDirection,
    SdpSetupRole,
    SdpExtmapEntry,
    SdpRtcpFbEntry,
    SdpSsrcEntry,
    SdpCodecSpec,
    parseWebRtcSdpExtended,
    parseWebRtcSdp,
} from "cap:net";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// 1. STUN IPv4 & IPv6 XOR-MAPPED-ADDRESS + 4 str0m decode_xor Error Guards
// ============================================================================

test("str0m STUN: IPv4 & IPv6 XOR-MAPPED-ADDRESS roundtrip + malformed family/length guards") {
    const tx_id: [12]u8 = [0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10, 0x32, 0x54, 0x76];
    let wire: [128]u8 = [0; 128];

    // 1a. IPv4 XOR-MAPPED-ADDRESS
    const v4 = StunIpv4Addr.init(198, 51, 100, 42, 54321);
    const n4 = encodeStunBindingResponse(tx_id, v4, wire[0..]) catch unreachable;
    const p4 = parseStunPacket(wire[0..n4]) catch unreachable;
    expect_eq(p4.msg_type, StunMsgType.binding_response);
    expect_true(p4.has_xor_mapped);
    expect_true(!p4.has_xor_mapped_v6);
    expect_true(p4.xor_mapped.eql(v4));
    expect_true(p4.fingerprint_valid);

    // 1b. IPv6 XOR-MAPPED-ADDRESS (2001:db8:85a3::8a2e:370:7334 port 49152)
    const ip6: [16]u8 = [
        0x20, 0x01, 0x0D, 0xB8, 0x85, 0xA3, 0x00, 0x00,
        0x00, 0x00, 0x8A, 0x2E, 0x03, 0x70, 0x73, 0x34,
    ];
    const v6 = StunIpv6Addr.init(ip6, 49152);
    const n6 = encodeStunBindingResponseV6(tx_id, v6, wire[0..]) catch unreachable;
    const p6 = parseStunPacket(wire[0..n6]) catch unreachable;
    expect_true(p6.has_xor_mapped_v6);
    expect_true(!p6.has_xor_mapped);
    expect_true(p6.xor_mapped_v6.eql(v6));
    expect_true(p6.fingerprint_valid);

    // 1c. decode_xor_buffer_shorter_than_4_is_error (attr_len=2, padded to 4)
    const bad_short: [28]u8 = [
        0x01, 0x01, 0x00, 0x08, 0x21, 0x12, 0xA4, 0x42,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
        0x00, 0x20, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00,
    ];
    let ok = false;
    * = parseStunPacket(bad_short[0..]) catch |e| { if (e == WebRtcIceError.InvalidAttribute) ok = true; };
    expect_true(ok);

    // 1d. decode_xor_ipv4_family_with_short_ip_buf_is_error (family=0x01, attr_len=6)
    const bad_v4: [32]u8 = [
        0x01, 0x01, 0x00, 0x0C, 0x21, 0x12, 0xA4, 0x42,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
        0x00, 0x20, 0x00, 0x06, 0x00, 0x01, 0x12, 0x34, 0xAA, 0xBB, 0x00, 0x00,
    ];
    ok = false;
    * = parseStunPacket(bad_v4[0..]) catch |e| { if (e == WebRtcIceError.InvalidAttribute) ok = true; };
    expect_true(ok);

    // 1e. decode_xor_ipv6_family_with_short_ip_buf_is_error (family=0x02, attr_len=8)
    const bad_v6: [32]u8 = [
        0x01, 0x01, 0x00, 0x0C, 0x21, 0x12, 0xA4, 0x42,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
        0x00, 0x20, 0x00, 0x08, 0x00, 0x02, 0x12, 0x34, 0xAA, 0xBB, 0xCC, 0xDD,
    ];
    ok = false;
    * = parseStunPacket(bad_v6[0..]) catch |e| { if (e == WebRtcIceError.InvalidAttribute) ok = true; };
    expect_true(ok);

    // 1f. decode_xor_unknown_family_is_error (family=0x03)
    const bad_fam: [32]u8 = [
        0x01, 0x01, 0x00, 0x0C, 0x21, 0x12, 0xA4, 0x42,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
        0x00, 0x20, 0x00, 0x08, 0x00, 0x03, 0x12, 0x34, 0xAA, 0xBB, 0xCC, 0xDD,
    ];
    ok = false;
    * = parseStunPacket(bad_fam[0..]) catch |e| { if (e == WebRtcIceError.InvalidAttribute) ok = true; };
    expect_true(ok);
}

// ============================================================================
// 2. STUN MESSAGE-INTEGRITY (HMAC-SHA1), FINGERPRINT Tamper & 487 Role Conflict
// ============================================================================

test("str0m STUN: MESSAGE-INTEGRITY HMAC-SHA1, FINGERPRINT tamper, ERROR-CODE & 487 Role Conflict") {
    const tx_id: [12]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88];
    const key = "0zV/Yu3y8aDzbHgqWhnVQhqP";
    let wire: [160]u8 = [0; 160];

    // 2a. Authenticated Binding Request (ICE-CONTROLLING + USE-CANDIDATE + MESSAGE-INTEGRITY + FINGERPRINT)
    const n1 = encodeStunBindingRequestAuth(
        tx_id,
        "gfNK:062g",
        1853824767,
        IceRole.controlling,
        0x1122334455667788,
        true,
        key,
        wire[0..],
    ) catch unreachable;

    const pkt1 = parseStunPacket(wire[0..n1]) catch unreachable;
    expect_true(bytesEq(pkt1.usernameSlice(wire[0..n1]), "gfNK:062g"));
    expect_eq(pkt1.priority, 1853824767);
    expect_true(pkt1.use_candidate);
    expect_true(pkt1.has_ice_controlling);
    expect_eq(pkt1.ice_controlling, 0x1122334455667788);
    expect_true(pkt1.has_message_integrity);
    expect_true(pkt1.fingerprint_valid);
    expect_true(verifyStunMessageIntegrity(wire[0..n1], pkt1, key));
    expect_true(!verifyStunMessageIntegrity(wire[0..n1], pkt1, "wrong-ice-password"));

    // 2b. Single-bit FINGERPRINT tamper -> InvalidFingerprint
    let tampered = wire;
    tampered[n1 - 1] ^= 0x01;
    let got_fp_err = false;
    * = parseStunPacket(tampered[0..n1]) catch |e| { if (e == WebRtcIceError.InvalidFingerprint) got_fp_err = true; };
    expect_true(got_fp_err);

    // 2c. 487 Role Conflict tie-breaker resolution (all 4 branches)
    expect_eq(resolveIceRoleConflict(IceRole.controlling, 0x1122334455667788, pkt1), IceRoleResolution.send_487_conflict);
    expect_eq(resolveIceRoleConflict(IceRole.controlling, 0x2000000000000000, pkt1), IceRoleResolution.send_487_conflict);
    expect_eq(resolveIceRoleConflict(IceRole.controlling, 0x0000000000000001, pkt1), IceRoleResolution.switch_to_controlled);
    expect_eq(resolveIceRoleConflict(IceRole.controlled, 0x0000000000000001, pkt1), IceRoleResolution.no_conflict);

    const n2 = encodeStunBindingRequestAuth(
        tx_id,
        "a:b",
        100,
        IceRole.controlled,
        500,
        false,
        key,
        wire[0..],
    ) catch unreachable;
    const pkt2 = parseStunPacket(wire[0..n2]) catch unreachable;
    expect_true(pkt2.has_ice_controlled);
    expect_eq(resolveIceRoleConflict(IceRole.controlled, 500, pkt2), IceRoleResolution.switch_to_controlling);
    expect_eq(resolveIceRoleConflict(IceRole.controlled, 499, pkt2), IceRoleResolution.send_487_conflict);

    // 2d. ERROR-CODE 487 Role Conflict reason phrase parsing
    const ne = encodeStunBindingError(tx_id, 487, "Role Conflict", wire[0..]) catch unreachable;
    const pe = parseStunPacket(wire[0..ne]) catch unreachable;
    expect_eq(pe.msg_type, StunMsgType.binding_error);
    expect_true(pe.has_error_code);
    expect_eq(pe.error_code, 487);
    expect_true(bytesEq(pe.errorReasonSlice(wire[0..ne]), "Role Conflict"));
}

// ============================================================================
// 3. str0m ICE Candidates: srflx/relay, tcptype, ssltcp, IPv6, mDNS `.local`, ufrag & generation
// ============================================================================

test("str0m ICE candidates: srflx, relay, tcptype active/passive/so, ssltcp, IPv6 & mDNS .local") {
    // srflx with raddr/rport/ufrag
    const s1 = "a=candidate:1 1 udp 1845494015 198.51.100.100 11100 typ srflx raddr 203.0.113.100 rport 10100 ufrag abc\r\n";
    const c1 = parseIceCandidateAttr(s1) catch unreachable;
    expect_eq(c1.kind, IceCandidateKind.srflx);
    expect_eq(c1.transport, IceTransportProto.udp);
    expect_eq(c1.port, 11100);
    expect_true(c1.has_raddr);
    expect_eq(c1.raddr_ip[0], 203);
    expect_eq(c1.raddr_port, 10100);
    expect_true(bytesEq(c1.ufragSlice(s1), "abc"));

    // tcp active + generation + Chrome network-id/network-cost
    const s2 = "a=candidate:2501718406 1 tcp 1518280447 10.217.229.219 9 typ host tcptype active generation 0 network-id 1 network-cost 900";
    const c2 = parseIceCandidateAttr(s2) catch unreachable;
    expect_eq(c2.transport, IceTransportProto.tcp);
    expect_eq(c2.tcptype, IceTcpType.active);
    expect_true(c2.has_generation);
    expect_eq(c2.generation, 0);

    // tcp passive
    const s3 = "a=candidate:2113932030 1 tcp 2113932030 142.250.82.253 19305 typ host tcptype passive generation 2";
    const c3 = parseIceCandidateAttr(s3) catch unreachable;
    expect_eq(c3.tcptype, IceTcpType.passive);
    expect_eq(c3.generation, 2);

    // ssltcp (Google legacy TCP)
    const s4 = "a=candidate:2113932029 1 ssltcp 2113932029 142.250.82.252 443 typ host generation 0";
    const c4 = parseIceCandidateAttr(s4) catch unreachable;
    expect_eq(c4.transport, IceTransportProto.ssltcp);
    expect_eq(c4.tcptype, IceTcpType.none);

    // IPv6 host candidate (str0m parse_candidates)
    const s5 = "a=candidate:3936339338 1 udp 2122265343 fd00:f8aa:3ff5:5914:90bf:3c5:3378:9d4b 59125 typ host generation 0 network-id 4";
    const c5 = parseIceCandidateAttr(s5) catch unreachable;
    expect_eq(c5.host_kind, IceHostKind.ipv6);
    expect_eq(c5.ip6[0], 0xFD);
    expect_eq(c5.ip6[1], 0x00);
    expect_eq(c5.ip6[14], 0x9D);
    expect_eq(c5.ip6[15], 0x4B);
    expect_eq(c5.port, 59125);

    // Compressed IPv6 `2001:db8::1`
    const s6 = "a=candidate:9 1 udp 2122260223 2001:db8::1 50000 typ relay raddr 192.0.2.1 rport 40000";
    const c6 = parseIceCandidateAttr(s6) catch unreachable;
    expect_eq(c6.host_kind, IceHostKind.ipv6);
    expect_eq(c6.kind, IceCandidateKind.relay);
    expect_eq(c6.ip6[0], 0x20);
    expect_eq(c6.ip6[1], 0x01);
    expect_eq(c6.ip6[2], 0x0D);
    expect_eq(c6.ip6[3], 0xB8);
    expect_eq(c6.ip6[15], 0x01);

    // mDNS `.local` obfuscated host candidate (RFC 8828)
    const s7 = "a=candidate:1467250027 1 udp 2122260223 e82a0a62-9e6a-45e4-8b50-8c34c8b9b7e1.local 51234 typ host generation 0";
    const c7 = parseIceCandidateAttr(s7) catch unreachable;
    expect_true(c7.isMdns());
    expect_eq(c7.port, 51234);
    expect_true(bytesEq(c7.hostSlice(s7), "e82a0a62-9e6a-45e4-8b50-8c34c8b9b7e1.local"));

    // Invalid candidate host -> InvalidCandidate
    let bad = false;
    * = parseIceCandidateAttr("a=candidate:1 1 udp 100 not-an-ip.example 5000 typ host") catch |e| {
        if (e == WebRtcIceError.InvalidCandidate) bad = true;
    };
    expect_true(bad);
}

// ============================================================================
// 4. Real Chrome, Firefox, FFmpeg WHIP & Safari DataChannel SDP Parsing
// ============================================================================

test("str0m SDP: real Chrome & Firefox offers (BUNDLE, mid, setup, extmap, rtcp-fb, ssrc, fmtp)") {
    const chrome_sdp = "v=0\r\n"
        ++ "o=- 5058682828002148772 3 IN IP4 127.0.0.1\r\n"
        ++ "s=-\r\n"
        ++ "t=0 0\r\n"
        ++ "a=group:BUNDLE 0\r\n"
        ++ "a=msid-semantic: WMS 5UUdwiuY7OML2EkQtF38pJtNP5v7In1LhjEK\r\n"
        ++ "m=audio 9 UDP/TLS/RTP/SAVPF 111 103 0 8\r\n"
        ++ "c=IN IP4 0.0.0.0\r\n"
        ++ "a=ice-ufrag:S5hk\r\n"
        ++ "a=ice-pwd:0zV/Yu3y8aDzbHgqWhnVQhqP\r\n"
        ++ "a=fingerprint:sha-256 8C:64:ED:03:76:D0:3D:B4:88:08:91:64:08:80:A8:C6:5A:BF:8B:4E:38:27:96:CA:08:49:25:73:46:60:20:DC\r\n"
        ++ "a=setup:actpass\r\n"
        ++ "a=mid:0\r\n"
        ++ "a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\n"
        ++ "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\n"
        ++ "a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\n"
        ++ "a=sendrecv\r\n"
        ++ "a=rtcp-mux\r\n"
        ++ "a=rtpmap:111 opus/48000/2\r\n"
        ++ "a=rtcp-fb:111 transport-cc\r\n"
        ++ "a=fmtp:111 minptime=10;useinbandfec=1\r\n"
        ++ "a=rtpmap:103 ISAC/16000\r\n"
        ++ "a=rtpmap:0 PCMU/8000\r\n"
        ++ "a=rtpmap:8 PCMA/8000\r\n"
        ++ "a=ssrc:3948621874 cname:xeXs3aE9AOBn00yJ\r\n";

    let codecs: [16]SdpCodecSpec = [SdpCodecSpec{}; 16];
    let extmaps: [8]SdpExtmapEntry = [SdpExtmapEntry{}; 8];
    let fbs: [8]SdpRtcpFbEntry = [SdpRtcpFbEntry{}; 8];
    let ssrcs: [8]SdpSsrcEntry = [SdpSsrcEntry{}; 8];

    const sc = parseWebRtcSdpExtended(chrome_sdp, codecs[0..], extmaps[0..], fbs[0..], ssrcs[0..]) catch unreachable;
    expect_true(sc.has_bundle);
    expect_true(bytesEq(sc.bundleMids(chrome_sdp), "0"));
    expect_true(bytesEq(sc.audioMid(chrome_sdp), "0"));
    expect_eq(sc.setup_role, SdpSetupRole.actpass);
    expect_eq(sc.audio_dir, SdpDirection.sendrecv);
    expect_eq(sc.codec_count, 4);
    expect_eq(sc.extmap_count, 3);
    expect_eq(extmaps[0].id, 1);
    expect_true(bytesEq(extmaps[0].uriSlice(chrome_sdp), "urn:ietf:params:rtp-hdrext:ssrc-audio-level"));
    expect_eq(sc.rtcp_fb_count, 1);
    expect_eq(fbs[0].payload_type, 111);
    expect_true(bytesEq(fbs[0].paramSlice(chrome_sdp), "transport-cc"));
    expect_eq(sc.ssrc_count, 1);
    expect_eq(ssrcs[0].ssrc, 3948621874);
    expect_true(bytesEq(ssrcs[0].attrSlice(chrome_sdp), "cname:xeXs3aE9AOBn00yJ"));
    expect_true(bytesEq(codecs[0].fmtpSlice(chrome_sdp), "minptime=10;useinbandfec=1"));

    // FFmpeg WHIP dual-media SDP (audio mid=0 + video mid=1, BUNDLE 0 1, setup:passive)
    const whip_sdp = "v=0\n"
        ++ "o=FFmpeg 4489045141692799359 2 IN IP4 127.0.0.1\n"
        ++ "s=FFmpegPublishSession\n"
        ++ "t=0 0\n"
        ++ "a=group:BUNDLE 0 1\n"
        ++ "m=audio 9 UDP/TLS/RTP/SAVPF 111\n"
        ++ "a=ice-ufrag:08a9c3f4\n"
        ++ "a=ice-pwd:a202d23be47d1decaa6f67d04d4e6c16\n"
        ++ "a=setup:passive\n"
        ++ "a=mid:0\n"
        ++ "a=sendonly\n"
        ++ "a=rtpmap:111 opus/48000/2\n"
        ++ "m=video 9 UDP/TLS/RTP/SAVPF 106 105\n"
        ++ "a=mid:1\n"
        ++ "a=sendonly\n"
        ++ "a=rtpmap:106 H264/90000\n"
        ++ "a=fmtp:106 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001e\n"
        ++ "a=rtcp-fb:106 nack\n"
        ++ "a=rtpmap:105 rtx/90000\n"
        ++ "a=fmtp:105 apt=106\n";

    const sw = parseWebRtcSdpExtended(whip_sdp, codecs[0..], extmaps[0..], fbs[0..], ssrcs[0..]) catch unreachable;
    expect_true(bytesEq(sw.bundleMids(whip_sdp), "0 1"));
    expect_true(bytesEq(sw.audioMid(whip_sdp), "0"));
    expect_true(bytesEq(sw.videoMid(whip_sdp), "1"));
    expect_eq(sw.setup_role, SdpSetupRole.passive);
    expect_eq(sw.audio_dir, SdpDirection.sendonly);
    expect_eq(sw.video_dir, SdpDirection.sendonly);
    expect_eq(sw.codec_count, 3);
    expect_eq(sw.rtcp_fb_count, 1);
    expect_true(bytesEq(fbs[0].paramSlice(whip_sdp), "nack"));
}

// ============================================================================
// 5. str0m SDP Edge Cases: Safari DataChannel, Missing a=setup, Empty BUNDLE, fmtp WS & PT > 127
// ============================================================================

test("str0m SDP edge guards: Safari DataChannel, Firefox missing setup, empty BUNDLE, fmtp WS & PT>127") {
    let codecs: [8]SdpCodecSpec = [SdpCodecSpec{}; 8];
    let extmaps: [4]SdpExtmapEntry = [SdpExtmapEntry{}; 4];
    let fbs: [4]SdpRtcpFbEntry = [SdpRtcpFbEntry{}; 4];
    let ssrcs: [4]SdpSsrcEntry = [SdpSsrcEntry{}; 4];

    // 5a. Safari DataChannel (application media)
    const safari_sdp = "v=0\r\no=- 4611516372927609806 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"
        ++ "a=group:BUNDLE 0\r\n"
        ++ "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n"
        ++ "a=setup:actpass\r\na=mid:0\r\n";
    const ss = parseWebRtcSdp(safari_sdp, codecs[0..]) catch unreachable;
    expect_true(ss.has_application);
    expect_true(bytesEq(ss.applicationMid(safari_sdp), "0"));

    // 5b. Firefox missing a=setup on inactive mid + extmap with direction `/recvonly`
    const ff_sdp = "v=0\r\no=mozilla 1 2 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\n"
        ++ "a=group:BUNDLE\r\n"
        ++ "m=audio 0 UDP/TLS/RTP/SAVPF 0\r\n"
        ++ "a=inactive\r\na=mid:1\r\n"
        ++ "a=extmap:2/recvonly urn:ietf:params:rtp-hdrext:csrc-audio-level\r\n"
        ++ "a=rtpmap:127 opus/48000/2\r\n"
        ++ "a=fmtp:127 \tminptime=10;useinbandfec=1\t \r\n";
    const sf = parseWebRtcSdpExtended(ff_sdp, codecs[0..], extmaps[0..], fbs[0..], ssrcs[0..]) catch unreachable;
    expect_eq(sf.setup_role, SdpSetupRole.none);
    expect_eq(sf.audio_dir, SdpDirection.inactive);
    expect_true(sf.has_bundle);
    expect_eq(sf.bundleMids(ff_sdp).len, 0);
    expect_eq(sf.extmap_count, 1);
    expect_eq(extmaps[0].id, 2);
    expect_eq(codecs[0].payload_type, 127);
    expect_true(bytesEq(codecs[0].fmtpSlice(ff_sdp), "minptime=10;useinbandfec=1"));

    // 5c. rtpmap_payload_type_over_127_is_rejected (PT=200 must fail InvalidSdp)
    const bad_pt_sdp = "v=0\r\nm=audio 9 UDP/TLS/RTP/SAVPF 200\r\na=rtpmap:200 opus/48000/2\r\n";
    let got_bad_pt = false;
    * = parseWebRtcSdp(bad_pt_sdp, codecs[0..]) catch |e| {
        if (e == WebRtcIceError.InvalidSdp) got_bad_pt = true;
    };
    expect_true(got_bad_pt);
}
