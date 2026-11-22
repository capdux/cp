// Origin: Pure-CP Stdlib Test Suite (lib/agent/tool_call_assembler.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Streaming LLM Tool-Call Delta Assembler:
//   1. OpenAI-style incremental call_id / tool_name / arguments_json assembly & real-time completion
//   2. Parallel interleaved & sparse out-of-order tool_index tracking + slotAt iteration
//   3. Deep JSON nesting & cross-chunk string escape FSM (braces/quotes/backslashes inside strings)
//   4. Stream truncation detection on finalizeStream (unclosed object/string/empty args) & post-finalize lock
//   5. Malformed JSON guards (bad root, mismatched delimiters, trailing garbage, control char, depth overflow)
//   6. Buffer overflow guards (call_id/name/args limits) & 16-slot capacity saturation (slots_full)
//   7. Anthropic-style content_block_start + input_json_delta & JSON array arguments via cap.agent namespace
//   8. Multi-turn reset() reuse after malformed/finalized stream

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TOOL_CALL_MAX_SLOTS,
    TOOL_CALL_ID_MAX_LEN,
    TOOL_CALL_NAME_MAX_LEN,
    TOOL_CALL_ARGS_MAX_LEN,
    TOOL_CALL_MAX_JSON_DEPTH,
    ToolCallArgsState,
    ToolCallFeedStatus,
    ToolStreamFinalStatus,
    ToolCallDelta,
    AssembledToolCall,
    ToolCallStreamReport,
    ToolCallAssembler,
} from "cap:agent";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: OpenAI Incremental Single Tool-Call Assembly & Real-Time Completion
// ============================================================================

fn testOpenAiIncrementalSingleToolCall()! {
    let assembler = ToolCallAssembler.init();
    expect_eq(assembler.activeCount(), 0);
    expect_eq(assembler.isFinalized(), false);
    expect_eq(assembler.isArgumentsComplete(0), false);

    // Chunk 1: call_id + tool_name + leading whitespace + partial JSON
    const s1 = assembler.feedChunk(0, "call_abc", "get_", "  {\"city\":");
    expect_eq(s1 == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.isArgumentsComplete(0), false);
    expect_eq(assembler.activeCount(), 1);

    // Chunk 2: rest of call_id + rest of tool_name + partial JSON
    const s2 = assembler.feedChunk(0, "_123", "weather", "\"San Fran");
    expect_eq(s2 == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.isArgumentsComplete(0), false);

    // Chunk 3: closing JSON + trailing whitespace
    const s3 = assembler.feedChunk(0, "", "", "cisco\",\"unit\":\"c\"} \n");
    expect_eq(s3 == ToolCallFeedStatus.completed, true);
    expect_eq(assembler.isArgumentsComplete(0), true);

    const tc: AssembledToolCall = assembler.getToolCall(0) ?? return error.MissingToolCall;
    expect_eq(tc.tool_index, 0);
    expect_eq(tc.isArgumentsComplete(), true);
    expect_eq(tc.nestingDepth(), 0);
    expect_eq(tc.delta_count, 3);
    expect_eq(sliceEq(tc.callId(), "call_abc_123"), true);
    expect_eq(sliceEq(tc.toolName(), "get_weather"), true);
    expect_eq(sliceEq(tc.argumentsJson(), "{\"city\":\"San Francisco\",\"unit\":\"c\"}"), true);

    const rep: ToolCallStreamReport = assembler.finalizeStream();
    expect_eq(rep.status == ToolStreamFinalStatus.complete, true);
    expect_eq(rep.total_calls, 1);
    expect_eq(rep.complete_calls, 1);
    expect_eq(rep.truncated_calls, 0);
    expect_eq(rep.malformed_calls, 0);
    expect_eq(rep.overflow_calls, 0);
    expect_eq(rep.delta_chunks_processed, 3);
}

// ============================================================================
// Test 2: Parallel Interleaved & Sparse Out-of-Order tool_index Tracking
// ============================================================================

