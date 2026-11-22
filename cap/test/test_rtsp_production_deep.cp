// Source: cp/cap/test/test_rtsp_production_deep.cp
// Deep Production Test Suite for cap:rtsp (RFC 2326 RTSP Protocol Codec, Transport Header & Interleaved '$' Framing)
// Covers:
//   1. RtspBuilder OPTIONS/DESCRIBE/SETUP/PLAY/TEARDOWN & Response round-trip (with Content-Length SDP body & auto-CSeq)
//   2. parseTransportHeader / formatTransportHeader for interleaved=0-1, client_port, server_port, ssrc, unicast, mode
//   3. RtspStreamParser 1..7 byte arbitrary chunking over mixed RTSP text messages + '$' interleaved RTP/RTCP frames
//   4. Security & malformed guards: InvalidVersion (RTSP/2.5), HeaderInjection (CR/LF), InvalidContentLength, PayloadTooLarge
//   5. Dynamic lazy namespace mounting (`import cap from "cap"; cap.rtsp`) & `std.testing.allocator` zero memory leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    RtspMethod,
    RtspMessage,
    RtspBuilder,
    RtspStreamParser,
    RtspStreamItem,
    TransportSpec,
    RtspError,
    parseTransportHeader,
    formatTransportHeader,
    encodeInterleavedFrame,
} from "cap:net";
import cap from "cap";

