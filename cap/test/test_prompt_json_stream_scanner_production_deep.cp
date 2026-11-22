// Origin: Pure-CP Stdlib Test Suite (lib/prompt/json_stream_scanner.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Streaming JSON Field Scanner:
//   1. Complete top-level multi-type parsing (string, number, boolean, null) & empty object {}
//   2. Exact equivalence between 1-byte streaming feed and single-chunk feed
//   3. Cross-chunk escape sequence decoding (\", \\, \/, \n, \r, \t split across chunk boundaries)
//   4. Streaming string_delta incremental reassembly matching final field_complete payload
//   5. Nested object {...} & array [...] depth tracking with embedded braces inside strings
//   6. Malformed JSON guards (non-object root, bad escape, raw control char, >16 depth, bad pair, trailing comma)
//   7. Clean reset & reuse from both completed and error states + stats consistency

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    JSON_STREAM_MAX_DEPTH,
    JSON_STREAM_MAX_KEY_LEN,
    JSON_STREAM_MAX_VAL_LEN,
    JSON_STREAM_MAX_EVENTS,
    JsonStreamEventKind,
    JsonStreamValueKind,
    JsonStreamEvent,
    JsonStreamStats,
    JsonStreamScanner,
} from "cap:prompt";

// ============================================================================
// Test 1: Top-Level Multi-Type Fields (string, number, boolean, null) & {}
// ============================================================================
test("json_stream_scanner: parses string, number, boolean, null & empty object", () => {
    let sc = JsonStreamScanner.init();
    let evs: [32]JsonStreamEvent = [JsonStreamEvent{}; 32];
    let evs_s: []JsonStreamEvent = evs[0..];

    const payload = "{\"name\":\"cp\",\"count\":-42.5e2,\"ok\":true,\"off\":false,\"err\":null}";
    const n = sc.feed(payload, evs_s);
    expect_eq(sc.is_complete(), true);
    expect_eq(sc.has_error(), false);

    let completed_fields: usize = 0;
    let saw_obj_complete = false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (evs[i].kind == JsonStreamEventKind.field_complete) {
            if (completed_fields == 0) {
                expect_eq(std.mem.eql(u8, evs[i].keySlice(), "name"), true);
                expect_eq(evs[i].val_kind == JsonStreamValueKind.string, true);
                expect_eq(std.mem.eql(u8, evs[i].dataSlice(), "cp"), true);
            } else if (completed_fields == 1) {
                expect_eq(std.mem.eql(u8, evs[i].keySlice(), "count"), true);
                expect_eq(evs[i].val_kind == JsonStreamValueKind.number, true);
                expect_eq(std.mem.eql(u8, evs[i].dataSlice(), "-42.5e2"), true);
            } else if (completed_fields == 2) {
                expect_eq(std.mem.eql(u8, evs[i].keySlice(), "ok"), true);
                expect_eq(evs[i].val_kind == JsonStreamValueKind.boolean, true);
                expect_eq(std.mem.eql(u8, evs[i].dataSlice(), "true"), true);
            } else if (completed_fields == 3) {
                expect_eq(std.mem.eql(u8, evs[i].keySlice(), "off"), true);
                expect_eq(evs[i].val_kind == JsonStreamValueKind.boolean, true);
                expect_eq(std.mem.eql(u8, evs[i].dataSlice(), "false"), true);
            } else if (completed_fields == 4) {
                expect_eq(std.mem.eql(u8, evs[i].keySlice(), "err"), true);
                expect_eq(evs[i].val_kind == JsonStreamValueKind.null_val, true);
                expect_eq(std.mem.eql(u8, evs[i].dataSlice(), "null"), true);
            }
            completed_fields += 1;
        } else if (evs[i].kind == JsonStreamEventKind.object_complete) {
            saw_obj_complete = true;
        }
    }
    expect_eq(completed_fields, 5);
    expect_eq(saw_obj_complete, true);
    expect_eq(sc.stats().fields_completed, 5);

    // Empty object {}
    sc.reset();
    const n_empty = sc.feed("  {  }  ", evs_s);
    expect_eq(n_empty, 1);
    expect_eq(evs[0].kind == JsonStreamEventKind.object_complete, true);
    expect_eq(sc.is_complete(), true);
});

