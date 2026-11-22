// Source: cp/cap/test/test_sdp_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:sdp & cap.sdp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SdpError,
    SdpAttribute,
    RtpCodec,
    IceCandidate,
    MediaSection,
    SdpSession,
} from "cap:net";

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    const webrtc_offer =
        "v=0\r\n" ++
        "o=- 4611731400430051336 2 IN IP4 127.0.0.1\r\n" ++
        "s=-\r\n" ++
        "t=0 0\r\n" ++
        "a=group:BUNDLE 0 1 2\r\n" ++
        "a=msid-semantic: WMS stream1\r\n" ++
        "m=audio 9 UDP/TLS/RTP/SAVPF 111 103\r\n" ++
        "c=IN IP4 0.0.0.0\r\n" ++
        "a=rtcp:9 IN IP4 0.0.0.0\r\n" ++
        "a=ice-ufrag:ufragA\r\n" ++
        "a=ice-pwd:passwordA1234567890123456\r\n" ++
        "a=sendrecv\r\n" ++
        "a=rtcp-mux\r\n" ++
        "a=rtpmap:111 opus/48000/2\r\n" ++
        "a=rtpmap:103 ISAC/16000\r\n" ++
        "m=video 9 UDP/TLS/RTP/SAVPF 96 98\r\n" ++
        "c=IN IP4 0.0.0.0\r\n" ++
        "a=ice-ufrag:ufragV\r\n" ++
        "a=sendonly\r\n" ++
        "a=rtpmap:96 VP8/90000\r\n" ++
        "a=rtpmap:98 H264/90000\r\n" ++
        "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n" ++
        "c=IN IP4 0.0.0.0\r\n" ++
        "a=sctp-port:5000\r\n" ++
        "a=max-message-size:262144\r\n";

    let sess = SdpSession.parse(alloc, webrtc_offer)?;
    defer sess.deinit(alloc);

    expect_eq(sess.version, 0 as u32);
    expect_eq(std.mem.eql(u8, sess.origin, "- 4611731400430051336 2 IN IP4 127.0.0.1"), true);
    expect_eq(std.mem.eql(u8, sess.session_name, "-"), true);
    expect_eq(std.mem.eql(u8, sess.timing, "0 0"), true);
    expect_eq(sess.media.len, 3 as usize);

    const bundle = sess.getAttr("group") orelse unreachable;
    expect_eq(std.mem.eql(u8, bundle, "BUNDLE 0 1 2"), true);

    const audio = sess.findMedia("audio") orelse unreachable;
    expect_eq(audio.port, 9 as u16);
    expect_eq(std.mem.eql(u8, audio.proto, "UDP/TLS/RTP/SAVPF"), true);
    expect_eq(audio.formats.len, 2 as usize);
    expect_eq(audio.hasFlag("rtcp-mux"), true);
    expect_eq(std.mem.eql(u8, audio.getAttr("ice-ufrag") orelse unreachable, "ufragA"), true);

    const video = sess.findMedia("video") orelse unreachable;
    expect_eq(video.formats.len, 2 as usize);

    const app = sess.findMedia("application") orelse unreachable;
    expect_eq(std.mem.eql(u8, app.proto, "UDP/DTLS/SCTP"), true);
    expect_eq(std.mem.eql(u8, app.getAttr("sctp-port") orelse unreachable, "5000"), true);

    // Round-trip format check
    const formatted = sess.format(alloc)?;
    defer alloc.free(formatted);
    expect_eq(std.mem.eql(u8, formatted, webrtc_offer), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    const sdp_dirs =
        "v=0\r\n" ++
        "o=- 1 1 IN IP4 127.0.0.1\r\n" ++
        "s=dirs\r\n" ++
        "t=0 0\r\n" ++
        "m=audio 5000 RTP/AVP 111 0\r\n" ++
        "a=sendrecv\r\n" ++
        "a=rtpmap:111 opus/48000/2\r\n" ++
        "a=rtpmap:0 PCMU/8000\r\n" ++
        "m=video 5002 RTP/AVP 96\r\n" ++
        "a=sendonly\r\n" ++
        "a=rtpmap:96 VP8/90000\r\n" ++
        "m=video 5004 RTP/AVP 97\r\n" ++
        "a=recvonly\r\n" ++
        "m=audio 5006 RTP/AVP 8\r\n" ++
        "a=inactive\r\n";

    let sess = SdpSession.parse(alloc, sdp_dirs)?;
    defer sess.deinit(alloc);
    expect_eq(sess.media.len, 4 as usize);

    const audio_codecs = sess.media[0].rtpmaps(alloc)?;
    defer alloc.free(audio_codecs);
    expect_eq(audio_codecs.len, 2 as usize);
    expect_eq(audio_codecs[0].payload_type, 111 as u8);
    expect_eq(std.mem.eql(u8, audio_codecs[0].name, "opus"), true);
    expect_eq(audio_codecs[0].clock_rate, 48000 as u32);
    expect_eq(std.mem.eql(u8, audio_codecs[0].encoding_params orelse unreachable, "2"), true);

    expect_eq(audio_codecs[1].payload_type, 0 as u8);
    expect_eq(std.mem.eql(u8, audio_codecs[1].name, "PCMU"), true);
    expect_eq(audio_codecs[1].clock_rate, 8000 as u32);
    expect_eq(audio_codecs[1].encoding_params == null, true);

    const video_codecs = sess.media[1].rtpmaps(alloc)?;
    defer alloc.free(video_codecs);
    expect_eq(video_codecs.len, 1 as usize);
    expect_eq(video_codecs[0].payload_type, 96 as u8);
    expect_eq(std.mem.eql(u8, video_codecs[0].name, "VP8"), true);
    expect_eq(video_codecs[0].clock_rate, 90000 as u32);
    expect_eq(video_codecs[0].encoding_params == null, true);

    expect_eq(std.mem.eql(u8, sess.media[0].direction() orelse unreachable, "sendrecv"), true);
    expect_eq(std.mem.eql(u8, sess.media[1].direction() orelse unreachable, "sendonly"), true);
    expect_eq(std.mem.eql(u8, sess.media[2].direction() orelse unreachable, "recvonly"), true);
    expect_eq(std.mem.eql(u8, sess.media[3].direction() orelse unreachable, "inactive"), true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;
    // LF-only line endings + mixed host/srflx/relay ICE candidates
    const lf_sdp =
        "v=0\n" ++
        "o=- 999 1 IN IP4 10.0.0.1\n" ++
        "s=ice-test\n" ++
        "t=0 0\n" ++
        "m=audio 9 UDP/TLS/RTP/SAVPF 111\n" ++
        "c=IN IP4 10.0.0.1\n" ++
        "a=candidate:842163049 1 udp 2122260223 192.168.1.10 54321 typ host generation 0\n" ++
        "a=candidate:123456789 1 udp 1686052607 203.0.113.5 61234 typ srflx raddr 192.168.1.10 rport 54321\n" ++
        "a=candidate:987654321 2 tcp 41885439 198.51.100.2 3478 typ relay raddr 203.0.113.5 rport 61234\n";

    let sess = SdpSession.parse(alloc, lf_sdp)?;
    defer sess.deinit(alloc);

    const audio = sess.findMedia("audio") orelse unreachable;
    const cands = audio.candidates(alloc)?;
    defer alloc.free(cands);
    expect_eq(cands.len, 3 as usize);

    expect_eq(std.mem.eql(u8, cands[0].foundation, "842163049"), true);
    expect_eq(cands[0].component, 1 as u32);
    expect_eq(std.mem.eql(u8, cands[0].transport, "udp"), true);
    expect_eq(cands[0].priority, 2122260223 as u64);
    expect_eq(std.mem.eql(u8, cands[0].address, "192.168.1.10"), true);
    expect_eq(cands[0].port, 54321 as u16);
    expect_eq(std.mem.eql(u8, cands[0].cand_type, "host"), true);

    expect_eq(std.mem.eql(u8, cands[1].cand_type, "srflx"), true);
    expect_eq(std.mem.eql(u8, cands[1].address, "203.0.113.5"), true);
    expect_eq(cands[1].port, 61234 as u16);

    expect_eq(cands[2].component, 2 as u32);
    expect_eq(std.mem.eql(u8, cands[2].transport, "tcp"), true);
    expect_eq(std.mem.eql(u8, cands[2].cand_type, "relay"), true);
    expect_eq(cands[2].port, 3478 as u16);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. Non-zero v= -> InvalidVersion
    let bad_ver_ok = false;
    * = SdpSession.parse(alloc, "v=1\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\n") catch |e| {
        expect_eq(e == SdpError.InvalidVersion, true);
        bad_ver_ok = true;
    };
    expect_eq(bad_ver_ok, true);

    // 2. Missing v= -> InvalidVersion
    let missing_ver_ok = false;
    * = SdpSession.parse(alloc, "o=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\n") catch |e| {
        expect_eq(e == SdpError.InvalidVersion, true);
        missing_ver_ok = true;
    };
    expect_eq(missing_ver_ok, true);

    // 3. Missing o= -> MissingOrigin
    let missing_origin_ok = false;
    * = SdpSession.parse(alloc, "v=0\r\ns=-\r\n") catch |e| {
        expect_eq(e == SdpError.MissingOrigin, true);
        missing_origin_ok = true;
    };
    expect_eq(missing_origin_ok, true);

    // 4. Missing s= -> MissingSessionName
    let missing_s_ok = false;
    * = SdpSession.parse(alloc, "v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\n") catch |e| {
        expect_eq(e == SdpError.MissingSessionName, true);
        missing_s_ok = true;
    };
    expect_eq(missing_s_ok, true);

    // 5. Malformed m= port -> InvalidPort
    let bad_port_ok = false;
    * = SdpSession.parse(alloc, "v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nm=audio notaport RTP/AVP 0\r\n") catch |e| {
        expect_eq(e == SdpError.InvalidPort, true);
        bad_port_ok = true;
    };
    expect_eq(bad_port_ok, true);

    // 6. Malformed m= missing formats -> InvalidMediaLine
    let bad_media_ok = false;
    * = SdpSession.parse(alloc, "v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\nm=audio 9 RTP/AVP\r\n") catch |e| {
        expect_eq(e == SdpError.InvalidMediaLine, true);
        bad_media_ok = true;
    };
    expect_eq(bad_media_ok, true);

    // 7. Malformed line without '=' -> MalformedLine
    let bad_line_ok = false;
    * = SdpSession.parse(alloc, "v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=-\r\ninvalid_line\r\n") catch |e| {
        expect_eq(e == SdpError.MalformedLine, true);
        bad_line_ok = true;
    };
    expect_eq(bad_line_ok, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;
    const raw =
        "v=0\r\n" ++
        "o=alice 2890844526 2890844526 IN IP4 host.example.com\r\n" ++
        "s=Call\r\n" ++
        "c=IN IP4 192.0.2.1\r\n" ++
        "t=0 0\r\n" ++
        "m=audio 49170/2 RTP/AVP 0\r\n" ++
        "a=rtpmap:0 PCMU/8000\r\n" ++
        "a=sendrecv\r\n";

    let sess = cap.net.SdpSession.parse(alloc, raw)?;
    defer sess.deinit(alloc);

    expect_eq(sess.version, 0 as u32);
    expect_eq(std.mem.eql(u8, sess.connection orelse unreachable, "IN IP4 192.0.2.1"), true);

    const audio = sess.findMedia("audio") orelse unreachable;
    expect_eq(audio.port, 49170 as u16);

    const codecs = audio.rtpmaps(alloc)?;
    defer alloc.free(codecs);
    expect_eq(codecs.len, 1 as usize);
    expect_eq(std.mem.eql(u8, codecs[0].name, "PCMU"), true);

    const out = sess.format(alloc)?;
    defer alloc.free(out);
    expect_eq(out.len > 0, true);
}

test("cap:sdp Test 1: WebRTC Offer/Answer parse & format round-trip") {
    runTest1() catch unreachable;
}

test("cap:sdp Test 2: rtpmaps() codec extraction & direction() detection") {
    runTest2() catch unreachable;
}

test("cap:sdp Test 3: ICE candidates() parsing & CRLF/LF compatibility") {
    runTest3() catch unreachable;
}

test("cap:sdp Test 4: RFC 8866 error validation (InvalidVersion, MissingOrigin/SessionName, InvalidPort/MediaLine)") {
    runTest4() catch unreachable;
}

test("cap:sdp Test 5: Dynamic cap.sdp mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
