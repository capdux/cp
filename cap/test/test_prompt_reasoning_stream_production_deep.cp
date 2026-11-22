// Origin: Pure-CP Stdlib Test Suite (AI Reasoning Model Streaming <think>/<tool_call> Tag Demuxer) | Created: 2026-11-22
// Deep Production Verification Suite for lib/prompt/reasoning_stream.cp
// Covers:
//   1. Single-shot vs 1-byte-at-a-time streaming demux across split tags ("<thi"+"nk>", "<tool_"+"call>")
//   2. False tag prefix rollback ("<this", "<t<think>", "<tool_c<tool_call>") with zero byte loss
//   3. Multi-turn mixed Thinking + ToolCall + Answer demux, inner literal tags & DemuxStats accuracy
//   4. Unclosed partial tag flush at stream EOF (both in Answer and Thinking channels)
//   5. Empty chunk, undersized out_buf / out_events overflow protection & reset() reuse

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    StreamChannel,
    DemuxEvent,
    DemuxStats,
    ReasoningStreamDemuxer,
} from "../../lib/prompt/index.cp";

fn appendSlice(dst: *[]u8, dst_len: *usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len && dst_len.* < dst.len) : (i += 1) {
        dst[dst_len.*] = src[i];
        dst_len.* += 1;
    }
}

fn collectDemuxedByChunkSize(
    input: string,
    chunk_size: usize,
    ans_buf: *[]u8,
    ans_len: *usize,
    thk_buf: *[]u8,
    thk_len: *usize,
    tool_buf: *[]u8,
    tool_len: *usize,
): DemuxStats {
    let demux = ReasoningStreamDemuxer.init();
    let out_buf: [256]u8 = [0; 256];
    let ev_buf: [16]DemuxEvent = [DemuxEvent{}; 16];

    let pos: usize = 0;
    while (pos < input.len) {
        const end = if (pos + chunk_size < input.len) (pos + chunk_size) else input.len;
        const n = demux.feed(input[pos..end], out_buf[0..], ev_buf[0..]) ?? 0;
        let k: usize = 0;
        while (k < n) : (k += 1) {
            const ev = ev_buf[k];
            const payload = out_buf[ev.start..(ev.start + ev.len)];
            if (ev.channel == StreamChannel.Answer) {
                appendSlice(ans_buf, ans_len, payload);
            } else if (ev.channel == StreamChannel.Thinking) {
                appendSlice(thk_buf, thk_len, payload);
            } else {
                appendSlice(tool_buf, tool_len, payload);
            }
        }
        pos = end;
    }

    const nf = demux.flush(out_buf[0..], ev_buf[0..]) ?? 0;
    let f: usize = 0;
    while (f < nf) : (f += 1) {
        const ev = ev_buf[f];
        const payload = out_buf[ev.start..(ev.start + ev.len)];
        if (ev.channel == StreamChannel.Answer) {
            appendSlice(ans_buf, ans_len, payload);
        } else if (ev.channel == StreamChannel.Thinking) {
            appendSlice(thk_buf, thk_len, payload);
        } else {
            appendSlice(tool_buf, tool_len, payload);
        }
    }

    return demux.stats();
}