// ============================================================================
// Test 2: 1-Byte Streaming Feed vs Single-Chunk Feed Exact Equivalence
// ============================================================================
test("json_stream_scanner: 1-byte chunked feed matches whole-buffer feed exactly", () => {
    const json = "{\"tool\":\"read_file\",\"path\":\"/src/main.cp\",\"lines\":120,\"raw\":false}";

    // Whole-buffer run
    let sc_whole = JsonStreamScanner.init();
    let evs_whole: [32]JsonStreamEvent = [JsonStreamEvent{}; 32];
    let evs_whole_s: []JsonStreamEvent = evs_whole[0..];
    const nw = sc_whole.feed(json, evs_whole_s);
    expect_eq(sc_whole.is_complete(), true);

    let whole_fields: [8]JsonStreamEvent = [JsonStreamEvent{}; 8];
    let wf_count: usize = 0;
    let i: usize = 0;
    while (i < nw) : (i += 1) {
        if (evs_whole[i].kind == JsonStreamEventKind.field_complete) {
            whole_fields[wf_count] = evs_whole[i];
            wf_count += 1;
        }
    }

    // 1-byte streaming run
    let sc_byte = JsonStreamScanner.init();
    let evs_step: [8]JsonStreamEvent = [JsonStreamEvent{}; 8];
    let evs_step_s: []JsonStreamEvent = evs_step[0..];
    let byte_fields: [8]JsonStreamEvent = [JsonStreamEvent{}; 8];
    let bf_count: usize = 0;

    let pos: usize = 0;
    while (pos < json.len) : (pos += 1) {
        const nb = sc_byte.feed(json[pos .. pos + 1], evs_step_s);
        let k: usize = 0;
        while (k < nb) : (k += 1) {
            if (evs_step[k].kind == JsonStreamEventKind.field_complete) {
                byte_fields[bf_count] = evs_step[k];
                bf_count += 1;
            }
        }
    }

    expect_eq(sc_byte.is_complete(), true);
    expect_eq(sc_byte.has_error(), false);
    expect_eq(bf_count, wf_count);
    expect_eq(bf_count, 4);

    let f: usize = 0;
    while (f < wf_count) : (f += 1) {
        expect_eq(byte_fields[f].val_kind == whole_fields[f].val_kind, true);
        expect_eq(std.mem.eql(u8, byte_fields[f].keySlice(), whole_fields[f].keySlice()), true);
        expect_eq(std.mem.eql(u8, byte_fields[f].dataSlice(), whole_fields[f].dataSlice()), true);
    }
});

// ============================================================================
// Test 3: Cross-Chunk Escape Sequence Decoding
// ============================================================================
test("json_stream_scanner: decodes escape sequences split across chunk boundaries", () => {
    let sc = JsonStreamScanner.init();
    let evs: [16]JsonStreamEvent = [JsonStreamEvent{}; 16];
    let evs_s: []JsonStreamEvent = evs[0..];

    // Intentionally split \", \\, \n, \t right after the backslash
    const c1 = "{\"msg\":\"say \\";
    const c2 = "\"hi\\\" & \\";
    const c3 = "\\path\\";
    const c4 = "nline2\\";
    const c5 = "tend\"}";

    * = sc.feed(c1, evs_s);
    * = sc.feed(c2, evs_s);
    * = sc.feed(c3, evs_s);
    * = sc.feed(c4, evs_s);
    const n5 = sc.feed(c5, evs_s);
    expect_eq(sc.is_complete(), true);
    expect_eq(sc.has_error(), false);

    let found_field = false;
    let i: usize = 0;
    while (i < n5) : (i += 1) {
        if (evs[i].kind == JsonStreamEventKind.field_complete) {
            found_field = true;
            expect_eq(std.mem.eql(u8, evs[i].keySlice(), "msg"), true);
            expect_eq(std.mem.eql(u8, evs[i].dataSlice(), "say \"hi\" & \\path\nline2\tend"), true);
        }
    }
    expect_eq(found_field, true);
});

// ============================================================================
// Test 4: Streaming string_delta Incremental Reassembly
// ============================================================================
test("json_stream_scanner: streaming string_delta reassembles to exact field_complete", () => {
    let sc = JsonStreamScanner.init();
    let evs: [8]JsonStreamEvent = [JsonStreamEvent{}; 8];
    let evs_s: []JsonStreamEvent = evs[0..];

    const chunks: [5]string = [
        "{\"code\":\"fn ",
        "main() { ",
        "return ",
        "42; }",
        "\"}",
    ];

    let joined: [128]u8 = [0; 128];
    let joined_len: usize = 0;
    let final_val: [128]u8 = [0; 128];
    let final_len: usize = 0;
    let delta_cnt: usize = 0;

    let c: usize = 0;
    while (c < 5) : (c += 1) {
        const n = sc.feed(chunks[c], evs_s);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (evs[i].kind == JsonStreamEventKind.string_delta) {
                delta_cnt += 1;
                const d = evs[i].dataSlice();
                let j: usize = 0;
                while (j < d.len) : (j += 1) {
                    joined[joined_len] = d[j];
                    joined_len += 1;
                }
            } else if (evs[i].kind == JsonStreamEventKind.field_complete) {
                const d = evs[i].dataSlice();
                let j: usize = 0;
                while (j < d.len) : (j += 1) {
                    final_val[j] = d[j];
                }
                final_len = d.len;
            }
        }
    }

    expect_eq(sc.is_complete(), true);
    expect_eq(delta_cnt, 4);
    expect_eq(sc.stats().delta_events, 4);
    expect_eq(std.mem.eql(u8, joined[0..joined_len], "fn main() { return 42; }"), true);
    expect_eq(std.mem.eql(u8, joined[0..joined_len], final_val[0..final_len]), true);
});

