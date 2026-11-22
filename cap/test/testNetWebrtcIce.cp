// Origin: Pure-CP Stdlib Test Suite (lib/net/webrtcIce.cp) | Created: 2026-11-23
// Production Deep Verification Suite for RFC 8489 STUN, RFC 8445 ICE & RFC 8866/9725 WebRTC SDP/WHEP:
//   1. STUN Binding Request/Response/Error roundtrip, XOR-MAPPED-ADDRESS, 4-byte padding & single-byte FINGERPRINT tamper rejection
//   2. RFC 8445 §5.1.2.1 candidate priority & §5.7.2 pair priority exact formula verification
//   3. `parseIceCandidateAttr` (host/srflx/relay + raddr/rport), `buildAndSortIcePairs` & `IceCheckList` nomination
//   4. High-cardinality external-slice ICE pairing (64 pairs) verifying O(N log N) heapsort & component filtering
//   5. WebRTC SDP parsing (Opus/PCMU/H264/VP8/AV1, fmtp, DTLS SHA-256 fingerprint, ICE ufrag/pwd, BUNDLE, rtcp-mux)
//   6. Offer/Answer `negotiateCommonCodecs` case-insensitive Offer-priority intersection
//   7. RFC 9725 WHEP HTTP/1.1 POST formatting (with/without Bearer token, Content-Length, CRLF injection guard)
//   8. Malformed wire/SDP/candidate boundary guards & `cap.net` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    STUN_MAGIC_COOKIE,
    STUN_HEADER_LEN,
    STUN_FINGERPRINT_XOR,
    ICE_PRIORITY_HOST,
    ICE_PRIORITY_SRFLX,
    ICE_PRIORITY_RELAY,
    WebRtcIceError,
    computeStunCrc32,
    StunMsgType,
    StunIpv4Addr,
    encodeStunBindingRequest,
    encodeStunBindingResponse,
    encodeStunBindingError,
    parseStunPacket,
    IceCandidateKind,
    IceRole,
    IcePairState,
    WebRtcIceCandidate,
    computeIceCandidatePriority,
    computeIcePairPriority,
    parseIceCandidateAttr,
    IceCandidatePair,
    buildAndSortIcePairs,
    selectBestNominatedPair,
    IceCheckList,
    SdpMediaKind,
    SdpDirection,
    SdpCodecSpec,
    parseWebRtcSdp,
    negotiateCommonCodecs,
    formatWhepOfferRequest,
} from "cap:net";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (sliceEq(haystack[i..(i + needle.len)], needle)) return true;
    }
    return false;
}

