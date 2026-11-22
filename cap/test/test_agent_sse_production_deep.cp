// Source: cp/cap/test/test_agent_sse_production_deep.cp (Production deep tests for cap:agent SSE & telemetry)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SseEvent,
    SseFeedResult,
    SseStreamParser,
    StreamMetrics,
    StreamTelemetry,
    extractJsonStringField,
} from "cap:agent";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

test("1. OpenAI SSE multi-event stream with [DONE] fed 1 byte at a time", () => {
    let line_b: [256]u8 = std.mem.zeroes([256]u8);
    let data_b: [256]u8 = std.mem.zeroes([256]u8);
    let ev_b: [64]u8 = std.mem.zeroes([64]u8);
    let id_b: [64]u8 = std.mem.zeroes([64]u8);
    let line_s: []u8 = line_b[0..];
    let data_s: []u8 = data_b[0..];
    let ev_s: []u8 = ev_b[0..];
    let id_s: []u8 = id_b[0..];

    let parser = SseStreamParser.init(&line_s, &data_s, &ev_s, &id_s);
    const stream: string =
        "data: {\"delta\":{\"content\":\"Hel\"}}\n\n" ++
        "data: {\"delta\":{\"content\":\"lo!\"}}\n\n" ++
        "data: [DONE]\n\n";

    let ev = SseEvent { event_type: "", data: "", id: "", is_done: false };
    let out_b: [64]u8 = std.mem.zeroes([64]u8);
    let out_s: []u8 = out_b[0..];
    let event_count: usize = 0;
    let saw_done: bool = false;

    // Feed 1 byte per call to stress arbitrary chunk splitting
    let idx: usize = 0;
    while (idx < stream.len) {
        const one: []const u8 = stream[idx..(idx + 1)];
        const res = parser.nextEvent(one, &ev);
        expect_eq(res.overflow, false);
        expect_eq(res.consumed, 1);
        if (res.has_event) {
            event_count += 1;
            expect_eq(strEq(ev.event_type, "message"), true);
            if (event_count == 1) {
                expect_eq(ev.is_done, false);
                const n = extractJsonStringField(ev.data, "content", &out_s) ?? unreachable;
                expect_eq(strEq(out_s[0..n], "Hel"), true);
            } else if (event_count == 2) {
                expect_eq(ev.is_done, false);
                const n = extractJsonStringField(ev.data, "content", &out_s) ?? unreachable;
                expect_eq(strEq(out_s[0..n], "lo!"), true);
            } else if (event_count == 3) {
                expect_eq(ev.is_done, true);
                expect_eq(strEq(ev.data, "[DONE]"), true);
                saw_done = true;
            }
        }
        idx += 1;
    }

    expect_eq(event_count, 3);
    expect_eq(saw_done, true);
});

test("2. Anthropic event type, multi-line data concatenation, id persistence, and :keepalive skipping", () => {
    let line_b: [256]u8 = std.mem.zeroes([256]u8);
    let data_b: [256]u8 = std.mem.zeroes([256]u8);
    let ev_b: [64]u8 = std.mem.zeroes([64]u8);
    let id_b: [64]u8 = std.mem.zeroes([64]u8);
    let line_s: []u8 = line_b[0..];
    let data_s: []u8 = data_b[0..];
    let ev_s: []u8 = ev_b[0..];
    let id_s: []u8 = id_b[0..];

    let parser = SseStreamParser.init(&line_s, &data_s, &ev_s, &id_s);
    const payload: string =
        ":keepalive ping\n" ++
        "\n" ++
        "id: evt-42\n" ++
        "event: content_block_delta\n" ++
        "data: line1\n" ++
        "data:  line2_leading_space\n" ++
        ":comment mid-event\n" ++
        "data:line3_no_space\n" ++
        "\n" ++
        "data: next_event\n\n";

    let rest: []const u8 = payload;
    let ev = SseEvent { event_type: "", data: "", id: "", is_done: false };

    // Event 1: Anthropic event with 3 joined data lines
    const r1 = parser.nextEvent(rest, &ev);
    expect_eq(r1.overflow, false);
    expect_eq(r1.has_event, true);
    expect_eq(strEq(ev.event_type, "content_block_delta"), true);
    expect_eq(strEq(ev.id, "evt-42"), true);
    expect_eq(strEq(ev.data, "line1\n line2_leading_space\nline3_no_space"), true);
    expect_eq(ev.is_done, false);

    rest = rest[r1.consumed..];

    // Event 2: event_type resets to "message", id persists as "evt-42"
    const r2 = parser.nextEvent(rest, &ev);
    expect_eq(r2.overflow, false);
    expect_eq(r2.has_event, true);
    expect_eq(strEq(ev.event_type, "message"), true);
    expect_eq(strEq(ev.id, "evt-42"), true);
    expect_eq(strEq(ev.data, "next_event"), true);
});

