// Origin: Pure-CP Stdlib Test Suite (lib/prompt/toolCallParser.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Streaming LLM Tool-Call Extractor & State Machine:
//   1. <tool_call>...</tool_call> extraction with surrounding natural language text & optional id
//   2. 1-byte extreme chunking across all 3 wrappers (xml_tag + ```json fence + bare JSON) concurrent calls
//   3. Brace/string/escape depth FSM: braces inside quotes, escaped quotes, fake </tool_call> & UTF-8
//   4. Non-tool text preservation (normal <xml>, {braces}, ```python code block remain in textSlice)
//   5. Truncated stream recovery & malformed <tool_call> payload reporting
//   6. Overflow guards (excess text dropped cleanly, >16 concurrent calls capped with dropped_calls)
//   7. reset() reuse & cap.prompt.ToolCallStreamParser facade verification

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    TOOL_CALL_MAX_CALLS,
    ToolCallStatus,
    ToolCallWrapperKind,
    ToolCallStreamParser,
} from "cap:prompt";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: <tool_call> Extraction & Text/Tool Separation
// ============================================================================
test("toolCallParser: extracts <tool_call> with id/name/arguments and separates surrounding text", () => {
    let parser = ToolCallStreamParser.init();
    parser.feed("Let me check. <tool_call>{\"id\":\"call_01\",\"name\":\"get_weather\",\"arguments\":{\"city\":\"Sydney\",\"unit\":\"C\"}}</tool_call> Thanks!");
    const st = parser.finish();

    expect_eq(st.total_calls, 1);
    expect_eq(st.complete_calls, 1);
    expect_true(bytesEq(parser.textSlice(), "Let me check.  Thanks!"));

    const calls = parser.toolCalls();
    expect_eq(calls.len, 1);
    expect_eq(calls[0].status, ToolCallStatus.complete);
    expect_eq(calls[0].wrapper, ToolCallWrapperKind.xml_tag);
    expect_true(bytesEq(calls[0].idSlice(), "call_01"));
    expect_true(bytesEq(calls[0].nameSlice(), "get_weather"));
    expect_true(bytesEq(calls[0].argsJsonSlice(), "{\"city\":\"Sydney\",\"unit\":\"C\"}"));
});

// ============================================================================
// Test 2: 1-Byte Extreme Chunking Across XML, Markdown ```json & Bare JSON
// ============================================================================
test("toolCallParser: 1-byte streaming extracts concurrent xml, code_fence & bare_json calls", () => {
    const stream: []const u8 =
        "Step1: <tool_call>{\"name\":\"read_file\",\"arguments\":{\"path\":\"a.cp\"}}</tool_call>\n" ++
        "Step2:\n```json\n{\"id\":\"f2\",\"name\":\"run_cmd\",\"arguments\":{\"cmd\":\"cap test\"}}\n```\n" ++
        "Step3: {\"name\":\"sum\",\"arguments\":{\"nums\":[1,2,3]}} End.";

    let parser = ToolCallStreamParser.init();
    let i: usize = 0;
    while (i < stream.len) : (i += 1) {
        parser.feed(stream[i .. i + 1]);
    }
    const st = parser.finish();

    expect_eq(st.total_calls, 3);
    expect_eq(st.complete_calls, 3);
    expect_eq(st.bytes_fed, stream.len as u64);

    const calls = parser.toolCalls();
    expect_eq(calls[0].wrapper, ToolCallWrapperKind.xml_tag);
    expect_true(bytesEq(calls[0].nameSlice(), "read_file"));
    expect_true(bytesEq(calls[0].argsJsonSlice(), "{\"path\":\"a.cp\"}"));

    expect_eq(calls[1].wrapper, ToolCallWrapperKind.code_fence);
    expect_true(bytesEq(calls[1].idSlice(), "f2"));
    expect_true(bytesEq(calls[1].nameSlice(), "run_cmd"));
    expect_true(bytesEq(calls[1].argsJsonSlice(), "{\"cmd\":\"cap test\"}"));

    expect_eq(calls[2].wrapper, ToolCallWrapperKind.bare_json);
    expect_true(bytesEq(calls[2].nameSlice(), "sum"));
    expect_true(bytesEq(calls[2].argsJsonSlice(), "{\"nums\":[1,2,3]}"));

    expect_true(bytesEq(parser.textSlice(), "Step1: \nStep2:\n\nStep3:  End."));
});