test("STUN RFC 8489: Binding Request/Response/Error roundtrip, XOR-MAPPED-ADDRESS & FINGERPRINT tamper guard") {
    // IEEE 802.3 CRC32 standard check value: CRC32("123456789") == 0xCBF43926
    expect_eq(computeStunCrc32("123456789"), @as(u32, 0xCBF43926));
    expect_eq(STUN_MAGIC_COOKIE, @as(u32, 0x2112A442));
    expect_eq(STUN_FINGERPRINT_XOR, @as(u32, 0x5354554E));

    const tx_id: [12]u8 = [0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87, 0x98, 0xA9, 0xBA, 0xCB];
    let buf: [256]u8 = [0; 256];

    // Non-4-byte-aligned username ("rem:loc12", 9 bytes -> padded to 12 bytes)
    const req_len = encodeStunBindingRequest(
        tx_id,
        "rem:loc12",
        1853824767,
        0xCAFEBABE12345678,
        true,
        buf[0..],
    ) catch {
        expect_true(false);
        return;
    };
    expect_eq(req_len % 4, @as(usize, 0));

    const req_pkt = parseStunPacket(buf[0..req_len]) catch {
        expect_true(false);
        return;
    };
    expect_true(req_pkt.msg_type == StunMsgType.binding_request);
    expect_true(req_pkt.matchesTxId(tx_id));
    expect_true(sliceEq(req_pkt.usernameSlice(buf[0..req_len]), "rem:loc12"));
    expect_true(req_pkt.has_priority);
    expect_eq(req_pkt.priority, @as(u32, 1853824767));
    expect_true(req_pkt.use_candidate);
    expect_true(req_pkt.has_ice_controlling);
    expect_eq(req_pkt.ice_controlling, @as(u64, 0xCAFEBABE12345678));
    expect_true(req_pkt.has_fingerprint);
    expect_true(req_pkt.fingerprint_valid);

    // Single-byte tamper in payload MUST be rejected with InvalidFingerprint
    buf[25] ^= 0x01;
    let fp_rejected: bool = false;
    * = parseStunPacket(buf[0..req_len]) catch |e| {
        if (e == WebRtcIceError.InvalidFingerprint) {
            fp_rejected = true;
        }
    };
    expect_true(fp_rejected);

    // Binding Response with XOR-MAPPED-ADDRESS (203.0.113.42:54321)
    const mapped = StunIpv4Addr.init(203, 0, 113, 42, 54321);
    const resp_len = encodeStunBindingResponse(tx_id, mapped, buf[0..]) catch {
        expect_true(false);
        return;
    };
    // 20 header + 12 XOR-MAPPED-ADDRESS + 8 FINGERPRINT = 40 bytes
    expect_eq(resp_len, @as(usize, 40));

    // Verify wire XOR encoding directly: port ^ 0x2112, ip ^ 0x2112A442
    const wire_xport: u16 = ((buf[26] as u16) << 8) | (buf[27] as u16);
    expect_eq(wire_xport, @as(u16, 54321 ^ 0x2112));
    expect_eq(buf[28], @as(u8, 203 ^ 0x21));
    expect_eq(buf[29], @as(u8, 0 ^ 0x12));
    expect_eq(buf[30], @as(u8, 113 ^ 0xA4));
    expect_eq(buf[31], @as(u8, 42 ^ 0x42));

    const resp_pkt = parseStunPacket(buf[0..resp_len]) catch {
        expect_true(false);
        return;
    };
    expect_true(resp_pkt.msg_type == StunMsgType.binding_response);
    expect_true(resp_pkt.has_xor_mapped);
    expect_true(resp_pkt.xor_mapped.eql(mapped));
    expect_true(resp_pkt.has_fingerprint && resp_pkt.fingerprint_valid);

    // Binding Error Response (487 Role Conflict)
    const err_len = encodeStunBindingError(tx_id, 487, "Role Conflict", buf[0..]) catch {
        expect_true(false);
        return;
    };
    const err_pkt = parseStunPacket(buf[0..err_len]) catch {
        expect_true(false);
        return;
    };
    expect_true(err_pkt.msg_type == StunMsgType.binding_error);
    expect_true(err_pkt.has_error_code);
    expect_eq(err_pkt.error_code, @as(u16, 487));
    expect_true(err_pkt.fingerprint_valid);
}

test("ICE RFC 8445: candidate priority & pair priority exact formula verification") {
    // RFC 8445 §5.1.2.1: (2^24)*type_pref + (2^8)*local_pref + (256 - component_id)
    // Host (126), local_pref=65535, component=1 => (126 << 24) + (65535 << 8) + 255 = 2130706431
    const host_p = computeIceCandidatePriority(IceCandidateKind.host, 65535, 1);
    expect_eq(host_p, @as(u32, 2130706431));

    // Srflx (100), local_pref=65535, component=1 => (100 << 24) + (65535 << 8) + 255 = 1694498815
    const srflx_p = computeIceCandidatePriority(IceCandidateKind.srflx, 65535, 1);
    expect_eq(srflx_p, @as(u32, 1694498815));

    // Relay (0), local_pref=65535, component=2 => (0 << 24) + (65535 << 8) + 254 = 16777214
    const relay_p = computeIceCandidatePriority(IceCandidateKind.relay, 65535, 2);
    expect_eq(relay_p, @as(u32, 16777214));

    expect_true(host_p > srflx_p);
    expect_true(srflx_p > relay_p);
    expect_eq(ICE_PRIORITY_HOST, @as(u32, 126));
    expect_eq(ICE_PRIORITY_SRFLX, @as(u32, 100));
    expect_eq(ICE_PRIORITY_RELAY, @as(u32, 0));

    // RFC 8445 §5.7.2: 2^32 * MIN(G,D) + 2 * MAX(G,D) + (G > D ? 1 : 0)
    const p_gd = computeIcePairPriority(2000, 1000);
    const p_dg = computeIcePairPriority(1000, 2000);
    const p_eq = computeIcePairPriority(1500, 1500);
    expect_eq(p_gd, (@as(u64, 1000) << 32) + @as(u64, 4001));
    expect_eq(p_dg, (@as(u64, 1000) << 32) + @as(u64, 4000));
    expect_eq(p_gd - p_dg, @as(u64, 1));
    expect_eq(p_eq, (@as(u64, 1500) << 32) + @as(u64, 3000));
}

