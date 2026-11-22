// Source: cp/cap/test/test_sse_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:sse & cap.sse

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SseError,
    SseEvent,
    SseParser,
    SseEncoder,
} from "cap:web";

fn runTest1(): !void {
    const alloc = std.testing.allocator;
    let parser = SseParser.init(alloc, 4096);
    defer parser.deinit();

    // 1. Comments only & event-without-data must NOT emit any event
    parser.feed(": keep-alive comment\n\nevent: ignored_no_data\n\n")?;
    expect_eq(parser.nextEvent() == null, true);

    // 2. Multi-line data concatenation, single leading space stripping, default event_type = "message"
    parser.feed(": header comment\ndata: first line\ndata:  second keeps one space\ndata:third no space\n\n")?;
    const ev1 = parser.nextEvent().?;
    defer ev1.deinit(alloc);

    expect_eq(std.mem.eql(u8, ev1.event_type, "message"), true);
    expect_eq(std.mem.eql(u8, ev1.data, "first line\n second keeps one space\nthird no space"), true);
    expect_eq(ev1.id == null, true);
    expect_eq(ev1.retry_ms == null, true);

    // 3. Explicit event type & empty data line (data:\n\n emits empty data)
    parser.feed("event: ping\ndata:\n\n")?;
    const ev2 = parser.nextEvent().?;
    defer ev2.deinit(alloc);
    expect_eq(std.mem.eql(u8, ev2.event_type, "ping"), true);
    expect_eq(std.mem.eql(u8, ev2.data, ""), true);

    // Next event resets event_type back to default "message"
    parser.feed("data: back to default\n\n")?;
    const ev3 = parser.nextEvent().?;
    defer ev3.deinit(alloc);
    expect_eq(std.mem.eql(u8, ev3.event_type, "message"), true);
    expect_eq(std.mem.eql(u8, ev3.data, "back to default"), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;
    let parser = SseParser.init(alloc, 4096);
    defer parser.deinit();

    // UTF-8 BOM + mixed \r\n, \r, \n + id (with NUL ignore check) + retry (valid & invalid)
    const stream = "\xEF\xBB\xBFid: id-1\r\nretry: 1500\r\nevent: update\rdata: part1\rdata: part2\r\n\r\nid: bad\x00id\nretry: 12abc\ndata: after-nul\n\n";

    // Feed 1 byte at a time to stress-test BOM split & \r / \n cross-chunk boundaries
    let i: usize = 0;
    while (i < stream.len) : (i += 1) {
        parser.feed(stream[i..(i + 1)])?;
    }

    const ev1 = parser.nextEvent().?;
    defer ev1.deinit(alloc);
    expect_eq(std.mem.eql(u8, ev1.event_type, "update"), true);
    expect_eq(std.mem.eql(u8, ev1.data, "part1\npart2"), true);
    expect_eq(std.mem.eql(u8, ev1.id.?, "id-1"), true);
    expect_eq(ev1.retry_ms.?, 1500 as u64);

    const ev2 = parser.nextEvent().?;
    defer ev2.deinit(alloc);
    expect_eq(std.mem.eql(u8, ev2.event_type, "message"), true);
    expect_eq(std.mem.eql(u8, ev2.data, "after-nul"), true);
    // id with \x00 must be ignored, keeping previous "id-1"
    expect_eq(std.mem.eql(u8, ev2.id.?, "id-1"), true);
    expect_eq(std.mem.eql(u8, parser.lastEventId(), "id-1"), true);
    // non-ASCII-digit retry must be ignored, keeping 1500
    expect_eq(parser.retryMs().?, 1500 as u64);
    expect_eq(parser.nextEvent() == null, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // 1. Encode multi-line data + event_type + id + retry and round-trip through SseParser
    const encoded = SseEncoder.formatEvent(alloc, "delta", "line1\nline2\nline3", "42", 2500)?;
    defer alloc.free(encoded);

    const comment = SseEncoder.formatComment(alloc, "heartbeat")?;
    defer alloc.free(comment);

    let parser = SseParser.init(alloc, 4096);
    defer parser.deinit();
    parser.feed(comment)?;
    parser.feed(encoded)?;

    const ev = parser.nextEvent().?;
    defer ev.deinit(alloc);
    expect_eq(std.mem.eql(u8, ev.event_type, "delta"), true);
    expect_eq(std.mem.eql(u8, ev.data, "line1\nline2\nline3"), true);
    expect_eq(std.mem.eql(u8, ev.id.?, "42"), true);
    expect_eq(ev.retry_ms.?, 2500 as u64);

    // 2. Reject CR/LF injection in event_type and id with SseError.InvalidEventData
    let bad_ev_lf = false;
    * = SseEncoder.formatEvent(alloc, "bad\nevent", "ok", null, null) catch e => err_ev_lf: {
        bad_ev_lf = (e == SseError.InvalidEventData);
        break :err_ev_lf "";
    };
    expect_eq(bad_ev_lf, true);

    let bad_id_cr = false;
    * = SseEncoder.formatEvent(alloc, "ok", "payload", "id\rinjection", null) catch e => err_id_cr: {
        bad_id_cr = (e == SseError.InvalidEventData);
        break :err_id_cr "";
    };
    expect_eq(bad_id_cr, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // Enforce max_event_bytes resource exhaustion guard
    let parser = SseParser.init(alloc, 32);
    defer parser.deinit();

    let overflow_single = false;
    * = parser.feed("data: 0123456789012345678901234567890123456789\n\n") catch e => err_ov1: {
        overflow_single = (e == SseError.EventBufferOverflow);
        break :err_ov1;
    };
    expect_eq(overflow_single, true);

    // Multi-line cumulative data overflow
    let parser2 = SseParser.init(alloc, 24);
    defer parser2.deinit();
    parser2.feed("data: 1234567890\n")?;
    parser2.feed("data: 1234567890\n")?;

    let overflow_multi = false;
    * = parser2.feed("data: 1234567890\n\n") catch e => err_ov2: {
        overflow_multi = (e == SseError.EventBufferOverflow);
        break :err_ov2;
    };
    expect_eq(overflow_multi, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mount: import cap from "cap"; cap.sse
    const frame = cap.web.SseEncoder.formatEvent(alloc, "stream", "chunk-a\nchunk-b", "99", 500)?;
    defer alloc.free(frame);

    let parser = cap.web.SseParser.init(alloc, 1024);
    defer parser.deinit();
    parser.feed(frame)?;

    const ev = parser.nextEvent().?;
    defer ev.deinit(alloc);
    expect_eq(std.mem.eql(u8, ev.event_type, "stream"), true);
    expect_eq(std.mem.eql(u8, ev.data, "chunk-a\nchunk-b"), true);
    expect_eq(std.mem.eql(u8, ev.id.?, "99"), true);
    expect_eq(ev.retry_ms.?, 500 as u64);
    expect_eq(std.mem.eql(u8, parser.lastEventId(), "99"), true);
    expect_eq(parser.retryMs().?, 500 as u64);
}

test("cap:sse W3C multi-line data, single-space strip, comments, empty data & default event_type") {
    runTest1() catch unreachable;
}

test("cap:sse arbitrary 1-byte chunking, CR/LF splits, UTF-8 BOM strip, Last-Event-ID NUL ignore & retry") {
    runTest2() catch unreachable;
}

test("cap:sse SseEncoder multi-line round-trip & CR/LF injection guard InvalidEventData") {
    runTest3() catch unreachable;
}

test("cap:sse max_event_bytes EventBufferOverflow resource exhaustion protection") {
    runTest4() catch unreachable;
}

test("cap.sse dynamic mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