fn testParallelInterleavedSparseToolCalls()! {
    let assembler = ToolCallAssembler.init();

    // Interleaved chunks arriving out-of-order for sparse tool indices: 5, 0, 2
    expect_eq(assembler.feedChunk(5, "call_5", "search_web", "{\"q\":\"cp ") == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.feedChunk(0, "call_0", "read_file", "{\"path\":") == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.feedChunk(2, "call_2", "run_sql", "{\"sql\":\"SELECT 1\"}") == ToolCallFeedStatus.completed, true);

    expect_eq(assembler.isArgumentsComplete(2), true);
    expect_eq(assembler.isArgumentsComplete(5), false);
    expect_eq(assembler.isArgumentsComplete(0), false);
    expect_eq(assembler.isArgumentsComplete(99), false);

    expect_eq(assembler.feedChunk(0, "", "", "\"main.cp\"}") == ToolCallFeedStatus.completed, true);
    expect_eq(assembler.feedChunk(5, "", "", "lang\",\"limit\":10}") == ToolCallFeedStatus.completed, true);

    expect_eq(assembler.activeCount(), 3);
    expect_eq(assembler.isArgumentsComplete(5), true);
    expect_eq(assembler.isArgumentsComplete(0), true);
    expect_eq(assembler.isArgumentsComplete(2), true);

    const tc5 = assembler.getToolCall(5) ?? return error.MissingSlot5;
    const tc0 = assembler.getToolCall(0) ?? return error.MissingSlot0;
    const tc2 = assembler.getToolCall(2) ?? return error.MissingSlot2;
    expect_eq(sliceEq(tc5.argumentsJson(), "{\"q\":\"cp lang\",\"limit\":10}"), true);
    expect_eq(sliceEq(tc0.argumentsJson(), "{\"path\":\"main.cp\"}"), true);
    expect_eq(sliceEq(tc2.argumentsJson(), "{\"sql\":\"SELECT 1\"}"), true);

    // Verify dense slotAt iteration and OOB null guard
    const first_slot = assembler.slotAt(0) ?? return error.MissingFirstSlot;
    expect_eq(first_slot.tool_index, 5);
    const oob = assembler.slotAt(3);
    expect_eq(oob == null, true);

    const rep = assembler.finalizeStream();
    expect_eq(rep.status == ToolStreamFinalStatus.complete, true);
    expect_eq(rep.total_calls, 3);
    expect_eq(rep.complete_calls, 3);
    expect_eq(rep.delta_chunks_processed, 5);
}

// ============================================================================
// Test 3: Complex Nesting & Cross-Chunk Escaped Quotes/Braces in Strings
// ============================================================================

fn testCrossChunkEscapesAndNestedJson()! {
    let assembler = ToolCallAssembler.init();

    // Braces, brackets, escaped quotes & trailing backslash split across chunk boundary
    expect_eq(assembler.feedChunk(0, "call_esc", "write_code", "{\"code\":\"fn main() { let a = [1,2]; }\\") == ToolCallFeedStatus.ok, true);
    // Previous chunk ended with '\', next chunk starts with '"' -> escaped quote, still inside string!
    expect_eq(assembler.feedChunk(0, "", "", "\"quoted\\\"\\\\\",") == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.isArgumentsComplete(0), false);

    // Nested arrays and objects
    expect_eq(assembler.feedChunk(0, "", "", "\"meta\":{\"tags\":[\"a\",\"b\"],\"nested\":{\"ok\":true}}") == ToolCallFeedStatus.ok, true);
    const mid = assembler.getToolCall(0) ?? return error.MissingMid;
    expect_eq(mid.nestingDepth(), 1);
    expect_eq(assembler.isArgumentsComplete(0), false);

    // Final closing brace
    expect_eq(assembler.feedChunk(0, "", "", "}") == ToolCallFeedStatus.completed, true);
    expect_eq(assembler.isArgumentsComplete(0), true);

    const rep = assembler.finalizeStream();
    expect_eq(rep.status == ToolStreamFinalStatus.complete, true);
    expect_eq(rep.complete_calls, 1);
}

// ============================================================================
// Test 4: Stream Truncation Detection & Post-Finalize Lock
// ============================================================================

fn testStreamTruncationAndFinalizeLock()! {
    let assembler = ToolCallAssembler.init();

    // Tool 0: complete
    expect_eq(assembler.feedChunk(0, "c0", "ping", "{}") == ToolCallFeedStatus.completed, true);
    // Tool 1: truncated mid-object
    expect_eq(assembler.feedChunk(1, "c1", "fetch", "{\"url\":\"https://example.com\"") == ToolCallFeedStatus.ok, true);
    // Tool 2: truncated mid-string
    expect_eq(assembler.feedChunk(2, "c2", "echo", "{\"msg\":\"unterminated") == ToolCallFeedStatus.ok, true);
    // Tool 3: header received, no JSON arguments ever arrived (empty -> truncated on finalize)
    expect_eq(assembler.feedChunk(3, "c3", "noop", "") == ToolCallFeedStatus.ok, true);

    const rep = assembler.finalizeStream();
    expect_eq(assembler.isFinalized(), true);
    expect_eq(rep.status == ToolStreamFinalStatus.truncated, true);
    expect_eq(rep.total_calls, 4);
    expect_eq(rep.complete_calls, 1);
    expect_eq(rep.truncated_calls, 3);
    expect_eq(rep.malformed_calls, 0);

    const st1 = assembler.toolState(1) ?? return error.MissingState1;
    const st2 = assembler.toolState(2) ?? return error.MissingState2;
    const st3 = assembler.toolState(3) ?? return error.MissingState3;
    expect_eq(st1 == ToolCallArgsState.truncated, true);
    expect_eq(st2 == ToolCallArgsState.truncated, true);
    expect_eq(st3 == ToolCallArgsState.truncated, true);

    // Feeding after finalizeStream must be rejected with stream_finalized
    expect_eq(assembler.feedChunk(1, "", "", "}") == ToolCallFeedStatus.stream_finalized, true);
    expect_eq(assembler.feedChunk(4, "c4", "late", "{}") == ToolCallFeedStatus.stream_finalized, true);
}