test("1. RtspBuilder OPTIONS/DESCRIBE/SETUP/PLAY/TEARDOWN & SDP Response Round-Trip") {
    const alloc = std.testing.allocator;

    let builder = RtspBuilder.init(alloc);
    defer builder.deinit();

    // Request 1: OPTIONS (auto CSeq=1)
    * = builder.request(.Options, "rtsp://example.com/media.mp4") catch unreachable;
    * = builder.header("User-Agent", "CP-RTSP/1.0") catch unreachable;
    const req1_bytes = builder.buildBytes() catch unreachable;
    defer alloc.free(req1_bytes);

    // Request 2: DESCRIBE (auto CSeq=2)
    * = builder.request(.Describe, "rtsp://example.com/media.mp4") catch unreachable;
    * = builder.header("Accept", "application/sdp") catch unreachable;
    const req2_bytes = builder.buildBytes() catch unreachable;
    defer alloc.free(req2_bytes);

    // Request 3: SETUP (auto CSeq=3)
    * = builder.request(.Setup, "rtsp://example.com/media.mp4/trackID=0") catch unreachable;
    * = builder.header("Transport", "RTP/AVP/TCP;unicast;interleaved=0-1") catch unreachable;
    const req3_bytes = builder.buildBytes() catch unreachable;
    defer alloc.free(req3_bytes);

    // Request 4: PLAY (auto CSeq=4)
    * = builder.request(.Play, "rtsp://example.com/media.mp4") catch unreachable;
    * = builder.session("98765432") catch unreachable;
    const req4_bytes = builder.buildBytes() catch unreachable;
    defer alloc.free(req4_bytes);

    // Request 5: TEARDOWN (auto CSeq=5)
    * = builder.request(.Teardown, "rtsp://example.com/media.mp4") catch unreachable;
    * = builder.session("98765432") catch unreachable;
    const req5_bytes = builder.buildBytes() catch unreachable;
    defer alloc.free(req5_bytes);

    // Response with SDP body & auto Content-Length
    const sdp = "v=0\r\no=- 0 0 IN IP4 127.0.0.1\r\ns=Live\r\nm=video 0 RTP/AVP 96\r\n";
    * = builder.response(200, "OK") catch unreachable;
    * = builder.cseq(2) catch unreachable;
    * = builder.header("Content-Type", "application/sdp") catch unreachable;
    * = builder.header("Content-Base", "rtsp://example.com/media.mp4/") catch unreachable;
    * = builder.header("Session", "98765432;timeout=60") catch unreachable;
    * = builder.setBody(sdp) catch unreachable;
    const resp_bytes = builder.buildBytes() catch unreachable;
    defer alloc.free(resp_bytes);

    let parser = RtspStreamParser.init(alloc, 65536);
    defer parser.deinit();

    parser.feed(req1_bytes) catch unreachable;
    parser.feed(req2_bytes) catch unreachable;
    parser.feed(req3_bytes) catch unreachable;
    parser.feed(req4_bytes) catch unreachable;
    parser.feed(req5_bytes) catch unreachable;
    parser.feed(resp_bytes) catch unreachable;

    // Verify OPTIONS CSeq=1
    if (item1 := parser.nextFrame() catch unreachable) {
        defer item1.deinit(alloc);
        switch (item1) {
            .message m => {
                expect_eq(m.cseq().?, 1 as u32);
                expect_eq(std.mem.eql(u8, m.header("user-agent").?, "CP-RTSP/1.0"), true);
            },
            .interleaved _ => unreachable,
        }
    } else {
        unreachable;
    }

    // Verify DESCRIBE CSeq=2
    if (item2 := parser.nextFrame() catch unreachable) {
        defer item2.deinit(alloc);
        switch (item2) {
            .message m => {
                expect_eq(m.cseq().?, 2 as u32);
                switch (m.start_line) {
                    .request r => expect_eq(r.method == .Describe, true),
                    .response _ => unreachable,
                }
            },
            .interleaved _ => unreachable,
        }
    } else {
        unreachable;
    }

    // Verify SETUP CSeq=3, PLAY CSeq=4, TEARDOWN CSeq=5
    if (item3 := parser.nextFrame() catch unreachable) {
        defer item3.deinit(alloc);
        switch (item3) {
            .message m => expect_eq(m.cseq().?, 3 as u32),
            .interleaved _ => unreachable,
        }
    } else {
        unreachable;
    }
    if (item4 := parser.nextFrame() catch unreachable) {
        defer item4.deinit(alloc);
        switch (item4) {
            .message m => {
                expect_eq(m.cseq().?, 4 as u32);
                expect_eq(std.mem.eql(u8, m.sessionId().?, "98765432"), true);
            },
            .interleaved _ => unreachable,
        }
    } else {
        unreachable;
    }
    if (item5 := parser.nextFrame() catch unreachable) {
        defer item5.deinit(alloc);
        switch (item5) {
            .message m => expect_eq(m.cseq().?, 5 as u32),
            .interleaved _ => unreachable,
        }
    } else {
        unreachable;
    }

    // Verify 200 OK SDP Response
    if (item6 := parser.nextFrame() catch unreachable) {
        defer item6.deinit(alloc);
        switch (item6) {
            .message m => {
                expect_eq(m.cseq().?, 2 as u32);
                expect_eq(std.mem.eql(u8, m.sessionId().?, "98765432"), true);
                expect_eq(std.mem.eql(u8, m.contentBase().?, "rtsp://example.com/media.mp4/"), true);
                expect_eq(std.mem.eql(u8, m.body, sdp), true);
            },
            .interleaved _ => unreachable,
        }
    } else {
        unreachable;
    }
}