test("ICE RFC 8445: candidate parsing, pair sorting & IceCheckList connectivity nomination") {
    const l0 = parseIceCandidateAttr("candidate:842163049 1 udp 2122260223 192.168.1.10 54321 typ host") catch {
        expect_true(false);
        return;
    };
    const l1 = parseIceCandidateAttr("a=candidate:2999745851 1 udp 1686052607 203.0.113.10 54321 typ srflx raddr 192.168.1.10 rport 54321\r\n") catch {
        expect_true(false);
        return;
    };
    const l2 = parseIceCandidateAttr("candidate:1001 1 udp 16777215 198.51.100.5 3478 typ relay raddr 203.0.113.10 rport 54321") catch {
        expect_true(false);
        return;
    };

    expect_eq(l0.component_id, @as(u16, 1));
    expect_true(l0.kind == IceCandidateKind.host);
    expect_eq(l0.priority, @as(u32, 2122260223));
    expect_eq(l0.ip[0], @as(u8, 192));
    expect_eq(l0.ip[3], @as(u8, 10));
    expect_eq(l0.port, @as(u16, 54321));
    expect_true(l1.kind == IceCandidateKind.srflx);
    expect_true(l1.has_raddr);
    expect_eq(l1.raddr_ip[0], @as(u8, 192));
    expect_eq(l1.raddr_port, @as(u16, 54321));
    expect_true(l2.kind == IceCandidateKind.relay);

    const r0 = parseIceCandidateAttr("candidate:777 1 udp 2113937151 10.0.0.20 50000 typ host") catch {
        expect_true(false);
        return;
    };
    const r1 = parseIceCandidateAttr("candidate:888 1 udp 1677729535 198.51.100.20 50001 typ srflx") catch {
        expect_true(false);
        return;
    };

    const locals: [3]WebRtcIceCandidate = [l0, l1, l2];
    const remotes: [2]WebRtcIceCandidate = [r0, r1];
    let pair_buf: [8]IceCandidatePair = [IceCandidatePair{}; 8];

    let cl = IceCheckList.init(locals[0..], remotes[0..], IceRole.controlling, 9999, pair_buf[0..]) catch {
        expect_true(false);
        return;
    };
    const active = cl.activePairs();
    expect_eq(active.len, @as(usize, 6));

    // Verify strictly non-increasing pair_priority order & top pair is (host, host)
    expect_eq(active[0].local_idx, @as(usize, 0));
    expect_eq(active[0].remote_idx, @as(usize, 0));
    expect_true(active[0].state == IcePairState.waiting);
    let i: usize = 1;
    while (i < active.len) : (i += 1) {
        expect_true(active[i - 1].pair_priority >= active[i].pair_priority);
    }

    // Drive connectivity checks: pair 0 fails, pair 1 succeeds & nominated, pair 2 succeeds & nominated
    const idx0 = cl.nextCheckPair() ?? 999;
    expect_eq(idx0, @as(usize, 0));
    expect_true(cl.markInProgress(idx0, 1_000));
    expect_true(cl.onCheckFailed(idx0));

    const idx1 = cl.nextCheckPair() ?? 999;
    expect_eq(idx1, @as(usize, 1));
    expect_true(cl.markInProgress(idx1, 2_000));
    expect_true(cl.onCheckSucceeded(idx1, 18_500, true));

    const idx2 = cl.nextCheckPair() ?? 999;
    expect_eq(idx2, @as(usize, 2));
    expect_true(cl.markInProgress(idx2, 3_000));
    expect_true(cl.onCheckSucceeded(idx2, 9_000, true));

    expect_eq(cl.succeededCount(), @as(usize, 2));
    // Best nominated pair must be idx1 (higher pair_priority than idx2)
    expect_eq(cl.bestNominatedPair() ?? 999, @as(usize, 1));
}