// ============================================================================
// Test 5: Malformed JSON Guards (Bad Root, Mismatch, Trailing, Control, Depth)
// ============================================================================

fn testMalformedJsonGuards()! {
    let assembler = ToolCallAssembler.init();

    // Slot 0: invalid root token (not '{' or '[')
    expect_eq(assembler.feedChunk(0, "m0", "t0", "123") == ToolCallFeedStatus.malformed, true);
    // Sticky malformed on subsequent chunks
    expect_eq(assembler.feedChunk(0, "", "", "{}") == ToolCallFeedStatus.malformed, true);

    // Slot 1: mismatched closing delimiter ({])
    expect_eq(assembler.feedChunk(1, "m1", "t1", "{\"a\":[1,2}") == ToolCallFeedStatus.malformed, true);

    // Slot 2: trailing non-whitespace garbage after balanced JSON closed
    expect_eq(assembler.feedChunk(2, "m2", "t2", "{\"ok\":true}") == ToolCallFeedStatus.completed, true);
    expect_eq(assembler.feedChunk(2, "", "", " extra") == ToolCallFeedStatus.malformed, true);
    expect_eq(assembler.isArgumentsComplete(2), false);

    // Slot 3: unescaped ASCII control char (0x07) inside JSON string
    let bad_str: [12]u8 = [ '{', '"', 'a', '"', ':', '"', 0x07, '"', '}', 0, 0, 0 ];
    expect_eq(assembler.feedChunk(3, "m3", "t3", bad_str[0..9]) == ToolCallFeedStatus.malformed, true);

    // Slot 4: nesting depth exceeds TOOL_CALL_MAX_JSON_DEPTH (32)
    let deep: [40]u8 = ['['; 40];
    expect_eq(assembler.feedChunk(4, "m4", "t4", deep[0..TOOL_CALL_MAX_JSON_DEPTH + 1]) == ToolCallFeedStatus.malformed, true);

    const rep = assembler.finalizeStream();
    expect_eq(rep.status == ToolStreamFinalStatus.malformed, true);
    expect_eq(rep.total_calls, 5);
    expect_eq(rep.malformed_calls, 5);
    expect_eq(rep.complete_calls, 0);
}

// ============================================================================
// Test 6: Buffer Overflow Guards & 16-Slot Capacity Saturation
// ============================================================================

fn testBufferOverflowAndSlotCapacityGuards()! {
    let assembler = ToolCallAssembler.init();

    // 1. call_id overflow (> 64 bytes)
    let big_id: [TOOL_CALL_ID_MAX_LEN + 4]u8 = ['x'; TOOL_CALL_ID_MAX_LEN + 4];
    expect_eq(assembler.feedChunk(0, big_id[0..], "t", "{}") == ToolCallFeedStatus.overflow, true);

    // 2. tool_name overflow (> 64 bytes)
    let big_name: [TOOL_CALL_NAME_MAX_LEN + 4]u8 = ['n'; TOOL_CALL_NAME_MAX_LEN + 4];
    expect_eq(assembler.feedChunk(1, "id1", big_name[0..], "{}") == ToolCallFeedStatus.overflow, true);

    // 3. arguments_json overflow (> 1024 bytes)
    expect_eq(assembler.feedChunk(2, "id2", "t2", "{\"k\":\"") == ToolCallFeedStatus.ok, true);
    let pad: [256]u8 = ['a'; 256];
    expect_eq(assembler.feedChunk(2, "", "", pad[0..]) == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.feedChunk(2, "", "", pad[0..]) == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.feedChunk(2, "", "", pad[0..]) == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.feedChunk(2, "", "", pad[0..]) == ToolCallFeedStatus.overflow, true);

    // 4. Fill remaining slots up to TOOL_CALL_MAX_SLOTS (16)
    let k: u16 = 3;
    while (k < (TOOL_CALL_MAX_SLOTS as u16)) : (k += 1) {
        expect_eq(assembler.feedChunk(k, "id", "fn", "{}") == ToolCallFeedStatus.completed, true);
    }
    expect_eq(assembler.activeCount(), TOOL_CALL_MAX_SLOTS);

    // 17th distinct tool_index must be rejected with slots_full
    expect_eq(assembler.feedChunk(100, "id_extra", "extra", "{}") == ToolCallFeedStatus.slots_full, true);

    const rep = assembler.finalizeStream();
    expect_eq(rep.status == ToolStreamFinalStatus.malformed, true);
    expect_eq(rep.total_calls, TOOL_CALL_MAX_SLOTS);
    expect_eq(rep.overflow_calls, 3);
    expect_eq(rep.complete_calls, TOOL_CALL_MAX_SLOTS - 3);
}