test("2. parseTransportHeader & formatTransportHeader Round-Trip") {
    const alloc = std.testing.allocator;

    const raw_tcp = "RTP/AVP/TCP;unicast;interleaved=0-1;ssrc=deadbeef";
    let spec_tcp = parseTransportHeader(alloc, raw_tcp) catch unreachable;
    defer spec_tcp.deinit(alloc);

    expect_eq(std.mem.eql(u8, spec_tcp.protocol, "RTP/AVP/TCP"), true);
    expect_eq(spec_tcp.unicast, true);
    expect_eq(spec_tcp.multicast, false);
    expect_eq(spec_tcp.interleaved.?.rtp, 0 as u16);
    expect_eq(spec_tcp.interleaved.?.rtcp.?, 1 as u16);
    expect_eq(std.mem.eql(u8, spec_tcp.ssrc.?, "deadbeef"), true);

    const fmt_tcp = formatTransportHeader(alloc, spec_tcp) catch unreachable;
    defer alloc.free(fmt_tcp);
    expect_eq(std.mem.eql(u8, fmt_tcp, raw_tcp), true);

    const raw_udp = "RTP/AVP;unicast;client_port=5000-5001;server_port=6000-6001;mode=\"PLAY\"";
    let spec_udp = parseTransportHeader(alloc, raw_udp) catch unreachable;
    defer spec_udp.deinit(alloc);

    expect_eq(std.mem.eql(u8, spec_udp.protocol, "RTP/AVP"), true);
    expect_eq(spec_udp.unicast, true);
    expect_eq(spec_udp.client_port.?.rtp, 5000 as u16);
    expect_eq(spec_udp.client_port.?.rtcp.?, 5001 as u16);
    expect_eq(spec_udp.server_port.?.rtp, 6000 as u16);
    expect_eq(spec_udp.server_port.?.rtcp.?, 6001 as u16);
    expect_eq(std.mem.eql(u8, spec_udp.mode.?, "PLAY"), true);

    const fmt_udp = formatTransportHeader(alloc, spec_udp) catch unreachable;
    defer alloc.free(fmt_udp);
    expect_eq(std.mem.eql(u8, fmt_udp, raw_udp), true);
}

test("3. RtspStreamParser Mixed Text Messages & '$' Interleaved Frames Across 1..7 Byte Chunks") {
    const alloc = std.testing.allocator;

    const rtp_payload = "\x80\x60\x00\x01\x00\x00\x00\xa0\xde\xad\xbe\xefHELLO_RTP";
    const rtcp_payload = "\x80\xc8\x00\x06\xde\xad\xbe\xefRTCP_SR";

    const f_rtp = encodeInterleavedFrame(alloc, 0, rtp_payload) catch unreachable;
    defer alloc.free(f_rtp);
    const f_rtcp = encodeInterleavedFrame(alloc, 1, rtcp_payload) catch unreachable;
    defer alloc.free(f_rtcp);

    let builder = RtspBuilder.init(alloc);
    defer builder.deinit();
    * = builder.response(200, "OK") catch unreachable;
    * = builder.cseq(7) catch unreachable;
    * = builder.setBody("v=0\r\n") catch unreachable;
    const msg_wire = builder.buildBytes() catch unreachable;
    defer alloc.free(msg_wire);

    let stream: std.ArrayList(u8) = .empty;
    defer stream.deinit(alloc);
    stream.appendSlice(alloc, f_rtp) catch unreachable;
    stream.appendSlice(alloc, msg_wire) catch unreachable;
    stream.appendSlice(alloc, f_rtcp) catch unreachable;
    stream.appendSlice(alloc, f_rtp) catch unreachable;

    let parser = RtspStreamParser.init(alloc, 65536);
    defer parser.deinit();

    // Feed in varying 1..7 byte chunks
    let pos: usize = 0;
    let step: usize = 1;
    let decoded_count: usize = 0;
    let rtp_count: usize = 0;
    let rtcp_count: usize = 0;
    let msg_count: usize = 0;

    while (pos < stream.items.len) {
        const end = @min(pos + step, stream.items.len);
        parser.feed(stream.items[pos..end]) catch unreachable;
        pos = end;
        step = (step % 7) + 1;

        while (true) {
            const opt = parser.nextFrame() catch unreachable;
            if (item := opt) {
                defer item.deinit(alloc);
                decoded_count += 1;
                switch (item) {
                    .interleaved f => {
                        if (f.channel == 0) {
                            expect_eq(std.mem.eql(u8, f.data, rtp_payload), true);
                            rtp_count += 1;
                        } else if (f.channel == 1) {
                            expect_eq(std.mem.eql(u8, f.data, rtcp_payload), true);
                            rtcp_count += 1;
                        } else {
                            unreachable;
                        }
                    },
                    .message m => {
                        expect_eq(m.cseq().?, 7 as u32);
                        expect_eq(std.mem.eql(u8, m.body, "v=0\r\n"), true);
                        msg_count += 1;
                    },
                }
            } else {
                break;
            }
        }
    }

    expect_eq(decoded_count, 4 as usize);
    expect_eq(rtp_count, 2 as usize);
    expect_eq(rtcp_count, 1 as usize);
    expect_eq(msg_count, 1 as usize);
}