fn testWholeAndByteByByteTagSplitting()! {
    const full = "Hello <THINK>step 1</Think><tool_call>{\"fn\":\"ls\"}</tool_call> world!";

    // 1. Whole-chunk feed
    let a1: [128]u8 = [0; 128];
    let t1: [128]u8 = [0; 128];
    let c1: [128]u8 = [0; 128];
    let a1_len: usize = 0;
    let t1_len: usize = 0;
    let c1_len: usize = 0;
    const st1 = collectDemuxedByChunkSize(full, full.len, a1[0..], *a1_len, t1[0..], *t1_len, c1[0..], *c1_len);

    expect_eq(std.mem.eql(u8, a1[0..a1_len], "Hello  world!"), true);
    expect_eq(std.mem.eql(u8, t1[0..t1_len], "step 1"), true);
    expect_eq(std.mem.eql(u8, c1[0..c1_len], "{\"fn\":\"ls\"}"), true);
    expect_eq(st1.thought_blocks, 1);
    expect_eq(st1.tool_blocks, 1);

    // 2. 1-byte-at-a-time streaming feed must produce bit-identical output and stats
    let a2: [128]u8 = [0; 128];
    let t2: [128]u8 = [0; 128];
    let c2: [128]u8 = [0; 128];
    let a2_len: usize = 0;
    let t2_len: usize = 0;
    let c2_len: usize = 0;
    const st2 = collectDemuxedByChunkSize(full, 1, a2[0..], *a2_len, t2[0..], *t2_len, c2[0..], *c2_len);

    expect_eq(std.mem.eql(u8, a2[0..a2_len], a1[0..a1_len]), true);
    expect_eq(std.mem.eql(u8, t2[0..t2_len], t1[0..t1_len]), true);
    expect_eq(std.mem.eql(u8, c2[0..c2_len], c1[0..c1_len]), true);
    expect_eq(st2.answer_bytes, st1.answer_bytes);
    expect_eq(st2.thinking_bytes, st1.thinking_bytes);
    expect_eq(st2.tool_bytes, st1.tool_bytes);
    expect_eq(st2.thought_blocks, 1);
    expect_eq(st2.tool_blocks, 1);
}

fn testFalsePrefixRollbackAndOverlappingTags()! {
    const tricky = "a <this is ok <t<think>deep</thi!still</think> <<tool_call>run</tool_call> z";

    let a: [128]u8 = [0; 128];
    let t: [128]u8 = [0; 128];
    let c: [128]u8 = [0; 128];
    let a_len: usize = 0;
    let t_len: usize = 0;
    let c_len: usize = 0;

    // Feed 3 bytes at a time to stress prefix rollback across chunk boundaries
    const st = collectDemuxedByChunkSize(tricky, 3, a[0..], *a_len, t[0..], *t_len, c[0..], *c_len);

    expect_eq(std.mem.eql(u8, a[0..a_len], "a <this is ok <t < z"), true);
    expect_eq(std.mem.eql(u8, t[0..t_len], "deep</thi!still"), true);
    expect_eq(std.mem.eql(u8, c[0..c_len], "run"), true);
    expect_eq(st.thought_blocks, 1);
    expect_eq(st.tool_blocks, 1);
}

fn testMultiTurnMixedChannelsAndStats()! {
    const stream =
        "P1" ++
        "<think>T1 mentions <tool_call> literally</think>" ++
        "<tool_call>C1</tool_call>" ++
        "P2" ++
        "<think>T2</think>" ++
        "<tool_call>C2</tool_call>" ++
        "P3";

    let a: [128]u8 = [0; 128];
    let t: [128]u8 = [0; 128];
    let c: [128]u8 = [0; 128];
    let a_len: usize = 0;
    let t_len: usize = 0;
    let c_len: usize = 0;

    const st = collectDemuxedByChunkSize(stream, 5, a[0..], *a_len, t[0..], *t_len, c[0..], *c_len);

    expect_eq(std.mem.eql(u8, a[0..a_len], "P1P2P3"), true);
    expect_eq(std.mem.eql(u8, t[0..t_len], "T1 mentions <tool_call> literallyT2"), true);
    expect_eq(std.mem.eql(u8, c[0..c_len], "C1C2"), true);
    expect_eq(st.thought_blocks, 2);
    expect_eq(st.tool_blocks, 2);
    expect_eq(st.answer_bytes, 6);
    expect_eq(st.thinking_bytes, t_len as u64);
    expect_eq(st.tool_bytes, 4);
}

