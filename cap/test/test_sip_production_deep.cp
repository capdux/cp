// Source: cp/cap/test/test_sip_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:sip & cap.sip

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SipError,
    SipMethod,
    SipHeader,
    SipStartLine,
    SipMessage,
    SipBuilder,
    parse,
    normalizeHeaderName,
} from "cap:sip";

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    const raw_invite =
        "INVITE sip:bob@biloxi.example.com SIP/2.0\r\n" ++
        "Via: SIP/2.0/UDP pc33.atlanta.example.com;branch=z9hG4bK776asdhds\r\n" ++
        "Via: SIP/2.0/UDP bigbox3.site3.atlanta.example.com;branch=z9hG4bK4b43c2ff8\r\n" ++
        "Max-Forwards: 70\r\n" ++
        "To: Bob <sip:bob@biloxi.example.com>\r\n" ++
        "From: Alice <sip:alice@atlanta.example.com>;tag=1928301774\r\n" ++
        "Call-ID: a84b4c76e66710@pc33.atlanta.example.com\r\n" ++
        "CSeq: 314159 INVITE\r\n" ++
        "Contact: <sip:alice@pc33.atlanta.example.com>\r\n" ++
        "Content-Type: application/sdp\r\n" ++
        "Content-Length: 42\r\n" ++
        "\r\n" ++
        "v=0\r\no=alice 2890844526 2890844526 IN IP4\r\n";

    let msg = parse(alloc, raw_invite)?;
    defer msg.deinit(alloc);

    switch (msg.start_line) {
        .request => |req| {
            expect_eq(std.mem.eql(u8, req.method, "INVITE"), true);
            expect_eq(SipMethod.fromString(req.method), SipMethod.Invite);
            expect_eq(std.mem.eql(u8, req.uri, "sip:bob@biloxi.example.com"), true);
            expect_eq(std.mem.eql(u8, req.version, "SIP/2.0"), true);
        },
        .response => unreachable,
    }

    const vias = msg.headersAll(alloc, "via")?;
    defer alloc.free(vias);
    expect_eq(vias.len, 2 as usize);
    expect_eq(std.mem.eql(u8, msg.viaTop() orelse unreachable, "SIP/2.0/UDP pc33.atlanta.example.com;branch=z9hG4bK776asdhds"), true);
    expect_eq(std.mem.eql(u8, msg.callId() orelse unreachable, "a84b4c76e66710@pc33.atlanta.example.com"), true);
    expect_eq(std.mem.eql(u8, msg.cseq() orelse unreachable, "314159 INVITE"), true);
    expect_eq(msg.body.len, 42 as usize);

    const formatted = msg.format(alloc)?;
    defer alloc.free(formatted);
    expect_eq(std.mem.eql(u8, formatted, raw_invite), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    const compact_resp =
        "SIP/2.0 200 OK\r\n" ++
        "v: SIP/2.0/UDP server10.biloxi.example.com;branch=z9hG4bKnashds8\r\n" ++
        "t: Bob <sip:bob@biloxi.example.com>;tag=a6c85cf\r\n" ++
        "f: Alice <sip:alice@atlanta.example.com>;tag=1928301774\r\n" ++
        "i: a84b4c76e66710\r\n" ++
        "CSeq: 314159 INVITE\r\n" ++
        "m: <sip:bob@192.0.2.4>\r\n" ++
        "c: application/sdp\r\n" ++
        "k: timer, 100rel\r\n" ++
        "s: SDP Session\r\n" ++
        "l: 0\r\n" ++
        "\r\n";

    let msg = parse(alloc, compact_resp)?;
    defer msg.deinit(alloc);

    switch (msg.start_line) {
        .response => |resp| {
            expect_eq(std.mem.eql(u8, resp.version, "SIP/2.0"), true);
            expect_eq(resp.status_code, 200 as u16);
            expect_eq(std.mem.eql(u8, resp.reason, "OK"), true);
        },
        .request => unreachable,
    }

    expect_eq(std.mem.eql(u8, normalizeHeaderName("v"), "Via"), true);
    expect_eq(std.mem.eql(u8, normalizeHeaderName("l"), "Content-Length"), true);
    expect_eq(std.mem.eql(u8, msg.header("Via") orelse unreachable, "SIP/2.0/UDP server10.biloxi.example.com;branch=z9hG4bKnashds8"), true);
    expect_eq(std.mem.eql(u8, msg.header("v") orelse unreachable, "SIP/2.0/UDP server10.biloxi.example.com;branch=z9hG4bKnashds8"), true);
    expect_eq(std.mem.eql(u8, msg.header("To") orelse unreachable, "Bob <sip:bob@biloxi.example.com>;tag=a6c85cf"), true);
    expect_eq(std.mem.eql(u8, msg.header("from") orelse unreachable, "Alice <sip:alice@atlanta.example.com>;tag=1928301774"), true);
    expect_eq(std.mem.eql(u8, msg.callId() orelse unreachable, "a84b4c76e66710"), true);
    expect_eq(std.mem.eql(u8, msg.header("Contact") orelse unreachable, "<sip:bob@192.0.2.4>"), true);
    expect_eq(std.mem.eql(u8, msg.header("Content-Type") orelse unreachable, "application/sdp"), true);
    expect_eq(std.mem.eql(u8, msg.header("Supported") orelse unreachable, "timer, 100rel"), true);
    expect_eq(std.mem.eql(u8, msg.header("Subject") orelse unreachable, "SDP Session"), true);
    expect_eq(std.mem.eql(u8, msg.header("Content-Length") orelse unreachable, "0"), true);

    const ringing =
        "SIP/2.0 180 Ringing\n" ++
        "i: ring-call-1\n" ++
        "l: 0\n\n";
    let msg180 = parse(alloc, ringing)?;
    defer msg180.deinit(alloc);
    switch (msg180.start_line) {
        .response => |resp| {
            expect_eq(resp.status_code, 180 as u16);
            expect_eq(std.mem.eql(u8, resp.reason, "Ringing"), true);
        },
        .request => unreachable,
    }
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // 1. SipBuilder REGISTER request with auto Content-Length
    let rb = SipBuilder.request(alloc, "REGISTER", "sip:registrar.biloxi.example.com");
    defer rb.deinit();
    * = rb.header("Via", "SIP/2.0/UDP bobspc.biloxi.example.com:5060;branch=z9hG4bKnashds7")?;
    * = rb.header("To", "Bob <sip:bob@biloxi.example.com>")?;
    * = rb.header("From", "Bob <sip:bob@biloxi.example.com>;tag=456248")?;
    * = rb.header("Call-ID", "843817637684230@998sdasdh09")?;
    * = rb.header("CSeq", "1826 REGISTER")?;
    * = rb.setBody("hello");

    const built = rb.build()?;
    defer alloc.free(built);

    let parsed_reg = parse(alloc, built)?;
    defer parsed_reg.deinit(alloc);
    expect_eq(std.mem.eql(u8, parsed_reg.header("Content-Length") orelse unreachable, "5"), true);
    expect_eq(std.mem.eql(u8, parsed_reg.body, "hello"), true);

    // 2. Multi-line folded header (RFC 3261 §7.3.1)
    const folded_msg =
        "INVITE sip:bob@biloxi.example.com SIP/2.0\n" ++
        "Subject: This is a folded\n" ++
        "   subject line\n" ++
        "\twith tab continuation\n" ++
        "Call-ID: folded-123\n" ++
        "Content-Length: 0\n\n";

    let parsed_fold = parse(alloc, folded_msg)?;
    defer parsed_fold.deinit(alloc);
    expect_eq(
        std.mem.eql(u8, parsed_fold.header("Subject") orelse unreachable, "This is a folded subject line with tab continuation"),
        true,
    );
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // InvalidVersion: SIP/1.0
    const bad_ver = "INVITE sip:a@b SIP/1.0\r\nCall-ID: 1\r\n\r\n";
    let saw_invalid_ver = false;
    * = parse(alloc, bad_ver) catch |e| {
        if (e == SipError.InvalidVersion) saw_invalid_ver = true;
    };
    expect_eq(saw_invalid_ver, true);

    // TruncatedBody: Content-Length > actual body
    const trunc = "INVITE sip:a@b SIP/2.0\r\nContent-Length: 20\r\n\r\nshort";
    let saw_trunc = false;
    * = parse(alloc, trunc) catch |e| {
        if (e == SipError.TruncatedBody) saw_trunc = true;
    };
    expect_eq(saw_trunc, true);

    // InvalidContentLength: non-numeric
    const bad_cl = "INVITE sip:a@b SIP/2.0\r\nContent-Length: abc\r\n\r\n";
    let saw_bad_cl = false;
    * = parse(alloc, bad_cl) catch |e| {
        if (e == SipError.InvalidContentLength) saw_bad_cl = true;
    };
    expect_eq(saw_bad_cl, true);

    // HeaderInjection in SipBuilder
    let b = SipBuilder.request(alloc, "OPTIONS", "sip:example.com");
    defer b.deinit();
    let saw_inj = false;
    * = b.header("X-Custom", "val\r\nInjected: evil") catch |e| {
        if (e == SipError.HeaderInjection) saw_inj = true;
    };
    expect_eq(saw_inj, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    let rb = cap.sip.SipBuilder.response(alloc, 200, "OK");
    defer rb.deinit();
    * = rb.header("v", "SIP/2.0/TCP proxy.example.com;branch=z9hG4bK123")?;
    * = rb.header("i", "dyn-call-id-999")?;
    * = rb.header("CSeq", "1 OPTIONS")?;
    const wire = rb.build()?;
    defer alloc.free(wire);

    let msg = cap.sip.parse(alloc, wire)?;
    defer msg.deinit(alloc);

    expect_eq(std.mem.eql(u8, msg.callId() orelse unreachable, "dyn-call-id-999"), true);
    expect_eq(std.mem.eql(u8, msg.viaTop() orelse unreachable, "SIP/2.0/TCP proxy.example.com;branch=z9hG4bK123"), true);
    expect_eq(std.mem.eql(u8, msg.header("Content-Length") orelse unreachable, "0"), true);
}

test("cap:sip Test 1: INVITE request with multiple Via & SDP body parse/format round-trip") {
    runTest1() catch unreachable;
}

test("cap:sip Test 2: 200 OK / 180 Ringing responses & RFC 3261 compact headers mapping") {
    runTest2() catch unreachable;
}

test("cap:sip Test 3: SipBuilder REGISTER request & multi-line folded header parsing") {
    runTest3() catch unreachable;
}

test("cap:sip Test 4: InvalidVersion, TruncatedBody, InvalidContentLength & HeaderInjection") {
    runTest4() catch unreachable;
}

test("cap:sip Test 5: Dynamic cap.sip mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