test("3. UTF-8 BOM stripping and mixed CRLF/LF/CR split across chunk boundaries", () => {
    let line_b: [128]u8 = std.mem.zeroes([128]u8);
    let data_b: [128]u8 = std.mem.zeroes([128]u8);
    let ev_b: [32]u8 = std.mem.zeroes([32]u8);
    let id_b: [32]u8 = std.mem.zeroes([32]u8);
    let line_s: []u8 = line_b[0..];
    let data_s: []u8 = data_b[0..];
    let ev_s: []u8 = ev_b[0..];
    let id_s: []u8 = id_b[0..];

    let parser = SseStreamParser.init(&line_s, &data_s, &ev_s, &id_s);
    let ev = SseEvent { event_type: "", data: "", id: "", is_done: false };

    // Feed UTF-8 BOM split across two chunks: [0xEF, 0xBB] then [0xBF, "data: first\r"] then ["\n\r\ndata: second\r\r"]
    const bom1: [2]u8 = [0xEF, 0xBB];
    const r_bom1 = parser.nextEvent(bom1[0..], &ev);
    expect_eq(r_bom1.has_event, false);
    expect_eq(r_bom1.overflow, false);

    const bom2: [1]u8 = [0xBF];
    const r_bom2 = parser.nextEvent(bom2[0..], &ev);
    expect_eq(r_bom2.has_event, false);

    // Split CRLF across chunk boundary: chunk ends with '\r', next starts with '\n'
    const c1: string = "data: first\r";
    const rc1 = parser.nextEvent(c1, &ev);
    expect_eq(rc1.has_event, false);

    const c2: string = "\n\r\ndata: second\r\r";
    let rest: []const u8 = c2;
    const rc2 = parser.nextEvent(rest, &ev);
    expect_eq(rc2.has_event, true);
    expect_eq(strEq(ev.data, "first"), true);

    rest = rest[rc2.consumed..];
    const rc3 = parser.nextEvent(rest, &ev);
    expect_eq(rc3.has_event, true);
    expect_eq(strEq(ev.data, "second"), true);
});

test("4. extractJsonStringField decodes escapes, ignores value substrings, and rejects null/overflow", () => {
    let out_b: [128]u8 = std.mem.zeroes([128]u8);
    let out_s: []u8 = out_b[0..];

    // OpenAI delta with escapes: \n, \t, \", \\, \u0041 ('A')
    const json1: string = "{\"note\":\"fake \\\"content\\\":\\\"bad\\\"\",\"delta\":{\"content\":\"Hi\\n\\t\\\"CP\\\"\\\\\\u0041\"}}";
    const n1 = extractJsonStringField(json1, "content", &out_s) ?? unreachable;
    expect_eq(strEq(out_s[0..n1], "Hi\n\t\"CP\"\\A"), true);

    // Anthropic text delta
    const json2: string = "{\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"hello world\"}}";
    const n2 = extractJsonStringField(json2, "text", &out_s) ?? unreachable;
    expect_eq(strEq(out_s[0..n2], "hello world"), true);

    // Null value or missing key -> null
    const json_null: string = "{\"delta\":{\"content\":null}}";
    expect_eq(extractJsonStringField(json_null, "content", &out_s) == null, true);
    expect_eq(extractJsonStringField(json2, "missing", &out_s) == null, true);

    // Undersized output buffer -> null
    let tiny: [3]u8 = std.mem.zeroes([3]u8);
    let tiny_s: []u8 = tiny[0..];
    expect_eq(extractJsonStringField(json2, "text", &tiny_s) == null, true);
});

