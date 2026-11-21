import std from "std";
import { test, expect_eq } from "cap:test";
import {
    JsonlIter,
    scanCompleteJsonlChunk,
    isBalancedJsonRecord,
    appendJsonlRecord,
} from "cap:jsonl";

test("1. JsonlIter skips UTF-8 BOM, blank lines, handles LF and CRLF, and preserves line content", () => {
    const input = "\xEF\xBB\xBF{\"a\":1}\r\n\r\n   \t  \n  {\"b\":\"hi\"}  \n[1,2,3]";
    let it = JsonlIter.init(input);

    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "{\"a\":1}"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "  {\"b\":\"hi\"}  "), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "[1,2,3]"), true);
    expect_eq(it.next() == null, true);

    // Blank-only input yields nothing
    let empty_it = JsonlIter.init("\xEF\xBB\xBF\n  \r\n\t\n");
    expect_eq(empty_it.next() == null, true);
});

test("2. scanCompleteJsonlChunk tracks consumed_bytes, line_count, and incomplete tail", () => {
    const chunk1 = "{\"id\":1}\n\n{\"id\":2}\r\n{\"id\":3";
    const r1 = scanCompleteJsonlChunk(chunk1);
    expect_eq(r1.line_count, usize(2));
    expect_eq(r1.has_incomplete_tail, true);
    expect_eq(std.mem.eql(u8, chunk1[r1.consumed_bytes..chunk1.len], "{\"id\":3"), true);

    // Complete chunk ending with LF has no incomplete tail
    const chunk2 = "{\"ok\":true}\n[1,2]\n";
    const r2 = scanCompleteJsonlChunk(chunk2);
    expect_eq(r2.line_count, usize(2));
    expect_eq(r2.consumed_bytes, chunk2.len);
    expect_eq(r2.has_incomplete_tail, false);

    // Chunk without any newline consumes 0 bytes
    const chunk3 = "{\"partial\":true";
    const r3 = scanCompleteJsonlChunk(chunk3);
    expect_eq(r3.line_count, usize(0));
    expect_eq(r3.consumed_bytes, usize(0));
    expect_eq(r3.has_incomplete_tail, true);

    // Empty chunk
    const r4 = scanCompleteJsonlChunk("");
    expect_eq(r4.line_count, usize(0));
    expect_eq(r4.consumed_bytes, usize(0));
    expect_eq(r4.has_incomplete_tail, false);
});

test("3. isBalancedJsonRecord accepts balanced objects, arrays, strings, escapes, and primitives", () => {
    expect_eq(isBalancedJsonRecord("{\"msg\":\"hello \\\"world\\\" { [ ] }\"}"), true);
    expect_eq(isBalancedJsonRecord("[1, {\"k\":[true, false, null]}, \"a\\\\b\"]"), true);
    expect_eq(isBalancedJsonRecord("\"simple string\""), true);
    expect_eq(isBalancedJsonRecord("42"), true);
    expect_eq(isBalancedJsonRecord("-3.14"), true);
    expect_eq(isBalancedJsonRecord("true"), true);
    expect_eq(isBalancedJsonRecord("null"), true);
});

test("4. isBalancedJsonRecord rejects truncated, mismatched, unclosed, concatenated, and blank lines", () => {
    expect_eq(isBalancedJsonRecord(""), false);
    expect_eq(isBalancedJsonRecord("   \t  "), false);
    expect_eq(isBalancedJsonRecord("{\"a\":1"), false);
    expect_eq(isBalancedJsonRecord("[1, 2"), false);
    expect_eq(isBalancedJsonRecord("{\"a\":[1,2}]"), false);
    expect_eq(isBalancedJsonRecord("{\"s\":\"unterminated"), false);
    expect_eq(isBalancedJsonRecord("{\"s\":\"bad\\\"}"), false);
    expect_eq(isBalancedJsonRecord("{\"a\":1}{\"b\":2}"), false);
    expect_eq(isBalancedJsonRecord("{\"a\":1}\n{\"b\":2}"), false);
    expect_eq(isBalancedJsonRecord("not-json"), false);
});

test("5. appendJsonlRecord frames valid records with LF and guards against embedded newlines or overflow", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    let pos: usize = 0;

    expect_eq(appendJsonlRecord("{\"a\":1}", &buf, &pos), true);
    expect_eq(appendJsonlRecord("[2,3]", &buf, &pos), true);
    expect_eq(std.mem.eql(u8, buf[0..pos], "{\"a\":1}\n[2,3]\n"), true);

    const saved_pos = pos;
    // Reject empty or embedded CR/LF without advancing pos
    expect_eq(appendJsonlRecord("", &buf, &pos), false);
    expect_eq(appendJsonlRecord("{\"a\":1}\n{\"b\":2}", &buf, &pos), false);
    expect_eq(appendJsonlRecord("{\"a\":1}\r", &buf, &pos), false);
    expect_eq(pos, saved_pos);

    // Reject buffer overflow without advancing pos
    let tiny: [5]u8 = std.mem.zeroes([5]u8);
    let tiny_pos: usize = 0;
    expect_eq(appendJsonlRecord("12345", &tiny, &tiny_pos), false);
    expect_eq(tiny_pos, usize(0));
    expect_eq(appendJsonlRecord("1234", &tiny, &tiny_pos), true);
    expect_eq(std.mem.eql(u8, tiny[0..tiny_pos], "1234\n"), true);
});