// ============================================================================
// Test 3: Braces Inside Quotes, Escaped Quotes, Fake Close Tag & UTF-8
// ============================================================================
test("toolCallParser: handles nested braces in strings, escaped quotes, fake tags & UTF-8", () => {
    const payload: []const u8 =
        "<tool_call>{\"name\":\"format_tpl\",\"arguments\":{\"tpl\":\"Hello {北京} \\\"quoted\\\" </tool_call> }\",\"nested\":{\"k\":1}}}</tool_call>";

    let parser = ToolCallStreamParser.init();
    // Feed in awkward 7-byte chunks
    let off: usize = 0;
    while (off < payload.len) {
        const end: usize = if (off + 7 < payload.len) (off + 7) else payload.len;
        parser.feed(payload[off..end]);
        off = end;
    }
    const st = parser.finish();
    expect_eq(st.complete_calls, 1);
    const c = parser.toolCalls()[0];
    expect_eq(c.status, ToolCallStatus.complete);
    expect_true(bytesEq(c.nameSlice(), "format_tpl"));
    expect_true(bytesEq(c.argsJsonSlice(), "{\"tpl\":\"Hello {北京} \\\"quoted\\\" </tool_call> }\",\"nested\":{\"k\":1}}"));
});

// ============================================================================
// Test 4: Non-Tool Natural Language, <xml>, {braces} & Non-JSON Code Fence
// ============================================================================
test("toolCallParser: preserves non-tool XML, bare braces & python code fence as normal text", () => {
    const raw: []const u8 = "Use <tool_hint>x</tool_hint> and {foo: bar} or:\n```python\nprint(1)\n```\n";
    let parser = ToolCallStreamParser.init();
    parser.feed(raw);
    const st = parser.finish();

    expect_eq(st.total_calls, 0);
    expect_true(bytesEq(parser.textSlice(), raw));
});

// ============================================================================
// Test 5: Truncated Stream Recovery & Malformed <tool_call> Reporting
// ============================================================================
test("toolCallParser: recovers truncated tool calls and flags malformed <tool_call> blocks", () => {
    // 5A. Truncated mid-arguments when stream ends
    let p1 = ToolCallStreamParser.init();
    p1.feed("Prefix <tool_call>{\"name\":\"search_web\",\"arguments\":{\"query\":\"cp lang");
    const st1 = p1.finish();
    expect_eq(st1.total_calls, 1);
    expect_eq(st1.truncated_calls, 1);
    expect_eq(p1.toolCalls()[0].status, ToolCallStatus.truncated);
    expect_true(bytesEq(p1.toolCalls()[0].nameSlice(), "search_web"));
    expect_true(bytesEq(p1.textSlice(), "Prefix "));

    // 5B. Malformed content inside explicit <tool_call>...</tool_call>
    let p2 = ToolCallStreamParser.init();
    p2.feed("<tool_call>not valid json</tool_call>OK");
    const st2 = p2.finish();
    expect_eq(st2.total_calls, 1);
    expect_eq(st2.malformed_calls, 1);
    expect_eq(p2.toolCalls()[0].status, ToolCallStatus.malformed);
    expect_true(bytesEq(p2.textSlice(), "OK"));
});

// ============================================================================
// Test 6: Overflow Guards (>16 Concurrent Calls & Excess Text)
// ============================================================================
test("toolCallParser: caps concurrent calls at 16 with dropped_calls & guards excess text", () => {
    let parser = ToolCallStreamParser.init();
    let i: usize = 0;
    while (i < 18) : (i += 1) {
        parser.feed("{\"name\":\"ping\",\"arguments\":{\"n\":1}}");
    }
    // Also feed > 1024 bytes of plain text
    const chunk64: [64]u8 = ['x'; 64];
    let t: usize = 0;
    while (t < 20) : (t += 1) {
        parser.feed(chunk64[0..]);
    }
    const st = parser.finish();

    expect_eq(st.total_calls, TOOL_CALL_MAX_CALLS);
    expect_eq(st.complete_calls, 18);
    expect_eq(st.dropped_calls, 2);
    expect_eq(st.text_bytes_emitted, 1024);
    expect_eq(st.text_bytes_dropped, 256);
});

// ============================================================================
// Test 7: reset() Reuse & cap.prompt Facade Verification
// ============================================================================
test("toolCallParser: reset() clears state cleanly & cap.prompt facade works", () => {
    let parser = cap.prompt.ToolCallStreamParser.init();
    parser.feed("<tool_call>{\"name\":\"first\"}</tool_call>");
    * = parser.finish();
    expect_eq(parser.toolCalls().len, 1);
    expect_true(bytesEq(parser.toolCalls()[0].argsJsonSlice(), "{}"));

    parser.reset();
    expect_eq(parser.toolCalls().len, 0);
    expect_eq(parser.textSlice().len, 0);

    parser.feed("Hi {\"name\":\"second\",\"parameters\":{\"x\":42}}");
    const st = parser.finish();
    expect_eq(st.complete_calls, 1);
    expect_true(bytesEq(parser.toolCalls()[0].nameSlice(), "second"));
    expect_true(bytesEq(parser.toolCalls()[0].argsJsonSlice(), "{\"x\":42}"));
    expect_true(bytesEq(parser.textSlice(), "Hi "));
});