test("ICE external-slice scaling: 8x8=64 pairs O(N log N) heapsort & component filtering") {
    let locals: [8]WebRtcIceCandidate = [WebRtcIceCandidate{}; 8];
    let remotes: [8]WebRtcIceCandidate = [WebRtcIceCandidate{}; 8];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        // Interleave component 1 and 2, with scrambled priorities
        const comp: u16 = if ((i % 2) == 0) 1 else 2;
        locals[i] = WebRtcIceCandidate{
            foundation_hash: (i + 1) as u32,
            component_id: comp,
            kind: IceCandidateKind.host,
            priority: computeIceCandidatePriority(IceCandidateKind.host, (1000 + ((i * 7919) % 50000)) as u16, comp),
            ip: [10, 0, 0, (i + 1) as u8],
            port: (50000 + i) as u16,
        };
        remotes[i] = WebRtcIceCandidate{
            foundation_hash: (i + 101) as u32,
            component_id: comp,
            kind: IceCandidateKind.srflx,
            priority: computeIceCandidatePriority(IceCandidateKind.srflx, (2000 + ((i * 3571) % 40000)) as u16, comp),
            ip: [203, 0, 113, (i + 1) as u8],
            port: (60000 + i) as u16,
        };
    }

    // 4 locals(comp=1) * 4 remotes(comp=1) + 4 locals(comp=2) * 4 remotes(comp=2) = 32 compatible pairs
    let out_pairs: [64]IceCandidatePair = [IceCandidatePair{}; 64];
    const n = buildAndSortIcePairs(locals[0..], remotes[0..], IceRole.controlled, out_pairs[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(n, @as(usize, 32));

    let k: usize = 1;
    while (k < n) : (k += 1) {
        expect_true(out_pairs[k - 1].pair_priority >= out_pairs[k].pair_priority);
    }

    // Tie-break nomination by RTT when two pairs share identical priority
    out_pairs[5].pair_priority = 999_999_999_999;
    out_pairs[5].state = IcePairState.succeeded;
    out_pairs[5].nominated = true;
    out_pairs[5].rtt_us = 25_000;

    out_pairs[12].pair_priority = 999_999_999_999;
    out_pairs[12].state = IcePairState.succeeded;
    out_pairs[12].nominated = true;
    out_pairs[12].rtt_us = 8_000;

    expect_eq(selectBestNominatedPair(out_pairs[0..n]) ?? 999, @as(usize, 12));
}

test("WebRTC SDP RFC 8866: parse Opus/PCMU/H264/VP8/AV1, fmtp, DTLS SHA-256, ICE creds, BUNDLE & rtcp-mux") {
    const offer_sdp =
        "v=0\r\n" ++
        "o=- 4611731400430051336 2 IN IP4 127.0.0.1\r\n" ++
        "s=-\r\n" ++
        "t=0 0\r\n" ++
        "a=group:BUNDLE 0 1\r\n" ++
        "a=ice-lite\r\n" ++
        "m=audio 9 UDP/TLS/RTP/SAVPF 111 0\r\n" ++
        "a=ice-ufrag:aBcD1234\r\n" ++
        "a=ice-pwd:superSecretIcePassword24Bytes\r\n" ++
        "a=fingerprint:sha-256 AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99\r\n" ++
        "a=setup:actpass\r\n" ++
        "a=recvonly\r\n" ++
        "a=rtcp-mux\r\n" ++
        "a=rtpmap:111 opus/48000/2\r\n" ++
        "a=fmtp:111 minptime=10;useinbandfec=1\r\n" ++
        "a=rtpmap:0 PCMU/8000\r\n" ++
        "m=video 9 UDP/TLS/RTP/SAVPF 96 98 45\r\n" ++
        "a=sendonly\r\n" ++
        "a=rtcp-mux\r\n" ++
        "a=rtpmap:96 H264/90000\r\n" ++
        "a=fmtp:96 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\n" ++
        "a=rtpmap:98 VP8/90000\r\n" ++
        "a=rtpmap:45 AV1/90000\r\n";

    let codecs: [16]SdpCodecSpec = [SdpCodecSpec{}; 16];
    const summary = parseWebRtcSdp(offer_sdp, codecs[0..]) catch {
        expect_true(false);
        return;
    };

    expect_true(summary.has_version);
    expect_true(summary.has_bundle);
    expect_true(summary.has_rtcp_mux);
    expect_true(summary.ice_lite);
    expect_true(summary.has_audio && summary.has_video);
    expect_true(summary.audio_dir == SdpDirection.recvonly);
    expect_true(summary.video_dir == SdpDirection.sendonly);
    expect_true(sliceEq(summary.iceUfrag(offer_sdp), "aBcD1234"));
    expect_true(sliceEq(summary.icePwd(offer_sdp), "superSecretIcePassword24Bytes"));
    expect_true(sliceEq(summary.dtlsFingerprintSha256(offer_sdp), "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99"));
    expect_true(sliceEq(summary.dtlsSetup(offer_sdp), "actpass"));

    expect_eq(summary.codec_count, @as(usize, 5));
    // Codec 0: opus/48000/2
    expect_true(codecs[0].media == SdpMediaKind.audio);
    expect_eq(codecs[0].payload_type, @as(u8, 111));
    expect_eq(codecs[0].clock_rate, @as(u32, 48000));
    expect_eq(codecs[0].channels, @as(u8, 2));
    expect_true(sliceEq(codecs[0].nameSlice(offer_sdp), "opus"));
    expect_true(sliceEq(codecs[0].fmtpSlice(offer_sdp), "minptime=10;useinbandfec=1"));
    // Codec 2: H264/90000
    expect_true(codecs[2].media == SdpMediaKind.video);
    expect_eq(codecs[2].payload_type, @as(u8, 96));
    expect_eq(codecs[2].clock_rate, @as(u32, 90000));
    expect_true(sliceEq(codecs[2].nameSlice(offer_sdp), "H264"));
    expect_true(containsSubstr(codecs[2].fmtpSlice(offer_sdp), "profile-level-id=42e01f"));
    // Codec 4: AV1/90000
    expect_eq(codecs[4].payload_type, @as(u8, 45));
    expect_true(sliceEq(codecs[4].nameSlice(offer_sdp), "AV1"));
}

test("WebRTC SDP Offer/Answer: negotiateCommonCodecs preserves Offer preference order") {
    const offer_sdp =
        "v=0\n" ++
        "m=audio 9 UDP/TLS/RTP/SAVPF 111 8 0\n" ++
        "a=rtpmap:111 opus/48000/2\n" ++
        "a=rtpmap:8 PCMA/8000\n" ++
        "a=rtpmap:0 PCMU/8000\n" ++
        "m=video 9 UDP/TLS/RTP/SAVPF 45 96 98\n" ++
        "a=rtpmap:45 AV1/90000\n" ++
        "a=rtpmap:96 H264/90000\n" ++
        "a=rtpmap:98 VP8/90000\n";

    let codecs: [8]SdpCodecSpec = [SdpCodecSpec{}; 8];
    const summary = parseWebRtcSdp(offer_sdp, codecs[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(summary.codec_count, @as(usize, 6));

    // Local supports VP8, h264 (lowercase!), OPUS (uppercase!) — Offer order must yield [0 (opus), 4 (H264), 5 (VP8)]
    const local_supported: [3][]const u8 = ["vp8", "h264", "OPUS"];
    let matched: [8]usize = [0; 8];
    const count = negotiateCommonCodecs(codecs[0..summary.codec_count], offer_sdp, local_supported[0..], matched[0..]);
    expect_eq(count, @as(usize, 3));
    expect_eq(matched[0], @as(usize, 0));
    expect_eq(matched[1], @as(usize, 4));
    expect_eq(matched[2], @as(usize, 5));

    // Bounded output slice truncates cleanly without overflow
    let tiny_matched: [2]usize = [0; 2];
    const tiny_count = negotiateCommonCodecs(codecs[0..summary.codec_count], offer_sdp, local_supported[0..], tiny_matched[0..]);
    expect_eq(tiny_count, @as(usize, 2));
    expect_eq(tiny_matched[0], @as(usize, 0));
    expect_eq(tiny_matched[1], @as(usize, 4));
}

test("WHEP RFC 9725: formatWhepOfferRequest with/without Bearer token & CRLF injection guard") {
    const sdp_body = "v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n";
    let out: [512]u8 = [0; 512];

    const n1 = formatWhepOfferRequest("/whep/live/cam01", "stream.edge.local:8443", "eyJhbGciOi", sdp_body, out[0..]) catch {
        expect_true(false);
        return;
    };
    const req1 = out[0..n1];
    expect_true(containsSubstr(req1, "POST /whep/live/cam01 HTTP/1.1\r\n"));
    expect_true(containsSubstr(req1, "Host: stream.edge.local:8443\r\n"));
    expect_true(containsSubstr(req1, "Content-Type: application/sdp\r\n"));
    expect_true(containsSubstr(req1, "Accept: application/sdp\r\n"));
    expect_true(containsSubstr(req1, "Authorization: Bearer eyJhbGciOi\r\n"));
    expect_true(containsSubstr(req1, "Content-Length: 43\r\n\r\nv=0\r\n"));

    // Without Bearer token
    const n2 = formatWhepOfferRequest("/whep", "edge.local", "", sdp_body, out[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(!containsSubstr(out[0..n2], "Authorization:"));

    // CRLF header injection guard
    let crlf_blocked: bool = false;
    * = formatWhepOfferRequest("/whep\r\nX-Evil: 1", "edge.local", "", sdp_body, out[0..]) catch |e| {
        if (e == WebRtcIceError.InvalidSdp) crlf_blocked = true;
    };
    expect_true(crlf_blocked);

    // BufferTooSmall guard
    let small: [32]u8 = [0; 32];
    let too_small: bool = false;
    * = formatWhepOfferRequest("/whep", "edge.local", "", sdp_body, small[0..]) catch |e| {
        if (e == WebRtcIceError.BufferTooSmall) too_small = true;
    };
    expect_true(too_small);
}

test("Boundary & malformed input guards + cap.net facade parity") {
    // 1. PacketTooShort (< 20 bytes)
    let short_buf: [12]u8 = [0; 12];
    let got_short: bool = false;
    * = cap.net.parseStunPacket(short_buf[0..]) catch |e| {
        if (e == WebRtcIceError.PacketTooShort) got_short = true;
    };
    expect_true(got_short);

    // 2. InvalidMagicCookie
    let bad_cookie: [20]u8 = [0; 20];
    bad_cookie[1] = 0x01; // binding_request
    bad_cookie[4] = 0xDE;
    bad_cookie[5] = 0xAD;
    let got_cookie: bool = false;
    * = parseStunPacket(bad_cookie[0..]) catch |e| {
        if (e == WebRtcIceError.InvalidMagicCookie) got_cookie = true;
    };
    expect_true(got_cookie);

    // 3. InvalidMessageLength (not a multiple of 4)
    let bad_len: [20]u8 = [0; 20];
    bad_len[1] = 0x01;
    bad_len[3] = 0x03; // msg_len = 3 (invalid)
    bad_len[4] = 0x21;
    bad_len[5] = 0x12;
    bad_len[6] = 0xA4;
    bad_len[7] = 0x42;
    let got_len: bool = false;
    * = parseStunPacket(bad_len[0..]) catch |e| {
        if (e == WebRtcIceError.InvalidMessageLength) got_len = true;
    };
    expect_true(got_len);

    // 4. InvalidCandidate (port 0 / missing typ)
    let got_cand: bool = false;
    * = cap.net.parseIceCandidateAttr("candidate:1 1 udp 100 192.168.1.1 0 typ host") catch |e| {
        if (e == WebRtcIceError.InvalidCandidate) got_cand = true;
    };
    expect_true(got_cand);

    // 5. InvalidSdp (missing v=0)
    let codecs: [4]SdpCodecSpec = [SdpCodecSpec{}; 4];
    let got_sdp: bool = false;
    * = cap.net.parseWebRtcSdp("m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n", codecs[0..]) catch |e| {
        if (e == WebRtcIceError.InvalidSdp) got_sdp = true;
    };
    expect_true(got_sdp);

    // 6. OutputSliceTooSmall in buildAndSortIcePairs
    const c = parseIceCandidateAttr("candidate:1 1 udp 2122260223 192.168.1.1 5000 typ host") catch {
        expect_true(false);
        return;
    };
    const arr: [2]WebRtcIceCandidate = [c, c];
    let tiny_pairs: [2]IceCandidatePair = [IceCandidatePair{}; 2];
    let got_overflow: bool = false;
    * = cap.net.buildAndSortIcePairs(arr[0..], arr[0..], IceRole.controlling, tiny_pairs[0..]) catch |e| {
        if (e == WebRtcIceError.OutputSliceTooSmall) got_overflow = true;
    };
    expect_true(got_overflow);

    // 7. cap.net facade constant parity
    expect_eq(cap.net.STUN_HEADER_LEN, STUN_HEADER_LEN);
    expect_eq(cap.net.STUN_MAGIC_COOKIE, STUN_MAGIC_COOKIE);
}