test("4. Security & Malformed Guards: InvalidVersion, HeaderInjection, InvalidContentLength, PayloadTooLarge") {
    const alloc = std.testing.allocator;

    // 4a. InvalidVersion: RTSP/2.5
    {
        let p = RtspStreamParser.init(alloc, 4096);
        defer p.deinit();
        p.feed("OPTIONS rtsp://example.com RTSP/2.5\r\nCSeq: 1\r\n\r\n") catch unreachable;
        let got_err = false;
        * = p.nextFrame() catch |e| {
            expect_eq(e == RtspError.InvalidVersion, true);
            got_err = true;
        };
        expect_eq(got_err, true);
    }

    // 4b. HeaderInjection via RtspBuilder
    {
        let b = RtspBuilder.init(alloc);
        defer b.deinit();
        * = b.request(.Options, "rtsp://example.com") catch unreachable;
        let got_inj = false;
        * = b.header("User-Agent", "Evil\r\nX-Injected: 1") catch |e| {
            expect_eq(e == RtspError.HeaderInjection, true);
            got_inj = true;
        };
        expect_eq(got_inj, true);
    }

    // 4c. InvalidContentLength
    {
        let p = RtspStreamParser.init(alloc, 4096);
        defer p.deinit();
        p.feed("RTSP/1.0 200 OK\r\nCSeq: 1\r\nContent-Length: abc\r\n\r\n") catch unreachable;
        let got_cl_err = false;
        * = p.nextFrame() catch |e| {
            expect_eq(e == RtspError.InvalidContentLength, true);
            got_cl_err = true;
        };
        expect_eq(got_cl_err, true);
    }

    // 4d. PayloadTooLarge on interleaved '$' frame
    {
        let p = RtspStreamParser.init(alloc, 16);
        defer p.deinit();
        const big_frame = encodeInterleavedFrame(alloc, 0, "01234567890123456789") catch unreachable;
        defer alloc.free(big_frame);
        p.feed(big_frame) catch unreachable;
        let got_too_large = false;
        * = p.nextFrame() catch |e| {
            expect_eq(e == RtspError.PayloadTooLarge, true);
            got_too_large = true;
        };
        expect_eq(got_too_large, true);
    }
}

test("5. Dynamic Lazy Namespace Mounting (cap.rtsp) & Zero Memory Leaks") {
    const alloc = std.testing.allocator;

    const rtsp = cap.net;
    let b = rtsp.RtspBuilder.init(alloc);
    defer b.deinit();

    let spec = rtsp.parseTransportHeader(alloc, "RTP/AVP/TCP;unicast;interleaved=2-3") catch unreachable;
    defer spec.deinit(alloc);

    * = b.request(.Setup, "rtsp://127.0.0.1:8554/stream") catch unreachable;
    * = b.transport(spec) catch unreachable;
    const wire = b.buildBytes() catch unreachable;
    defer alloc.free(wire);

    let p = rtsp.RtspStreamParser.init(alloc, 4096);
    defer p.deinit();
    p.feed(wire) catch unreachable;

    if (item := p.nextFrame() catch unreachable) {
        defer item.deinit(alloc);
        switch (item) {
            .message m => {
                expect_eq(m.cseq().?, 1 as u32);
                const tr = m.header("Transport").?;
                expect_eq(std.mem.eql(u8, tr, "RTP/AVP/TCP;unicast;interleaved=2-3"), true);
            },
            .interleaved _ => unreachable,
        }
    } else {
        unreachable;
    }
}