test("5. StreamTelemetry accurately computes TTFT, TPS, inter-token average, and empty-stream sentinel", () => {
    let tel = StreamTelemetry.init(1000);
    // Warmup metadata chunk with 0 tokens does not trigger TTFT
    tel.onTokenChunk(1050, 0);
    // Token chunks: 1200 (10 tok), 1250 (10 tok), 1300 (10 tok)
    tel.onTokenChunk(1200, 10);
    tel.onTokenChunk(1250, 10);
    tel.onTokenChunk(1300, 10);

    const m = tel.finish(1400);
    expect_eq(m.ttft_ms, 200);
    expect_eq(m.total_ms, 400);
    expect_eq(m.chunk_count, 4);
    expect_eq(m.total_tokens, 30);
    // 30 tokens / 0.4 sec = 75.0 TPS
    expect_eq(approxEq(m.tokens_per_sec, 75.0, 1e-9), true);
    // (1300 - 1200) / (4 - 1) = 100 / 3 ms
    expect_eq(approxEq(m.inter_token_avg_ms, 100.0 / 3.0, 1e-9), true);

    // Empty stream -> ttft_ms == -1, tps == 0.0
    let empty_tel = StreamTelemetry.init(500);
    const em = empty_tel.finish(600);
    expect_eq(em.ttft_ms, -1);
    expect_eq(em.total_ms, 100);
    expect_eq(em.tokens_per_sec, 0.0);
});

test("6. SseStreamParser gracefully signals overflow without crashing and recovers after reset", () => {
    let line_b: [16]u8 = std.mem.zeroes([16]u8);
    let data_b: [12]u8 = std.mem.zeroes([12]u8);
    let ev_b: [8]u8 = std.mem.zeroes([8]u8);
    let id_b: [8]u8 = std.mem.zeroes([8]u8);
    let line_s: []u8 = line_b[0..];
    let data_s: []u8 = data_b[0..];
    let ev_s: []u8 = ev_b[0..];
    let id_s: []u8 = id_b[0..];

    let parser = SseStreamParser.init(&line_s, &data_s, &ev_s, &id_s);
    let ev = SseEvent { event_type: "", data: "", id: "", is_done: false };

    // 1. line_buf overflow (> 16 bytes on a single line)
    const long_line: string = "data: 0123456789abcdef0123456789\n\n";
    const r_line = parser.nextEvent(long_line, &ev);
    expect_eq(r_line.overflow, true);
    expect_eq(r_line.has_event, false);

    // 2. data_buf overflow across multiple short data lines (8 + 1 + 8 = 17 > 12)
    parser.reset();
    const multi_data: string = "data: 12345678\ndata: 12345678\n\n";
    const r_data = parser.nextEvent(multi_data, &ev);
    expect_eq(r_data.overflow, true);
    expect_eq(r_data.has_event, false);

    // 3. event_buf overflow (> 8 bytes)
    parser.reset();
    const long_ev: string = "event: toolong123\n";
    const r_ev = parser.nextEvent(long_ev, &ev);
    expect_eq(r_ev.overflow, true);

    // 4. Recovers cleanly after reset()
    parser.reset();
    const ok_msg: string = "event: ok\ndata: hi\n\n";
    const r_ok = parser.nextEvent(ok_msg, &ev);
    expect_eq(r_ok.overflow, false);
    expect_eq(r_ok.has_event, true);
    expect_eq(strEq(ev.event_type, "ok"), true);
    expect_eq(strEq(ev.data, "hi"), true);
});
