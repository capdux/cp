// Source: cp/cap/test/test_stomp_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:stomp & cap.stomp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    StompError,
    StompCommand,
    StompHeader,
    StompFrame,
    StompBuilder,
    StompStreamParser,
    encodeFrame,
    escapeHeaderValue,
    unescapeHeaderValue,
} from "cap:stomp";

fn expectStrEq(actual_opt: ([]const u8)?, expected: []const u8) {
    if (actual := actual_opt) {
        expect_eq(std.mem.eql(u8, actual, expected), true);
    } else {
        expect_eq(false, true);
    }
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    let parser = StompStreamParser.init(alloc, 4096);
    defer parser.deinit();

    // Round-trip STOMP, CONNECT, SEND, SUBSCRIBE, MESSAGE, RECEIPT, ERROR
    let b_stomp = StompBuilder.init(alloc, StompCommand.Stomp);
    defer b_stomp.deinit();
    b_stomp.addHeader("accept-version", "1.2")?;
    b_stomp.addHeader("host", "broker.local")?;
    const w_stomp = b_stomp.build()?;
    defer alloc.free(w_stomp);

    let b_connect = StompBuilder.init(alloc, StompCommand.Connect);
    defer b_connect.deinit();
    b_connect.addHeader("accept-version", "1.2")?;
    const w_connect = b_connect.build()?;
    defer alloc.free(w_connect);

    let b_sub = StompBuilder.init(alloc, StompCommand.Subscribe);
    defer b_sub.deinit();
    b_sub.addHeader("id", "sub-0")?;
    b_sub.addHeader("destination", "/topic/chat")?;
    const w_sub = b_sub.build()?;
    defer alloc.free(w_sub);

    let b_send = StompBuilder.init(alloc, StompCommand.Send);
    defer b_send.deinit();
    b_send.addHeader("destination", "/topic/chat")?;
    b_send.addHeader("content-type", "text/plain")?;
    b_send.setBody("hello stomp")?;
    const w_send = b_send.build()?;
    defer alloc.free(w_send);

    let b_msg = StompBuilder.init(alloc, StompCommand.Message);
    defer b_msg.deinit();
    b_msg.addHeader("subscription", "sub-0")?;
    b_msg.addHeader("message-id", "msg-42")?;
    b_msg.addHeader("destination", "/topic/chat")?;
    b_msg.addHeader("content-type", "application/json")?;
    b_msg.setBody("{\"ok\":true}")?;
    const w_msg = b_msg.build()?;
    defer alloc.free(w_msg);

    let b_rcpt = StompBuilder.init(alloc, StompCommand.Receipt);
    defer b_rcpt.deinit();
    b_rcpt.addHeader("receipt-id", "rcpt-9")?;
    const w_rcpt = b_rcpt.build()?;
    defer alloc.free(w_rcpt);

    let b_err = StompBuilder.init(alloc, StompCommand.Error);
    defer b_err.deinit();
    b_err.addHeader("message", "malformed")?;
    b_err.setBody("detail")?;
    const w_err = b_err.build()?;
    defer alloc.free(w_err);

    parser.feed(w_stomp)?;
    parser.feed(w_connect)?;
    parser.feed(w_sub)?;
    parser.feed(w_send)?;
    parser.feed(w_msg)?;
    parser.feed(w_rcpt)?;
    parser.feed(w_err)?;

    if (f1 := parser.nextFrame()) {
        defer f1.deinit(alloc);
        expect_eq(f1.command == StompCommand.Stomp, true);
        expectStrEq(f1.header("host"), "broker.local");
    } else {
        expect_eq(false, true);
    }

    if (f2 := parser.nextFrame()) {
        defer f2.deinit(alloc);
        expect_eq(f2.command == StompCommand.Connect, true);
    } else {
        expect_eq(false, true);
    }

    if (f3 := parser.nextFrame()) {
        defer f3.deinit(alloc);
        expect_eq(f3.command == StompCommand.Subscribe, true);
        expectStrEq(f3.header("id"), "sub-0");
    } else {
        expect_eq(false, true);
    }

    if (f4 := parser.nextFrame()) {
        defer f4.deinit(alloc);
        expect_eq(f4.command == StompCommand.Send, true);
        expectStrEq(f4.contentType(), "text/plain");
        expectStrEq(f4.header("content-length"), "11");
        expect_eq(std.mem.eql(u8, f4.body, "hello stomp"), true);
    } else {
        expect_eq(false, true);
    }

    if (f5 := parser.nextFrame()) {
        defer f5.deinit(alloc);
        expect_eq(f5.command == StompCommand.Message, true);
        expectStrEq(f5.subscription(), "sub-0");
        expectStrEq(f5.messageId(), "msg-42");
        expectStrEq(f5.contentType(), "application/json");
        expect_eq(std.mem.eql(u8, f5.body, "{\"ok\":true}"), true);
    } else {
        expect_eq(false, true);
    }

    if (f6 := parser.nextFrame()) {
        defer f6.deinit(alloc);
        expect_eq(f6.command == StompCommand.Receipt, true);
        expectStrEq(f6.header("receipt-id"), "rcpt-9");
    } else {
        expect_eq(false, true);
    }

    if (f7 := parser.nextFrame()) {
        defer f7.deinit(alloc);
        expect_eq(f7.command == StompCommand.Error, true);
        expect_eq(std.mem.eql(u8, f7.body, "detail"), true);
    } else {
        expect_eq(false, true);
    }
    expect_eq(parser.nextFrame() == null, true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // 1. Direct escapeHeaderValue & unescapeHeaderValue for \r, \n, :, \
    const raw = "a:b\\c\rd\ne";
    const escaped = escapeHeaderValue(alloc, raw)?;
    defer alloc.free(escaped);
    expect_eq(std.mem.eql(u8, escaped, "a\\cb\\\\c\\rd\\ne"), true);

    const unescaped = unescapeHeaderValue(alloc, escaped)?;
    defer alloc.free(unescaped);
    expect_eq(std.mem.eql(u8, unescaped, raw), true);

    // 2. Repeated headers: STOMP 1.2 takes the FIRST occurrence
    let hdrs: [3]StompHeader = [
        StompHeader{ name: "destination", value: "/queue/first:1" },
        StompHeader{ name: "x-meta", value: "line1\nline2\\end\r" },
        StompHeader{ name: "destination", value: "/queue/second" },
    ];
    const send_frame = StompFrame{
        command: StompCommand.Send,
        headers: &hdrs,
        body: "",
    };
    const wire = encodeFrame(alloc, send_frame)?;
    defer alloc.free(wire);

    let parser = StompStreamParser.init(alloc, 4096);
    defer parser.deinit();
    parser.feed(wire)?;

    if (decoded := parser.nextFrame()) {
        defer decoded.deinit(alloc);
        expectStrEq(decoded.header("destination"), "/queue/first:1");
        expectStrEq(decoded.header("x-meta"), "line1\nline2\\end\r");
    } else {
        expect_eq(false, true);
    }

    // 3. CONNECT & CONNECTED do NOT escape/unescape backslashes or colons per STOMP 1.2
    let conn_hdrs: [2]StompHeader = [
        StompHeader{ name: "passcode", value: "p\\c:123" },
        StompHeader{ name: "host", value: "my:host" },
    ];
    const conn_wire = encodeFrame(alloc, StompFrame{
        command: StompCommand.Connect,
        headers: &conn_hdrs,
        body: "",
    })?;
    defer alloc.free(conn_wire);

    parser.feed(conn_wire)?;
    if (decoded_conn := parser.nextFrame()) {
        defer decoded_conn.deinit(alloc);
        expectStrEq(decoded_conn.header("passcode"), "p\\c:123");
        expectStrEq(decoded_conn.header("host"), "my:host");
    } else {
        expect_eq(false, true);
    }
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // 1. Inter-frame heartbeat EOLs (\n and \r\n), CRLF headers, embedded NUL binary body via content-length,
    //    and a second NUL-terminated frame without content-length (coalesced in one stream).
    const bin_body = "AB\x00CD\x00EF";
    const stream = "\n\r\n\nMESSAGE\r\nsubscription:s1\r\nmessage-id:m1\r\ncontent-length:8\r\n\r\nAB\x00CD\x00EF\x00\r\n\nDISCONNECT\nreceipt:r-77\n\n\x00\n";

    // Stress-test arbitrary 1..5 byte chunking across header/body/EOL boundaries
    let parser = StompStreamParser.init(alloc, 1024);
    defer parser.deinit();

    let offset: usize = 0;
    let step: usize = 1;
    while (offset < stream.len) {
        const end = if (offset + step <= stream.len) offset + step else stream.len;
        parser.feed(stream[offset..end])?;
        offset = end;
        step = (step % 5) + 1;
    }

    if (f1 := parser.nextFrame()) {
        defer f1.deinit(alloc);
        expect_eq(f1.command == StompCommand.Message, true);
        expectStrEq(f1.subscription(), "s1");
        expectStrEq(f1.messageId(), "m1");
        expect_eq(f1.body.len, 8 as usize);
        expect_eq(std.mem.eql(u8, f1.body, bin_body), true);
    } else {
        expect_eq(false, true);
    }

    if (f2 := parser.nextFrame()) {
        defer f2.deinit(alloc);
        expect_eq(f2.command == StompCommand.Disconnect, true);
        expectStrEq(f2.header("receipt"), "r-77");
        expect_eq(f2.body.len, 0 as usize);
    } else {
        expect_eq(false, true);
    }
    expect_eq(parser.nextFrame() == null, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. InvalidCommand
    let p1 = StompStreamParser.init(alloc, 1024);
    defer p1.deinit();
    let got_bad_cmd = false;
    * = p1.feed("BOGUS\n\n\x00") catch e => blk1: {
        got_bad_cmd = (e == StompError.InvalidCommand);
        break :blk1;
    };
    expect_eq(got_bad_cmd, true);

    // 2. InvalidEscape
    let got_bad_esc = false;
    * = unescapeHeaderValue(alloc, "abc\\qdef") catch e => blk2: {
        got_bad_esc = (e == StompError.InvalidEscape);
        break :blk2 "";
    };
    expect_eq(got_bad_esc, true);

    let p2 = StompStreamParser.init(alloc, 1024);
    defer p2.deinit();
    let got_bad_hdr_esc = false;
    * = p2.feed("SEND\nbad:val\\x\n\n\x00") catch e => blk2b: {
        got_bad_hdr_esc = (e == StompError.InvalidEscape);
        break :blk2b;
    };
    expect_eq(got_bad_hdr_esc, true);

    // 3. InvalidHeader (missing colon)
    let p3 = StompStreamParser.init(alloc, 1024);
    defer p3.deinit();
    let got_bad_hdr = false;
    * = p3.feed("SEND\nno_colon_here\n\n\x00") catch e => blk3: {
        got_bad_hdr = (e == StompError.InvalidHeader);
        break :blk3;
    };
    expect_eq(got_bad_hdr, true);

    // 4. TruncatedFrame (missing NUL terminator after content-length bytes)
    let p4 = StompStreamParser.init(alloc, 1024);
    defer p4.deinit();
    let got_trunc = false;
    * = p4.feed("SEND\ncontent-length:3\n\nabcX") catch e => blk4: {
        got_trunc = (e == StompError.TruncatedFrame);
        break :blk4;
    };
    expect_eq(got_trunc, true);

    // 5. InvalidContentLength
    let p5 = StompStreamParser.init(alloc, 1024);
    defer p5.deinit();
    let got_bad_cl = false;
    * = p5.feed("SEND\ncontent-length:12px\n\n\x00") catch e => blk5: {
        got_bad_cl = (e == StompError.InvalidContentLength);
        break :blk5;
    };
    expect_eq(got_bad_cl, true);

    // 6. BodyTooLarge (both via content-length and NUL-scanned body)
    let p6 = StompStreamParser.init(alloc, 8);
    defer p6.deinit();
    let got_too_large_cl = false;
    * = p6.feed("SEND\ncontent-length:9\n\n123456789\x00") catch e => blk6: {
        got_too_large_cl = (e == StompError.BodyTooLarge);
        break :blk6;
    };
    expect_eq(got_too_large_cl, true);

    let p7 = StompStreamParser.init(alloc, 8);
    defer p7.deinit();
    let got_too_large_nul = false;
    * = p7.feed("SEND\n\n123456789\x00") catch e => blk7: {
        got_too_large_nul = (e == StompError.BodyTooLarge);
        break :blk7;
    };
    expect_eq(got_too_large_nul, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mount: import cap from "cap"; cap.stomp
    let builder = cap.stomp.StompBuilder.init(alloc, cap.stomp.StompCommand.Send);
    defer builder.deinit();
    builder.addHeader("destination", "/queue/cap:stomp")?;
    builder.setBody("ok\x00cap")?;

    const wire = builder.build()?;
    defer alloc.free(wire);

    let parser = cap.stomp.StompStreamParser.init(alloc, 4096);
    defer parser.deinit();
    parser.feed(wire)?;

    if (frame := parser.nextFrame()) {
        defer frame.deinit(alloc);
        expect_eq(frame.command == cap.stomp.StompCommand.Send, true);
        expectStrEq(frame.header("destination"), "/queue/cap:stomp");
        expect_eq(std.mem.eql(u8, frame.body, "ok\x00cap"), true);
    } else {
        expect_eq(false, true);
    }
}

test("cap:stomp STOMP/CONNECT/SEND/SUBSCRIBE/MESSAGE/RECEIPT/ERROR frame encode & parse round-trip") {
    runTest1() catch unreachable;
}

test("cap:stomp STOMP 1.2 header escaping/unescaping, repeated first-wins & CONNECT/CONNECTED rules") {
    runTest2() catch unreachable;
}

test("cap:stomp StompStreamParser 1..5 byte chunking, heartbeat EOLs & embedded NUL binary body") {
    runTest3() catch unreachable;
}

test("cap:stomp InvalidCommand, InvalidEscape, InvalidHeader, TruncatedFrame, InvalidContentLength & BodyTooLarge") {
    runTest4() catch unreachable;
}

test("cap.stomp dynamic mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
