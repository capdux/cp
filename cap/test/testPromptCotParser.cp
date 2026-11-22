// Origin: Pure-CP Stdlib Test Suite (lib/prompt/cotParser.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Streaming CoT Multi-Channel Tag Parser & Thinking-Budget Guard:
//   1. 1-byte chunk streaming across split <think>/<thinking>/<reflection>/<answer> tags (case-insensitive)
//   2. Markdown code-fence (```) pseudo-tag immunity both in answer and inside <think> blocks
//   3. Thinking-budget guard (max_thinking_bytes): budget_exceeded signal, dropped byte accounting & answer continuation
//   4. Configurable tag family masks (e.g. COT_TAG_REFLECTION only)
//   5. Malformed tag resilience: nested open tags, orphan close tags, unclosed tags auto-repaired on finish() / <answer>
//   6. Resumable partial-drain loop with tiny 4-byte out_buf & 1-event out_events buffer (zero byte loss)
//   7. Edge cases: empty chunks, false-prefix rollback (`<<think>`, `<div>`, inline `` ` ``), reset(), invalid config & cap.prompt facade

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    COT_TAG_THINK,
    COT_TAG_THINKING,
    COT_TAG_REFLECTION,
    COT_TAG_ANSWER,
    COT_TAG_ALL,
    CotSegmentKind,
    CotParserConfig,
    CotSegmentEvent,
    CotStreamParser,
} from "cap:prompt";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn appendSlice(dst: *[]u8, dst_len: *usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len && dst_len.* < dst.len) : (i += 1) {
        dst[dst_len.*] = src[i];
        dst_len.* += 1;
    }
}

// ============================================================================
// Test 1: 1-Byte Chunk Streaming Across Split Tags & Multiple Tag Families
// ============================================================================
test("cotParser: 1-byte chunk streaming across split <think>, <reflection> & <answer> tags", () => {
    let parser = CotStreamParser.init(CotParserConfig.defaults()) ?? return;
    const stream: []const u8 = "<THINK>step1</think><reflection>step2</reflection><answer>42</answer>";

    let think_acc: [64]u8 = [0; 64];
    let think_len: usize = 0;
    let ans_acc: [64]u8 = [0; 64];
    let ans_len: usize = 0;

    let out_buf: [32]u8 = [0; 32];
    let out_s: []u8 = out_buf[0..];
    let ev_buf: [8]CotSegmentEvent = [CotSegmentEvent{}; 8];
    let ev_s: []CotSegmentEvent = ev_buf[0..];

    // Feed 1 byte at a time to stress-test split tags like "<thi" + "nk>"
    let i: usize = 0;
    while (i < stream.len) : (i += 1) {
        const res = parser.feed(stream[i .. i + 1], *out_s, *ev_s);
        expect_eq(res.bytes_consumed, 1 as usize);
        expect_eq(res.needs_drain, false);
        let e: usize = 0;
        while (e < res.events_emitted) : (e += 1) {
            const ev = ev_buf[e];
            const seg = out_buf[ev.start .. ev.start + ev.len];
            if (ev.kind == CotSegmentKind.thinking) {
                let ts: []u8 = think_acc[0..];
                appendSlice(*ts, &think_len, seg);
            } else if (ev.kind == CotSegmentKind.answer) {
                let as_s: []u8 = ans_acc[0..];
                appendSlice(*as_s, &ans_len, seg);
            }
        }
    }

    const fin = parser.finish(*out_s, *ev_s);
    expect_eq(fin.needs_drain, false);
    expect_true(strEq(think_acc[0..think_len], "step1step2"));
    expect_true(strEq(ans_acc[0..ans_len], "42"));

    const sum = parser.summary();
    expect_eq(sum.thinking_bytes, 10 as u64);
    expect_eq(sum.answer_bytes, 2 as u64);
    expect_eq(sum.thinking_blocks, 2 as u32);
    expect_eq(sum.answer_blocks, 1 as u32);
    expect_eq(sum.unclosed_tags_repaired, 0 as u32);
});

