// Source: cp/cap/test/test_sse_parser_production_deep.cp (Production deep tests for cap:sse_parser)

import { test, expect_eq } from "cap:test";
import {
    ZeroAllocSseEvent as SseEvent,
    ZeroAllocSseParser as SseParser,
    formatSseEvent,
} from "cap:http";

test("sse_parser: standard LLM delta stream with BOM, CRLF and comments") {
    const stream =
        "\xEF\xBB\xBF" ++
        ": keep-alive ping\r\n" ++
        "\r\n" ++
        "data: {\"delta\":\"hello\"}\r\n" ++
        "\r\n" ++
        ": another comment\n" ++
        "data: {\"delta\":\" world\"}\n" ++
        "\n" ++
        "data: [DONE]\r\n" ++
        "\r\n";

    let parser = SseParser.init(stream);

    const e1 = parser.next().?;
    expect_eq(std.mem.eql(u8, e1.event_type, "message"), true);
    expect_eq(std.mem.eql(u8, e1.data, "{\"delta\":\"hello\"}"), true);
    expect_eq(e1.has_id, false);
    expect_eq(e1.retry_ms == null, true);

    const e2 = parser.next().?;
    expect_eq(std.mem.eql(u8, e2.data, "{\"delta\":\" world\"}"), true);

    const e3 = parser.next().?;
    expect_eq(std.mem.eql(u8, e3.data, "[DONE]"), true);

    expect_eq(parser.next() == null, true);
}

test("sse_parser: multiline data joining, single leading space stripping and CR line endings") {
    const stream =
        "data: line1\n" ++
        "data:  two_spaces_keeps_one\r" ++
        "data:no_space\r\n" ++
        "\n";

    let parser = SseParser.init(stream);
    const ev = parser.next().?;
    expect_eq(std.mem.eql(u8, ev.data, "line1\n two_spaces_keeps_one\nno_space"), true);
    expect_eq(parser.next() == null, true);
}

test("sse_parser: custom event type, persistent lastEventId, NUL id ignore and retry validation") {
    const stream =
        "event: token\n" ++
        "id: evt-100\n" ++
        "retry: 3000\n" ++
        "data: chunk1\n" ++
        "\n" ++
        // id 含 NUL 字节必须忽略，lastEventId 保持 evt-100；非法 retry 忽略
        "id: bad\x00id\n" ++
        "retry: 12ms\n" ++
        "data: chunk2\n" ++
        "\n";

    let parser = SseParser.init(stream);

    const e1 = parser.next().?;
    expect_eq(std.mem.eql(u8, e1.event_type, "token"), true);
    expect_eq(std.mem.eql(u8, e1.id, "evt-100"), true);
    expect_eq(e1.has_id, true);
    expect_eq(e1.retry_ms.?, 3000);
    expect_eq(std.mem.eql(u8, e1.data, "chunk1"), true);
    expect_eq(std.mem.eql(u8, parser.lastEventId(), "evt-100"), true);

    const e2 = parser.next().?;
    expect_eq(std.mem.eql(u8, e2.event_type, "message"), true);
    expect_eq(std.mem.eql(u8, e2.id, "evt-100"), true);
    expect_eq(e2.retry_ms == null, true);
    expect_eq(std.mem.eql(u8, e2.data, "chunk2"), true);
    expect_eq(std.mem.eql(u8, parser.lastEventId(), "evt-100"), true);
}

test("sse_parser: formatSseEvent multiline data split and parse round-trip") {
    const orig = SseEvent{
        event_type: "update",
        data: "row1\nrow2\nrow3",
        id: "42",
        has_id: true,
        retry_ms: 5000,
    };

    let buf: [256]u8 = [0; 256];
    const wire = formatSseEvent(orig, buf[0..]).?;
    const expected =
        "event: update\n" ++
        "id: 42\n" ++
        "retry: 5000\n" ++
        "data: row1\n" ++
        "data: row2\n" ++
        "data: row3\n" ++
        "\n";
    expect_eq(std.mem.eql(u8, wire, expected), true);

    // 往返解析一致
    let parser = SseParser.init(wire);
    const parsed = parser.next().?;
    expect_eq(std.mem.eql(u8, parsed.event_type, "update"), true);
    expect_eq(std.mem.eql(u8, parsed.id, "42"), true);
    expect_eq(parsed.has_id, true);
    expect_eq(parsed.retry_ms.?, 5000);
    expect_eq(std.mem.eql(u8, parsed.data, "row1\nrow2\nrow3"), true);
}

test("sse_parser: incomplete trailing event without blank line and small output buffer guard") {
    // 末尾缺少空行的未完结事件按 WHATWG 规范不派发
    let p = SseParser.init("data: incomplete_no_blank_line\n");
    expect_eq(p.next() == null, true);

    // 仅含注释或仅含 event 的块不派发
    let p2 = SseParser.init("event: orphan\n\n: comment only\n\n");
    expect_eq(p2.next() == null, true);

    // formatSseEvent 过小缓冲区返回 null
    let tiny: [6]u8 = [0; 6];
    expect_eq(formatSseEvent(SseEvent{ data: "hello world" }, tiny[0..]) == null, true);
}
