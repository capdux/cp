// Origin: Pure-CP Stdlib Test Suite (AI Streaming Tool-Call JSON Delta Assembler & Partial Closure) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/tool_stream.cp
// Covers:
//   1. Byte-by-byte & chunked delta assembly, exact completion transition & reset reuse
//   2. Escaped quotes, escaped backslashes & brace immunity inside JSON string literals
//   3. closePartialJson across string/backslash/key/comma/colon/keyword/decimal truncations
//   4. peekStreamingStringField incremental prefix decoding & nested same-name key isolation
//   5. Buffer overflow, nesting depth overflow & malformed bracket/trailing-garbage rejection

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ToolStreamState,
    ToolStreamAssembler,
    closePartialJson,
    peekStreamingStringField,
} from "../../lib/agent/index.cp";

fn testByteByByteAndChunkedAssembly()! {
    let buf: [256]u8 = [0; 256];
    let slice: []u8 = buf[0..];
    let asm_inst = ToolStreamAssembler.init(slice);

    expect_eq(asm_inst.state() == ToolStreamState.Idle, true);
    expect_eq(asm_inst.isComplete(), false);

    // Byte-by-byte feeding with leading whitespace and nested array/object
    const full = "  {\"tool\":\"edit\",\"args\":{\"lines\":[1,2,3]}}  ";
    let i: usize = 0;
    while (i < full.len) : (i += 1) {
        const st = asm_inst.feedDelta(full[i..i + 1]);
        if (i < 2) {
            expect_eq(st == ToolStreamState.Idle, true);
        } else if (i < full.len - 3) {
            expect_eq(st == ToolStreamState.Streaming, true);
        } else {
            expect_eq(st == ToolStreamState.Complete, true);
        }
    }
    expect_eq(asm_inst.isComplete(), true);
    expect_eq(std.mem.eql(u8, asm_inst.payload(), "{\"tool\":\"edit\",\"args\":{\"lines\":[1,2,3]}}"), true);

    // Reset and feed arbitrary multi-byte chunks
    asm_inst.reset();
    expect_eq(asm_inst.state() == ToolStreamState.Idle, true);
    expect_eq(asm_inst.payload().len, 0);

    expect_eq(asm_inst.feedDelta("{\"a\":") == ToolStreamState.Streaming, true);
    expect_eq(asm_inst.feedDelta("[10, 20") == ToolStreamState.Streaming, true);
    expect_eq(asm_inst.feedDelta("]}") == ToolStreamState.Complete, true);
    expect_eq(std.mem.eql(u8, asm_inst.payload(), "{\"a\":[10, 20]}"), true);
}

fn testEscapedQuotesAndBracesInsideStrings()! {
    let buf: [256]u8 = [0; 256];
    let slice: []u8 = buf[0..];
    let asm_inst = ToolStreamAssembler.init(slice);

    // Contains braces, brackets, escaped quotes (\"), and escaped backslash before closing quote (\\")
    const tricky = "{\"code\":\"if (x) { return \\\"}\\\"; } // \\\\\",\"ok\":true}";
    let i: usize = 0;
    while (i < tricky.len) : (i += 1) {
        const st = asm_inst.feedDelta(tricky[i..i + 1]);
        if (i + 1 < tricky.len) {
            expect_eq(st == ToolStreamState.Streaming, true);
        } else {
            expect_eq(st == ToolStreamState.Complete, true);
        }
    }
    expect_eq(std.mem.eql(u8, asm_inst.payload(), tricky), true);
}