// ============================================================================
// Test 7: Anthropic-Style Deltas & JSON Array Root via cap.agent Namespace
// ============================================================================

fn testAnthropicStyleAndArrayArgsViaNamespace()! {
    let assembler = cap.agent.ToolCallAssembler.init();

    // Anthropic content_block_start: id + name, empty input_json
    const d0 = ToolCallDelta{
        tool_index: 1,
        call_id_delta: "toolu_01A09q90qw90lq917835lq9",
        name_delta: "batch_lookup",
        arguments_delta: "",
    };
    expect_eq(assembler.feed(d0) == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.isArgumentsComplete(1), false);

    // Anthropic input_json_delta chunks with JSON array root
    expect_eq(assembler.feed(ToolCallDelta{ tool_index: 1, arguments_delta: "[{\"id\":1}," }) == ToolCallFeedStatus.ok, true);
    expect_eq(assembler.feed(ToolCallDelta{ tool_index: 1, arguments_delta: "{\"id\":2}]" }) == ToolCallFeedStatus.completed, true);

    const tc = assembler.getToolCall(1) ?? return error.MissingAnthropicCall;
    expect_eq(sliceEq(tc.callId(), "toolu_01A09q90qw90lq917835lq9"), true);
    expect_eq(sliceEq(tc.toolName(), "batch_lookup"), true);
    expect_eq(sliceEq(tc.argumentsJson(), "[{\"id\":1},{\"id\":2}]"), true);

    const rep = assembler.finalizeStream();
    expect_eq(rep.status == ToolStreamFinalStatus.complete, true);
    expect_eq(rep.total_calls, 1);
    expect_eq(rep.complete_calls, 1);
}

// ============================================================================
// Test 8: Multi-Turn reset() Reuse After Malformed/Finalized Stream
// ============================================================================

fn testMultiTurnResetReuse()! {
    let assembler = ToolCallAssembler.init();

    // Turn 1: malformed & finalized
    expect_eq(assembler.feedChunk(0, "bad_id", "bad_fn", "{]") == ToolCallFeedStatus.malformed, true);
    const r1 = assembler.finalizeStream();
    expect_eq(r1.status == ToolStreamFinalStatus.malformed, true);
    expect_eq(assembler.isFinalized(), true);

    // Reset for Turn 2
    assembler.reset();
    expect_eq(assembler.activeCount(), 0);
    expect_eq(assembler.isFinalized(), false);
    expect_eq(assembler.getToolCall(0) == null, true);

    // Turn 2: clean completion
    expect_eq(assembler.feedChunk(0, "good_id", "calc", "{\"x\":42}") == ToolCallFeedStatus.completed, true);
    const r2 = assembler.finalizeStream();
    expect_eq(r2.status == ToolStreamFinalStatus.complete, true);
    expect_eq(r2.total_calls, 1);
    expect_eq(r2.complete_calls, 1);
    expect_eq(r2.malformed_calls, 0);
    expect_eq(r2.delta_chunks_processed, 1);

    const tc = assembler.getToolCall(0) ?? return error.MissingTurn2Call;
    expect_eq(sliceEq(tc.callId(), "good_id"), true);
    expect_eq(sliceEq(tc.toolName(), "calc"), true);
    expect_eq(sliceEq(tc.argumentsJson(), "{\"x\":42}"), true);
}

// ============================================================================
// Test Entrypoints
// ============================================================================

test("tool_call_assembler: OpenAI incremental single tool-call assembly & real-time completion", testOpenAiIncrementalSingleToolCall);
test("tool_call_assembler: parallel interleaved & sparse out-of-order tool_index tracking", testParallelInterleavedSparseToolCalls);
test("tool_call_assembler: complex nesting & cross-chunk escaped quotes/braces in strings", testCrossChunkEscapesAndNestedJson);
test("tool_call_assembler: stream truncation detection & post-finalize lock", testStreamTruncationAndFinalizeLock);
test("tool_call_assembler: malformed JSON guards (bad root, mismatch, trailing, control, depth)", testMalformedJsonGuards);
test("tool_call_assembler: buffer overflow guards & 16-slot capacity saturation", testBufferOverflowAndSlotCapacityGuards);
test("tool_call_assembler: Anthropic-style deltas & JSON array args via cap.agent namespace", testAnthropicStyleAndArrayArgsViaNamespace);
test("tool_call_assembler: multi-turn reset() reuse after malformed/finalized stream", testMultiTurnResetReuse);