fn testUnclosedTagFlushAtEof()! {
    let demux = ReasoningStreamDemuxer.init();
    let out_buf: [64]u8 = [0; 64];
    let ev_buf: [4]DemuxEvent = [DemuxEvent{}; 4];

    // 1. Stream ends mid-opening tag "<thi" in Answer channel
    const n1 = demux.feed("abc<thi", out_buf[0..], ev_buf[0..]) ?? 99;
    expect_eq(n1, 1);
    expect_eq(ev_buf[0].channel, StreamChannel.Answer);
    expect_eq(std.mem.eql(u8, out_buf[ev_buf[0].start..(ev_buf[0].start + ev_buf[0].len)], "abc"), true);
    expect_eq(demux.pendingLen(), 4);

    const nf1 = demux.flush(out_buf[0..], ev_buf[0..]) ?? 99;
    expect_eq(nf1, 1);
    expect_eq(ev_buf[0].channel, StreamChannel.Answer);
    expect_eq(std.mem.eql(u8, out_buf[ev_buf[0].start..(ev_buf[0].start + ev_buf[0].len)], "<thi"), true);
    expect_eq(demux.pendingLen(), 0);
    // Second flush is a no-op
    expect_eq(demux.flush(out_buf[0..], ev_buf[0..]), 0);

    // 2. Stream ends mid-closing tag "</thi" inside Thinking channel
    demux.reset();
    const n2 = demux.feed("<think>idea</thi", out_buf[0..], ev_buf[0..]) ?? 99;
    expect_eq(n2, 1);
    expect_eq(ev_buf[0].channel, StreamChannel.Thinking);
    expect_eq(std.mem.eql(u8, out_buf[ev_buf[0].start..(ev_buf[0].start + ev_buf[0].len)], "idea"), true);
    expect_eq(demux.channel(), StreamChannel.Thinking);
    expect_eq(demux.pendingLen(), 5);

    const nf2 = demux.flush(out_buf[0..], ev_buf[0..]) ?? 99;
    expect_eq(nf2, 1);
    expect_eq(ev_buf[0].channel, StreamChannel.Thinking);
    expect_eq(std.mem.eql(u8, out_buf[ev_buf[0].start..(ev_buf[0].start + ev_buf[0].len)], "</thi"), true);
    expect_eq(demux.pendingLen(), 0);
}

fn testEmptyChunkOverflowAndReset()! {
    let demux = ReasoningStreamDemuxer.init();
    let out_buf: [4]u8 = [0; 4];
    let ev_buf: [2]DemuxEvent = [DemuxEvent{}; 2];

    // Empty chunk returns 0 cleanly
    expect_eq(demux.feed("", out_buf[0..], ev_buf[0..]), 0);

    // Output byte buffer too small (4 bytes < 8 bytes) returns null
    expect_eq(demux.feed("12345678", out_buf[0..], ev_buf[0..]) == null, true);

    // Zero-length event slice returns null when bytes need to be emitted
    demux.reset();
    let zero_ev_buf: [0]DemuxEvent = [];
    let zero_ev: []DemuxEvent = zero_ev_buf[0..];
    expect_eq(demux.feed("hi", out_buf[0..], zero_ev) == null, true);

    // After reset(), demuxer works cleanly from Answer state with zeroed stats
    demux.reset();
    expect_eq(demux.channel(), StreamChannel.Answer);
    expect_eq(demux.stats().answer_bytes, 0);
    let ok_buf: [16]u8 = [0; 16];
    const ok_n = demux.feed("ok", ok_buf[0..], ev_buf[0..]) ?? 99;
    expect_eq(ok_n, 1);
    expect_eq(demux.stats().answer_bytes, 2);
}

test("prompt reasoning_stream: whole-chunk and 1-byte streaming tag demux parity", testWholeAndByteByByteTagSplitting);
test("prompt reasoning_stream: false tag prefix rollback and overlapping tag recovery", testFalsePrefixRollbackAndOverlappingTags);
test("prompt reasoning_stream: multi-turn mixed channels, inner literal tags and stats", testMultiTurnMixedChannelsAndStats);
test("prompt reasoning_stream: unclosed partial tag flush at stream EOF", testUnclosedTagFlushAtEof);
test("prompt reasoning_stream: empty chunk, buffer overflow guards and reset reuse", testEmptyChunkOverflowAndReset);