// ============================================================================
// Test 2: Code-Fence Pseudo-Tag Immunity (Answer & Inside Thinking)
// ============================================================================
test("cotParser: ignores pseudo <think>/</think> tags inside ``` code fences", () => {
    let parser = CotStreamParser.init(CotParserConfig.defaults()) ?? return;
    const chunks: [4][]const u8 = [
        "Use ``",
        "`xml\n<thi",
        "nk>demo</think>\n``` done.<think>real ``",
        "`</think>``` still thinking</think>OK",
    ];

    let think_acc: [128]u8 = [0; 128];
    let think_len: usize = 0;
    let ans_acc: [128]u8 = [0; 128];
    let ans_len: usize = 0;
    let code_acc: [128]u8 = [0; 128];
    let code_len: usize = 0;

    let out_buf: [128]u8 = [0; 128];
    let out_s: []u8 = out_buf[0..];
    let ev_buf: [16]CotSegmentEvent = [CotSegmentEvent{}; 16];
    let ev_s: []CotSegmentEvent = ev_buf[0..];

    let c: usize = 0;
    while (c < 4) : (c += 1) {
        const res = parser.feed(chunks[c], *out_s, *ev_s);
        expect_eq(res.bytes_consumed, chunks[c].len);
        let e: usize = 0;
        while (e < res.events_emitted) : (e += 1) {
            const ev = ev_buf[e];
            const seg = out_buf[ev.start .. ev.start + ev.len];
            if (ev.kind == CotSegmentKind.thinking) {
                let ts: []u8 = think_acc[0..];
                appendSlice(*ts, &think_len, seg);
            } else if (ev.kind == CotSegmentKind.answer) {
                let as_s: []u8 = ans_acc[0..];
                appendSlice(*as_s, &ans_len, seg);
            } else if (ev.kind == CotSegmentKind.code_fence) {
                let cs: []u8 = code_acc[0..];
                appendSlice(*cs, &code_len, seg);
            }
        }
    }
    * = parser.finish(*out_s, *ev_s);

    // Pseudo <think> and </think> in answer code fence + </think> in thinking code fence = 3 ignored
    const sum = parser.summary();
    expect_eq(sum.ignored_code_fence_tags, 3 as u32);
    expect_eq(sum.code_fence_blocks, 2 as u32);
    expect_eq(sum.thinking_blocks, 1 as u32);
    expect_true(strEq(think_acc[0..think_len], "real  still thinking"));
    expect_true(strEq(ans_acc[0..ans_len], "Use  done.OK"));
    expect_true(strEq(code_acc[0..code_len], "```xml\n<think>demo</think>\n``````</think>```"));
});

// ============================================================================
// Test 3: Thinking-Budget Guard (max_thinking_bytes)
// ============================================================================
test("cotParser: thinking budget guard emits budget_exceeded and drops excess thinking bytes", () => {
    const cfg = CotParserConfig{
        tag_mask: COT_TAG_ALL,
        max_thinking_bytes: 12,
        respect_code_fences: true,
    };
    let parser = CotStreamParser.init(cfg) ?? return;

    const input: []const u8 = "<think>0123456789ABCDEFGH</think>FinalAnswer";
    let out_buf: [128]u8 = [0; 128];
    let out_s: []u8 = out_buf[0..];
    let ev_buf: [16]CotSegmentEvent = [CotSegmentEvent{}; 16];
    let ev_s: []CotSegmentEvent = ev_buf[0..];

    const res = parser.feed(input, *out_s, *ev_s);
    expect_eq(res.bytes_consumed, input.len);
    expect_eq(res.budget_exceeded, true);
    expect_eq(parser.isBudgetExceeded(), true);

    // Expect 3 events: thinking("0123456789AB"), budget_exceeded(len=0), answer("FinalAnswer")
    expect_eq(res.events_emitted, 3 as usize);
    expect_eq(ev_buf[0].kind, CotSegmentKind.thinking);
    expect_true(strEq(out_buf[ev_buf[0].start .. ev_buf[0].start + ev_buf[0].len], "0123456789AB"));
    expect_eq(ev_buf[1].kind, CotSegmentKind.budget_exceeded);
    expect_eq(ev_buf[1].len, 0 as usize);
    expect_eq(ev_buf[2].kind, CotSegmentKind.answer);
    expect_true(strEq(out_buf[ev_buf[2].start .. ev_buf[2].start + ev_buf[2].len], "FinalAnswer"));

    * = parser.finish(*out_s, *ev_s);
    const sum = parser.summary();
    expect_eq(sum.thinking_bytes, 12 as u64);
    expect_eq(sum.estimated_thinking_tokens, 3 as u64);
    expect_eq(sum.dropped_thinking_bytes, 6 as u64);
    expect_eq(sum.answer_bytes, 11 as u64);
    expect_eq(sum.thinking_budget_exceeded, true);
});