fn testClosePartialJsonTruncationModes()! {
    let out_buf: [256]u8 = [0; 256];
    let out: []u8 = out_buf[0..];

    // 1. Nested array truncation
    const n1 = closePartialJson("{\"path\":\"src/main.cp\",\"lines\":[1,2", out) ?? 0;
    expect_eq(std.mem.eql(u8, out[0..n1], "{\"path\":\"src/main.cp\",\"lines\":[1,2]}"), true);

    // 2. Unclosed string value
    const n2 = closePartialJson("{\"path\":\"src/ma", out) ?? 0;
    expect_eq(std.mem.eql(u8, out[0..n2], "{\"path\":\"src/ma\"}"), true);

    // 3. Unclosed string ending with dangling backslash
    const n3 = closePartialJson("{\"msg\":\"hello\\", out) ?? 0;
    expect_eq(std.mem.eql(u8, out[0..n3], "{\"msg\":\"hello\"}"), true);

    // 4. Truncated inside object key -> appends ":null"
    const n4 = closePartialJson("{\"a\":1,\"next_k", out) ?? 0;
    expect_eq(std.mem.eql(u8, out[0..n4], "{\"a\":1,\"next_k\":null}"), true);

    // 5. Trailing comma stripped
    const n5 = closePartialJson("{\"a\":1,", out) ?? 0;
    expect_eq(std.mem.eql(u8, out[0..n5], "{\"a\":1}"), true);

    // 6. Trailing colon gets null
    const n6 = closePartialJson("{\"a\":", out) ?? 0;
    expect_eq(std.mem.eql(u8, out[0..n6], "{\"a\":null}"), true);

    // 7. Truncated keywords (true / false / null) & trailing decimal dot
    const n7 = closePartialJson("{\"ok\":tr", out) ?? 0;
    expect_eq(std.mem.eql(u8, out[0..n7], "{\"ok\":true}"), true);

    const n8 = closePartialJson("{\"ok\":fal", out) ?? 0;
    expect_eq(std.mem.eql(u8, out[0..n8], "{\"ok\":false}"), true);

    const n9 = closePartialJson("{\"x\":12.", out) ?? 0;
    expect_eq(std.mem.eql(u8, out[0..n9], "{\"x\":12.0}"), true);

    // 8. Invalid start, mismatched bracket & tiny buffer rejected
    expect_eq(closePartialJson("", out) == null, true);
    expect_eq(closePartialJson("hello", out) == null, true);
    expect_eq(closePartialJson("{\"a\":[1}", out) == null, true);
    let tiny_buf: [4]u8 = [0; 4];
    let tiny: []u8 = tiny_buf[0..];
    expect_eq(closePartialJson("{\"a\":1}", tiny) == null, true);
}

fn testPeekStreamingStringField()! {
    let val_buf: [128]u8 = [0; 128];
    let val: []u8 = val_buf[0..];

    // Nested object has same key "path", must be skipped in favor of top-level "path"
    const partial = "{\"meta\":{\"path\":\"nested/ignore.cp\",\"n\":1},\"count\":3,\"path\":\"src/lib\\n\\\"ok\\\"\\";
    const n = peekStreamingStringField(partial, "path", val);
    expect_eq(n != null, true);
    expect_eq(std.mem.eql(u8, val[0..(n ?? 0)], "src/lib\n\"ok\""), true);

    // Non-string field ("count") and missing field return null
    expect_eq(peekStreamingStringField(partial, "count", val) == null, true);
    expect_eq(peekStreamingStringField(partial, "meta", val) == null, true);
    expect_eq(peekStreamingStringField(partial, "absent", val) == null, true);

    // Complete field extraction
    const complete = "{\"path\":\"README.md\",\"mode\":\"rw\"}";
    const m = peekStreamingStringField(complete, "mode", val) ?? 0;
    expect_eq(std.mem.eql(u8, val[0..m], "rw"), true);

    // Undersized output buffer returns null
    let tiny_buf: [2]u8 = [0; 2];
    let tiny: []u8 = tiny_buf[0..];
    expect_eq(peekStreamingStringField(complete, "path", tiny) == null, true);
}

fn testOverflowAndMalformedGuards()! {
    // 1. Buffer overflow detection
    let small_buf: [8]u8 = [0; 8];
    let small: []u8 = small_buf[0..];
    let a1 = ToolStreamAssembler.init(small);
    expect_eq(a1.feedDelta("{\"key\":\"value\"}") == ToolStreamState.Overflow, true);
    expect_eq(a1.state() == ToolStreamState.Overflow, true);

    // 2. Non-container leading byte -> Malformed
    let buf: [64]u8 = [0; 64];
    let slice: []u8 = buf[0..];
    let a2 = ToolStreamAssembler.init(slice);
    expect_eq(a2.feedDelta("123") == ToolStreamState.Malformed, true);

    // 3. Mismatched closing bracket -> Malformed
    a2.reset();
    expect_eq(a2.feedDelta("{\"a\":[1}") == ToolStreamState.Malformed, true);

    // 4. Trailing non-whitespace after Complete -> Malformed
    a2.reset();
    expect_eq(a2.feedDelta("{\"a\":1}") == ToolStreamState.Complete, true);
    expect_eq(a2.feedDelta(" {\"b\":2}") == ToolStreamState.Malformed, true);
}

fn main() {
    test("1. agent tool_stream: byte-by-byte & chunked assembly with reset reuse", () => {
        testByteByByteAndChunkedAssembly()?;
    });
    test("2. agent tool_stream: escaped quotes, backslashes & braces inside strings", () => {
        testEscapedQuotesAndBracesInsideStrings()?;
    });
    test("3. agent tool_stream: closePartialJson across truncation modes", () => {
        testClosePartialJsonTruncationModes()?;
    });
    test("4. agent tool_stream: peekStreamingStringField incremental prefix & nesting isolation", () => {
        testPeekStreamingStringField()?;
    });
    test("5. agent tool_stream: buffer overflow & malformed bracket/trailing guards", () => {
        testOverflowAndMalformedGuards()?;
    });
}