// ============================================================================
// Test 5: Nested Object & Array Depth Tracking with Embedded Braces in Strings
// ============================================================================
test("json_stream_scanner: captures nested objects & arrays ignoring braces in strings", () => {
    let sc = JsonStreamScanner.init();
    let evs: [16]JsonStreamEvent = [JsonStreamEvent{}; 16];
    let evs_s: []JsonStreamEvent = evs[0..];

    const json = "{\"meta\":{\"k\":\"v}with]brace\",\"arr\":[1,{\"x\":true}]},\"tags\":[\"a\",\"b\"]}";
    const n = sc.feed(json, evs_s);
    expect_eq(sc.is_complete(), true);
    expect_eq(sc.has_error(), false);

    let f_idx: usize = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (evs[i].kind == JsonStreamEventKind.field_complete) {
            if (f_idx == 0) {
                expect_eq(std.mem.eql(u8, evs[i].keySlice(), "meta"), true);
                expect_eq(evs[i].val_kind == JsonStreamValueKind.nested_object, true);
                expect_eq(std.mem.eql(u8, evs[i].dataSlice(), "{\"k\":\"v}with]brace\",\"arr\":[1,{\"x\":true}]}"), true);
            } else if (f_idx == 1) {
                expect_eq(std.mem.eql(u8, evs[i].keySlice(), "tags"), true);
                expect_eq(evs[i].val_kind == JsonStreamValueKind.nested_array, true);
                expect_eq(std.mem.eql(u8, evs[i].dataSlice(), "[\"a\",\"b\"]"), true);
            }
            f_idx += 1;
        }
    }
    expect_eq(f_idx, 2);
});

// ============================================================================
// Test 6: Malformed JSON, Non-Object Root, Bad Escape & >16 Depth Guards
// ============================================================================
test("json_stream_scanner: emits syntax_error on invalid root, escape, depth & brackets", () => {
    expect_eq(JSON_STREAM_MAX_DEPTH, 16);
    expect_eq(JSON_STREAM_MAX_KEY_LEN, 64);
    expect_eq(JSON_STREAM_MAX_VAL_LEN, 256);
    expect_eq(JSON_STREAM_MAX_EVENTS, 32);

    let evs: [16]JsonStreamEvent = [JsonStreamEvent{}; 16];
    let evs_s: []JsonStreamEvent = evs[0..];

    // 1. Non-object root '[1,2]'
    let s1 = JsonStreamScanner.init();
    const n1 = s1.feed("[1,2]", evs_s);
    expect_eq(s1.has_error(), true);
    expect_eq(n1 >= 1 && evs[n1 - 1].kind == JsonStreamEventKind.syntax_error, true);
    // Subsequent feed after error returns 0
    expect_eq(s1.feed("{\"a\":1}", evs_s), 0);

    // 2. Invalid escape '\q'
    let s2 = JsonStreamScanner.init();
    * = s2.feed("{\"a\":\"\\q\"}", evs_s);
    expect_eq(s2.has_error(), true);

    // 3. Unescaped raw control char (LF) inside string
    let s3 = JsonStreamScanner.init();
    * = s3.feed("{\"a\":\"bad\nline\"}", evs_s);
    expect_eq(s3.has_error(), true);

    // 4. Mismatched brackets in nested value
    let s4 = JsonStreamScanner.init();
    * = s4.feed("{\"a\":[1,2}}", evs_s);
    expect_eq(s4.has_error(), true);

    // 5. Trailing comma rejected
    let s5 = JsonStreamScanner.init();
    * = s5.feed("{\"a\":1,}", evs_s);
    expect_eq(s5.has_error(), true);

    // 6. Nesting depth > 16 (root '{' + 16 nested '[' = depth 17)
    let s6 = JsonStreamScanner.init();
    * = s6.feed("{\"a\":[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]}", evs_s);
    expect_eq(s6.has_error(), true);
});

// ============================================================================
// Test 7: Clean reset & Reuse with Stats Consistency
// ============================================================================
test("json_stream_scanner: reset clears error/completed state for clean reuse", () => {
    let sc = JsonStreamScanner.init();
    let evs: [16]JsonStreamEvent = [JsonStreamEvent{}; 16];
    let evs_s: []JsonStreamEvent = evs[0..];

    * = sc.feed("bad_json", evs_s);
    expect_eq(sc.has_error(), true);

    sc.reset();
    expect_eq(sc.has_error(), false);
    expect_eq(sc.is_complete(), false);
    const st0 = sc.stats();
    expect_eq(st0.bytes_processed, 0);
    expect_eq(st0.fields_completed, 0);

    const msg = "{\"ok\":true}";
    * = sc.feed(msg, evs_s);
    expect_eq(sc.is_complete(), true);
    const st1 = sc.stats();
    expect_eq(st1.bytes_processed, msg.len as u64);
    expect_eq(st1.fields_completed, 1);
    expect_eq(st1.completed, true);
    expect_eq(st1.has_error, false);
});