// ============================================================================
// Test 4: Configurable Tag Family Mask (COT_TAG_REFLECTION Only)
// ============================================================================
test("cotParser: configurable tag_mask isolates only enabled tag families", () => {
    const cfg = CotParserConfig{
        tag_mask: COT_TAG_REFLECTION,
        max_thinking_bytes: 0,
        respect_code_fences: true,
    };
    let parser = CotStreamParser.init(cfg) ?? return;

    const input: []const u8 = "<think>literal</think><reflection>inner</reflection>";
    let out_buf: [128]u8 = [0; 128];
    let out_s: []u8 = out_buf[0..];
    let ev_buf: [8]CotSegmentEvent = [CotSegmentEvent{}; 8];
    let ev_s: []CotSegmentEvent = ev_buf[0..];

    const res = parser.feed(input, *out_s, *ev_s);
    expect_eq(res.events_emitted, 2 as usize);
    expect_eq(ev_buf[0].kind, CotSegmentKind.answer);
    expect_true(strEq(out_buf[ev_buf[0].start .. ev_buf[0].start + ev_buf[0].len], "<think>literal</think>"));
    expect_eq(ev_buf[1].kind, CotSegmentKind.thinking);
    expect_true(strEq(out_buf[ev_buf[1].start .. ev_buf[1].start + ev_buf[1].len], "inner"));
});

// ============================================================================
// Test 5: Malformed Tags (Nested Open, Orphan Close, Unclosed Auto-Repair)
// ============================================================================
test("cotParser: handles nested open tags, orphan close tags & unclosed tag auto-repair", () => {
    let parser = CotStreamParser.init(CotParserConfig.defaults()) ?? return;
    // Orphan </think> + nested <think><thinking> + unclosed <think> repaired by <answer> + unclosed <answer> at EOF
    const input: []const u8 = "</think>A<think><thinking>B</think><think>C<answer>D";

    let out_buf: [64]u8 = [0; 64];
    let out_s: []u8 = out_buf[0..];
    let ev_buf: [16]CotSegmentEvent = [CotSegmentEvent{}; 16];
    let ev_s: []CotSegmentEvent = ev_buf[0..];

    * = parser.feed(input, *out_s, *ev_s);
    * = parser.finish(*out_s, *ev_s);

    const sum = parser.summary();
    expect_eq(sum.orphan_close_tags, 1 as u32);
    expect_eq(sum.nested_open_tags, 1 as u32);
    // 1 repaired when <answer> closed <think>, 1 repaired on finish() for unclosed <answer>
    expect_eq(sum.unclosed_tags_repaired, 2 as u32);
    expect_eq(sum.thinking_bytes, 2 as u64); // "BC"
    expect_eq(sum.answer_bytes, 2 as u64);   // "AD"
});

