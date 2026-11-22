// Origin: Pure-CP Stdlib Test Suite (lib/agent/llm_stream.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Unified LLM Streaming Delta Parser:
//   1. OpenAI SSE: text delta, reasoning_content, tool_calls start/args delta, finish_reason, usage & [DONE]
//   2. Anthropic SSE: message_start, thinking_delta, text_delta, tool_use, input_json_delta, message_delta, message_stop
//   3. JSON string escape decoding (\", \n, \r, \t, \\, \/, \uXXXX ASCII & UTF-8)
//   4. LlmDeltaParser state machine, LlmStreamStats accumulation, reset() & SseStreamParser end-to-end pipeline
//   5. Malformed JSON, unclosed strings, invalid escapes, bracket mismatch & deep nesting defense
//   6. Small & zero-length caller buffer boundary defense (safe truncation, zero OOB)

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    LlmProviderWire,
    LlmDeltaKind,
    LlmDeltaEvent,
    LlmStreamStats,
    parseOpenAiSseData,
    parseAnthropicSseData,
    LlmDeltaParser,
    SseEvent,
    SseStreamParser,
} from "cap:agent";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: OpenAI Chat Completions SSE Full Lifecycle
// ============================================================================

fn testOpenAiFullStream()! {
    let scratch: [512]u8 = [0; 512];
    let buf: []u8 = scratch[0..];

    // 1a. Initial role-only chunk -> Ignore
    const ev0: LlmDeltaEvent = cap.agent.parseOpenAiSseData(
        "{\"id\":\"chatcmpl-1\",\"choices\":[{\"index\":0,\"delta\":{\"role\":\"assistant\"},\"finish_reason\":null}]}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev0.kind == LlmDeltaKind.Ignore, true);
    expect_eq(ev0.truncated, false);

    // 1b. Reasoning delta (o-series / DeepSeek / Qwen)
    const ev1: LlmDeltaEvent = parseOpenAiSseData(
        "data: {\"choices\":[{\"index\":0,\"delta\":{\"reasoning_content\":\"Step 1: check weather\"}}]}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev1.kind == LlmDeltaKind.ReasoningDelta, true);
    expect_eq(strEq(ev1.text, "Step 1: check weather"), true);

    // 1c. Text delta
    const ev2: LlmDeltaEvent = parseOpenAiSseData(
        "{\"choices\":[{\"index\":0,\"delta\":{\"content\":\"Checking Tokyo weather...\"}}]}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev2.kind == LlmDeltaKind.TextDelta, true);
    expect_eq(strEq(ev2.text, "Checking Tokyo weather..."), true);

    // 1d. Tool call start (with id + function.name + initial arguments prefix)
    const ev3: LlmDeltaEvent = parseOpenAiSseData(
        "{\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":1,\"id\":\"call_abc123\",\"type\":\"function\",\"function\":{\"name\":\"get_weather\",\"arguments\":\"{\\\"city\\\":\"}}]}}]}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev3.kind == LlmDeltaKind.ToolCallStart, true);
    expect_eq(ev3.tool_index, 1);
    expect_eq(strEq(ev3.tool_call_id, "call_abc123"), true);
    expect_eq(strEq(ev3.tool_name, "get_weather"), true);
    expect_eq(strEq(ev3.text, "{\"city\":"), true);

    // 1e. Tool call arguments delta
    const ev4: LlmDeltaEvent = parseOpenAiSseData(
        "{\"choices\":[{\"index\":0,\"delta\":{\"tool_calls\":[{\"index\":1,\"function\":{\"arguments\":\"\\\"Tokyo\\\"}\"}}]}}]}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev4.kind == LlmDeltaKind.ToolCallArgsDelta, true);
    expect_eq(ev4.tool_index, 1);
    expect_eq(strEq(ev4.text, "\"Tokyo\"}"), true);

    // 1f. finish_reason chunk
    const ev5: LlmDeltaEvent = parseOpenAiSseData(
        "{\"choices\":[{\"index\":0,\"delta\":{},\"finish_reason\":\"tool_calls\"}]}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev5.kind == LlmDeltaKind.Done, true);
    expect_eq(strEq(ev5.finish_reason, "tool_calls"), true);

    // 1g. Usage-only chunk (stream_options.include_usage = true)
    const ev6: LlmDeltaEvent = parseOpenAiSseData(
        "{\"choices\":[],\"usage\":{\"prompt_tokens\":128,\"completion_tokens\":64,\"total_tokens\":192}}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev6.kind == LlmDeltaKind.Usage, true);
    expect_eq(ev6.input_tokens, 128);
    expect_eq(ev6.output_tokens, 64);

    // 1h. Terminal [DONE] sentinel
    const ev7: LlmDeltaEvent = parseOpenAiSseData("data: [DONE]", buf) ?? return error.ParseFailed;
    expect_eq(ev7.kind == LlmDeltaKind.Done, true);
}

// ============================================================================
// Test 2: Anthropic Messages SSE Full Lifecycle
// ============================================================================

fn testAnthropicFullStream()! {
    let scratch: [512]u8 = [0; 512];
    let buf: []u8 = scratch[0..];

    // 2a. message_start with input_tokens
    const ev0: LlmDeltaEvent = parseAnthropicSseData(
        "message_start",
        "{\"type\":\"message_start\",\"message\":{\"id\":\"msg_01\",\"usage\":{\"input_tokens\":250,\"output_tokens\":1}}}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev0.kind == LlmDeltaKind.Usage, true);
    expect_eq(ev0.input_tokens, 250);
    expect_eq(ev0.output_tokens, 1);

    // 2b. ping -> Ignore
    const ev1: LlmDeltaEvent = parseAnthropicSseData(
        "ping",
        "{\"type\":\"ping\"}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev1.kind == LlmDeltaKind.Ignore, true);

    // 2c. content_block_delta: thinking_delta (empty event_type -> fallback to JSON "type")
    const ev2: LlmDeltaEvent = parseAnthropicSseData(
        "",
        "{\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"thinking_delta\",\"thinking\":\"Analyzing request...\"}}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev2.kind == LlmDeltaKind.ReasoningDelta, true);
    expect_eq(ev2.tool_index, 0);
    expect_eq(strEq(ev2.text, "Analyzing request..."), true);

    // 2d. content_block_delta: text_delta
    const ev3: LlmDeltaEvent = parseAnthropicSseData(
        "content_block_delta",
        "{\"type\":\"content_block_delta\",\"index\":1,\"delta\":{\"type\":\"text_delta\",\"text\":\"Let me run a search.\"}}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev3.kind == LlmDeltaKind.TextDelta, true);
    expect_eq(strEq(ev3.text, "Let me run a search."), true);

    // 2e. content_block_start: tool_use
    const ev4: LlmDeltaEvent = parseAnthropicSseData(
        "content_block_start",
        "{\"type\":\"content_block_start\",\"index\":2,\"content_block\":{\"type\":\"tool_use\",\"id\":\"toolu_01XYZ\",\"name\":\"search_code\",\"input\":{}}}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev4.kind == LlmDeltaKind.ToolCallStart, true);
    expect_eq(ev4.tool_index, 2);
    expect_eq(strEq(ev4.tool_call_id, "toolu_01XYZ"), true);
    expect_eq(strEq(ev4.tool_name, "search_code"), true);

    // 2f. content_block_delta: input_json_delta
    const ev5: LlmDeltaEvent = parseAnthropicSseData(
        "content_block_delta",
        "{\"type\":\"content_block_delta\",\"index\":2,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"{\\\"query\\\":\\\"cap:agent\\\"}\"}}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev5.kind == LlmDeltaKind.ToolCallArgsDelta, true);
    expect_eq(ev5.tool_index, 2);
    expect_eq(strEq(ev5.text, "{\"query\":\"cap:agent\"}"), true);

    // 2g. message_delta with stop_reason & output_tokens
    const ev6: LlmDeltaEvent = parseAnthropicSseData(
        "message_delta",
        "{\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"tool_use\",\"stop_sequence\":null},\"usage\":{\"output_tokens\":89}}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev6.kind == LlmDeltaKind.Usage, true);
    expect_eq(strEq(ev6.finish_reason, "tool_use"), true);
    expect_eq(ev6.output_tokens, 89);

    // 2h. message_stop -> Done
    const ev7: LlmDeltaEvent = parseAnthropicSseData(
        "message_stop",
        "{\"type\":\"message_stop\"}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev7.kind == LlmDeltaKind.Done, true);
}

// ============================================================================
// Test 3: JSON Escape Decoding (\", \n, \r, \t, \\, \/, \uXXXX)
// ============================================================================

fn testJsonEscapeDecoding()! {
    let scratch: [256]u8 = [0; 256];
    let buf: []u8 = scratch[0..];

    const ev: LlmDeltaEvent = parseOpenAiSseData(
        "{\"choices\":[{\"delta\":{\"content\":\"Line1\\n\\t\\\"quoted\\\"\\\\slash\\/\\u0041\\u4E2D\"}}]}",
        buf,
    ) ?? return error.ParseFailed;
    expect_eq(ev.kind == LlmDeltaKind.TextDelta, true);
    expect_eq(ev.truncated, false);
    // Expected: Line1\n\t"quoted"\slash/A中 (中 = 0xE4 0xB8 0xAD)
    expect_eq(strEq(ev.text, "Line1\n\t\"quoted\"\\slash/A中"), true);
}

// ============================================================================
// Test 4: LlmDeltaParser Stats Accumulation, Reset & SseStreamParser Pipeline
// ============================================================================

fn testParserStatsAndSsePipeline()! {
    // Combine SseStreamParser -> LlmDeltaParser on an Anthropic multi-event SSE stream
    let line_b: [512]u8 = [0; 512];
    let data_b: [512]u8 = [0; 512];
    let ev_b: [64]u8 = [0; 64];
    let id_b: [64]u8 = [0; 64];
    let line_s: []u8 = line_b[0..];
    let data_s: []u8 = data_b[0..];
    let ev_s: []u8 = ev_b[0..];
    let id_s: []u8 = id_b[0..];
    let sse = SseStreamParser.init(*line_s, *data_s, *ev_s, *id_s);

    let delta_parser = LlmDeltaParser.init(LlmProviderWire.Anthropic);
    let scratch: [256]u8 = [0; 256];
    let out_buf: []u8 = scratch[0..];

    const wire_stream: string =
        "event: message_start\ndata: {\"type\":\"message_start\",\"message\":{\"usage\":{\"input_tokens\":100}}}\n\n" ++
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"thinking_delta\",\"thinking\":\"hmm\"}}\n\n" ++
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":1,\"delta\":{\"type\":\"text_delta\",\"text\":\"Hi!\"}}\n\n" ++
        "event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":2,\"content_block\":{\"type\":\"tool_use\",\"id\":\"t1\",\"name\":\"calc\"}}\n\n" ++
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":2,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"{\\\"x\\\":1}\"}}\n\n" ++
        "event: message_delta\ndata: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"end_turn\"},\"usage\":{\"output_tokens\":45}}\n\n" ++
        "event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n";

    let rest: []const u8 = wire_stream;
    let sse_ev = SseEvent{ event_type: "", data: "", id: "", is_done: false };
    while (rest.len > 0) {
        const res = sse.nextEvent(rest, *sse_ev);
        expect_eq(res.overflow, false);
        rest = rest[res.consumed..];
        if (res.has_event) {
            const parsed = delta_parser.parseEvent(sse_ev.event_type, sse_ev.data, out_buf);
            expect_eq(parsed != null, true);
        }
    }

    const st: LlmStreamStats = delta_parser.stats();
    expect_eq(delta_parser.isDone(), true);
    expect_eq(st.events_parsed, 7);
    expect_eq(st.reasoning_deltas, 1);
    expect_eq(st.reasoning_bytes, 3);
    expect_eq(st.text_deltas, 1);
    expect_eq(st.text_bytes, 3);
    expect_eq(st.tool_calls_started, 1);
    expect_eq(st.tool_args_deltas, 1);
    expect_eq(st.tool_args_bytes, 7);
    expect_eq(st.input_tokens, 100);
    expect_eq(st.output_tokens, 45);
    expect_eq(st.parse_errors, 0);
    expect_eq(st.truncated_events, 0);

    // Reset clears all counters cleanly
    delta_parser.reset();
    const st2 = delta_parser.stats();
    expect_eq(delta_parser.isDone(), false);
    expect_eq(st2.events_parsed, 0);
    expect_eq(st2.input_tokens, 0);
    expect_eq(st2.output_tokens, 0);
}

// ============================================================================
// Test 5: Malformed JSON & Invalid Escape Boundary Defense
// ============================================================================

fn testMalformedJsonDefense()! {
    let scratch: [128]u8 = [0; 128];
    let buf: []u8 = scratch[0..];
    let parser = LlmDeltaParser.init(LlmProviderWire.OpenAi);

    // 5a. Truncated JSON object
    expect_eq(parser.parseData("{\"choices\":[{\"delta\":{\"content\":\"hi\"}", buf) == null, true);
    // 5b. Unclosed string literal
    expect_eq(parser.parseData("{\"choices\":[{\"delta\":{\"content\":\"unterminated}}]}", buf) == null, true);
    // 5c. Invalid escape sequence (\q)
    expect_eq(parser.parseData("{\"choices\":[{\"delta\":{\"content\":\"bad\\qescape\"}}]}", buf) == null, true);
    // 5d. Invalid unicode escape (\u12ZZ)
    expect_eq(parser.parseData("{\"choices\":[{\"delta\":{\"content\":\"\\u12ZZ\"}}]}", buf) == null, true);
    // 5e. Non-JSON garbage
    expect_eq(parser.parseData("not-json-at-all", buf) == null, true);
    // 5f. Mismatched brackets
    expect_eq(parser.parseData("{\"choices\":]}", buf) == null, true);
    // 5g. Malformed Anthropic payload
    expect_eq(parseAnthropicSseData("content_block_delta", "{\"type\":\"content_block_delta\",", buf) == null, true);

    const st = parser.stats();
    expect_eq(st.parse_errors, 6);
    expect_eq(st.events_parsed, 0);
}

// ============================================================================
// Test 6: Small & Zero-Length Buffer Boundary Defense
// ============================================================================

fn testSmallBufferBoundaryDefense()! {
    let parser = LlmDeltaParser.init(LlmProviderWire.OpenAi);

    // 6a. Small 5-byte buffer: safely truncates without OOB and sets truncated=true
    let tiny: [5]u8 = [0; 5];
    let tiny_s: []u8 = tiny[0..];
    const ev1: LlmDeltaEvent = parser.parseData(
        "{\"choices\":[{\"delta\":{\"content\":\"HelloWorld12345\"}}]}",
        tiny_s,
    ) ?? return error.ParseFailed;
    expect_eq(ev1.kind == LlmDeltaKind.TextDelta, true);
    expect_eq(ev1.truncated, true);
    expect_eq(strEq(ev1.text, "Hello"), true);

    // 6b. Zero-length buffer: safely returns empty text with truncated=true
    let empty_arr: [0]u8 = [];
    let empty_s: []u8 = empty_arr[0..];
    const ev2: LlmDeltaEvent = parser.parseData(
        "{\"choices\":[{\"delta\":{\"content\":\"ABC\"}}]}",
        empty_s,
    ) ?? return error.ParseFailed;
    expect_eq(ev2.kind == LlmDeltaKind.TextDelta, true);
    expect_eq(ev2.truncated, true);
    expect_eq(ev2.text.len, 0);

    // 6c. Anthropic small buffer with multi-byte UTF-8 \u4E2D (3 bytes) when only 2 bytes remain
    let small_utf8: [3]u8 = [0; 3];
    let su_s: []u8 = small_utf8[0..];
    const ev3: LlmDeltaEvent = parseAnthropicSseData(
        "content_block_delta",
        "{\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"A\\u4E2D\"}}",
        su_s,
    ) ?? return error.ParseFailed;
    expect_eq(ev3.kind == LlmDeltaKind.TextDelta, true);
    expect_eq(ev3.truncated, true);
    // 'A' (1 byte) fits; 3-byte UTF-8 char won't fit in remaining 2 bytes -> safely omitted
    expect_eq(strEq(ev3.text, "A"), true);

    const st = parser.stats();
    expect_eq(st.truncated_events, 2);
    expect_eq(st.text_deltas, 2);
}

test("agent.llm_stream: OpenAI SSE full lifecycle (reasoning, text, tools, usage, DONE)", testOpenAiFullStream);
test("agent.llm_stream: Anthropic SSE full lifecycle (message_start, thinking, text, tool_use, input_json, stop)", testAnthropicFullStream);
test("agent.llm_stream: JSON string escape decoding (quotes, newlines, tabs, slashes, unicode)", testJsonEscapeDecoding);
test("agent.llm_stream: LlmDeltaParser stats accumulation, reset & SseStreamParser end-to-end pipeline", testParserStatsAndSsePipeline);
test("agent.llm_stream: malformed JSON, invalid escapes & bracket mismatch defense", testMalformedJsonDefense);
test("agent.llm_stream: small & zero-length caller buffer truncation defense", testSmallBufferBoundaryDefense);