// ============================================================================
// Test 6: Resumable Partial Drain with Tiny Output Buffers
// ============================================================================
test("cotParser: resumable batch draining with tiny 4-byte out_buf & 1-event buffer", () => {
    let parser = CotStreamParser.init(CotParserConfig.defaults()) ?? return;
    const input: []const u8 = "Pre<think>LongThoughtHere</think>Post<thx";

    let think_acc: [64]u8 = [0; 64];
    let think_len: usize = 0;
    let ans_acc: [64]u8 = [0; 64];
    let ans_len: usize = 0;

    let tiny_buf: [4]u8 = [0; 4];
    let tiny_buf_s: []u8 = tiny_buf[0..];
    let tiny_ev: [1]CotSegmentEvent = [CotSegmentEvent{}];
    let tiny_ev_s: []CotSegmentEvent = tiny_ev[0..];

    let offset: usize = 0;
    let safety_iters: usize = 0;
    while (offset < input.len && safety_iters < 64) : (safety_iters += 1) {
        const res = parser.feed(input[offset..], *tiny_buf_s, *tiny_ev_s);
        offset += res.bytes_consumed;
        let e: usize = 0;
        while (e < res.events_emitted) : (e += 1) {
            const ev = tiny_ev[e];
            const seg = tiny_buf[ev.start .. ev.start + ev.len];
            if (ev.kind == CotSegmentKind.thinking) {
                let ts: []u8 = think_acc[0..];
                appendSlice(*ts, &think_len, seg);
            } else if (ev.kind == CotSegmentKind.answer) {
                let as_s: []u8 = ans_acc[0..];
                appendSlice(*as_s, &ans_len, seg);
            }
        }
    }
    expect_eq(offset, input.len);

    // Drain finish() in a loop as well
    let fin_done: bool = false;
    safety_iters = 0;
    while (!fin_done && safety_iters < 16) : (safety_iters += 1) {
        const fin = parser.finish(*tiny_buf_s, *tiny_ev_s);
        let e: usize = 0;
        while (e < fin.events_emitted) : (e += 1) {
            const ev = tiny_ev[e];
            const seg = tiny_buf[ev.start .. ev.start + ev.len];
            if (ev.kind == CotSegmentKind.thinking) {
                let ts: []u8 = think_acc[0..];
                appendSlice(*ts, &think_len, seg);
            } else if (ev.kind == CotSegmentKind.answer) {
                let as_s: []u8 = ans_acc[0..];
                appendSlice(*as_s, &ans_len, seg);
            }
        }
        if (!fin.needs_drain) {
            fin_done = true;
        }
    }
    expect_true(fin_done);
    expect_true(strEq(think_acc[0..think_len], "LongThoughtHere"));
    expect_true(strEq(ans_acc[0..ans_len], "PrePost<thx"));
});

// ============================================================================
// Test 7: Edge Cases (Empty Chunks, False Prefixes, Reset, Invalid Config & cap.prompt Facade)
// ============================================================================
test("cotParser: empty chunks, <<think> false prefix, inline backticks, reset & cap.prompt facade", () => {
    // Invalid config (tag_mask == 0) must return null
    expect_eq(CotStreamParser.init(CotParserConfig{ tag_mask: 0 }) == null, true);

    // Construct via dynamic cap.prompt facade
    let parser = cap.prompt.CotStreamParser.init(CotParserConfig.defaults()) ?? return;
    let out_buf: [64]u8 = [0; 64];
    let out_s: []u8 = out_buf[0..];
    let ev_buf: [8]CotSegmentEvent = [CotSegmentEvent{}; 8];
    let ev_s: []CotSegmentEvent = ev_buf[0..];

    // Empty chunk is a no-op
    const empty_res = parser.feed("", *out_s, *ev_s);
    expect_eq(empty_res.bytes_consumed, 0 as usize);
    expect_eq(empty_res.bytes_written, 0 as usize);
    expect_eq(empty_res.events_emitted, 0 as usize);

    // False prefixes: `<<think>`, `<div>`, inline `` `x` ``
    const input: []const u8 = "<<think>ok</think>`x`<div>";
    const res = parser.feed(input, *out_s, *ev_s);
    expect_eq(res.bytes_consumed, input.len);
    expect_eq(res.events_emitted, 3 as usize);
    expect_eq(ev_buf[0].kind, CotSegmentKind.answer);
    expect_true(strEq(out_buf[ev_buf[0].start .. ev_buf[0].start + ev_buf[0].len], "<"));
    expect_eq(ev_buf[1].kind, CotSegmentKind.thinking);
    expect_true(strEq(out_buf[ev_buf[1].start .. ev_buf[1].start + ev_buf[1].len], "ok"));
    expect_eq(ev_buf[2].kind, CotSegmentKind.answer);
    expect_true(strEq(out_buf[ev_buf[2].start .. ev_buf[2].start + ev_buf[2].len], "`x`<div>"));

    // Reset clears all counters cleanly
    parser.reset();
    const sum_zero = parser.summary();
    expect_eq(sum_zero.thinking_bytes, 0 as u64);
    expect_eq(sum_zero.answer_bytes, 0 as u64);
    expect_eq(sum_zero.thinking_blocks, 0 as u32);
});
