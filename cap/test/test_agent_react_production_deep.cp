// Source: cp/cap/test/test_agent_react_production_deep.cp (Production deep tests for cap:agent ReAct & checkpoint)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CHECKPOINT_MAGIC,
    ReactPhase,
    StepKind,
    TraceEntry,
    ReactSession,
    saveCheckpoint,
    loadCheckpoint,
} from "cap:agent";

test("1. Full ReAct multi-step lifecycle Idle -> Thinking -> AwaitingTool -> Observing -> Thinking -> Completed", () => {
    let trace_arr: [16]TraceEntry = std.mem.zeroes([16]TraceEntry);
    let trace_s: []TraceEntry = trace_arr[0..];

    let sess = ReactSession.init(1001, 5, &trace_s);
    expect_eq(sess.phase() == ReactPhase.Idle, true);
    expect_eq(sess.stepCount(), 0);
    expect_eq(sess.isTerminal(), false);
    expect_eq(sess.traceLen(), 0);

    // Idle -> Thinking
    expect_eq(sess.startTurn(1000), true);
    expect_eq(sess.phase() == ReactPhase.Thinking, true);

    // Step 1: Thought -> ToolCall -> ToolResult
    expect_eq(sess.recordThought(250, 40, 1040), true);
    expect_eq(sess.phase() == ReactPhase.Thinking, true);

    expect_eq(sess.requestToolCall(0x11112222, 1050), true);
    expect_eq(sess.phase() == ReactPhase.AwaitingTool, true);
    expect_eq(sess.stepCount(), 1);

    expect_eq(sess.recordToolResult(0x11112222, true, 120, 1170), true);
    expect_eq(sess.phase() == ReactPhase.Observing, true);

    // Step 2: Observing -> Thinking (via recordThought) -> ToolCall -> ToolResult
    expect_eq(sess.recordThought(180, 30, 1200), true);
    expect_eq(sess.phase() == ReactPhase.Thinking, true);

    expect_eq(sess.requestToolCall(0x33334444, 1210), true);
    expect_eq(sess.phase() == ReactPhase.AwaitingTool, true);
    expect_eq(sess.stepCount(), 2);

    expect_eq(sess.recordToolResult(0x33334444, false, 85, 1295), true);
    expect_eq(sess.phase() == ReactPhase.Observing, true);

    // Finalize: Observing -> Thinking -> Completed
    expect_eq(sess.recordThought(120, 20, 1315), true);
    expect_eq(sess.finishWithAnswer(300, 1350), true);
    expect_eq(sess.phase() == ReactPhase.Completed, true);
    expect_eq(sess.isTerminal(), true);
    expect_eq(sess.total_tokens, 850);
    expect_eq(sess.traceLen(), 8);

    const e0 = sess.traceAt(0) ?? unreachable;
    expect_eq(e0.kind == StepKind.Thought, true);
    expect_eq(e0.tokens_used, 250);
    expect_eq(e0.timestamp_ms, 1040);

    const e5 = sess.traceAt(5) ?? unreachable;
    expect_eq(e5.kind == StepKind.ToolResult, true);
    expect_eq(e5.tool_hash, 0x33334444);
    expect_eq(e5.ok, false);
    expect_eq(e5.duration_ms, 85);

    const e7 = sess.traceAt(7) ?? unreachable;
    expect_eq(e7.kind == StepKind.FinalAnswer, true);
    expect_eq(e7.tokens_used, 300);
});

test("2. Illegal phase transitions are rejected and terminal phases are immutable", () => {
    let trace_arr: [8]TraceEntry = std.mem.zeroes([8]TraceEntry);
    let trace_s: []TraceEntry = trace_arr[0..];
    let sess = ReactSession.init(2002, 4, &trace_s);

    // Idle rejects thought, tool call, tool result, and finish
    expect_eq(sess.recordThought(10, 5, 100), false);
    expect_eq(sess.requestToolCall(0xAA, 100), false);
    expect_eq(sess.recordToolResult(0xAA, true, 10, 100), false);
    expect_eq(sess.finishWithAnswer(10, 100), false);
    expect_eq(sess.phase() == ReactPhase.Idle, true);

    // Thinking rejects double startTurn and recordToolResult
    expect_eq(sess.startTurn(100), true);
    expect_eq(sess.startTurn(101), false);
    expect_eq(sess.recordToolResult(0xAA, true, 10, 102), false);

    // AwaitingTool rejects startTurn, thought, nested tool call, and finish
    expect_eq(sess.requestToolCall(0xAA, 105), true);
    expect_eq(sess.startTurn(106), false);
    expect_eq(sess.recordThought(10, 5, 107), false);
    expect_eq(sess.requestToolCall(0xBB, 108), false);
    expect_eq(sess.finishWithAnswer(10, 109), false);

    // Fail transitions to terminal Failed; all subsequent operations rejected
    expect_eq(sess.failSession(110), true);
    expect_eq(sess.phase() == ReactPhase.Failed, true);
    expect_eq(sess.isTerminal(), true);
    expect_eq(sess.recordToolResult(0xAA, true, 10, 111), false);
    expect_eq(sess.failSession(112), false);
    expect_eq(sess.haltSession(113), false);
});

test("3. max_steps hard limit transitions session to Halted on overflow", () => {
    let trace_arr: [8]TraceEntry = std.mem.zeroes([8]TraceEntry);
    let trace_s: []TraceEntry = trace_arr[0..];
    let sess = ReactSession.init(3003, 2, &trace_s);

    expect_eq(sess.startTurn(100), true);

    // Tool call #1 (allowed)
    expect_eq(sess.requestToolCall(1, 110), true);
    expect_eq(sess.recordToolResult(1, true, 10, 120), true);

    // Tool call #2 (allowed directly from Observing)
    expect_eq(sess.requestToolCall(2, 130), true);
    expect_eq(sess.recordToolResult(2, true, 10, 140), true);
    expect_eq(sess.stepCount(), 2);

    // Tool call #3 exceeds max_steps = 2 -> Halted!
    expect_eq(sess.requestToolCall(3, 150), false);
    expect_eq(sess.phase() == ReactPhase.Halted, true);
    expect_eq(sess.isTerminal(), true);
    expect_eq(sess.stepCount(), 2);

    const last = sess.traceAt(sess.traceLen() - 1) ?? unreachable;
    expect_eq(last.kind == StepKind.ErrorNote, true);
    expect_eq(last.ok, false);
});

test("4. Fixed ring buffer overflow retains the latest traces in chronological order", () => {
    let trace_arr: [3]TraceEntry = std.mem.zeroes([3]TraceEntry);
    let trace_s: []TraceEntry = trace_arr[0..];
    let sess = ReactSession.init(4004, 10, &trace_s);

    expect_eq(sess.startTurn(0), true);
    // Push 6 thoughts with distinct token counts 10..60
    expect_eq(sess.recordThought(10, 1, 100), true);
    expect_eq(sess.recordThought(20, 2, 200), true);
    expect_eq(sess.recordThought(30, 3, 300), true);
    expect_eq(sess.recordThought(40, 4, 400), true);
    expect_eq(sess.recordThought(50, 5, 500), true);
    expect_eq(sess.recordThought(60, 6, 600), true);

    expect_eq(sess.traceLen(), 3);
    expect_eq((sess.traceAt(0) ?? unreachable).tokens_used, 40);
    expect_eq((sess.traceAt(1) ?? unreachable).tokens_used, 50);
    expect_eq((sess.traceAt(2) ?? unreachable).tokens_used, 60);
    expect_eq(sess.traceAt(3) == null, true);
    expect_eq(sess.total_tokens, 210);
});

test("5. saveCheckpoint and loadCheckpoint losslessly resume an AwaitingTool session across restart", () => {
    // Use ring capacity 3 and wrap around before checkpointing to test chronological serialization
    let trace_arr: [3]TraceEntry = std.mem.zeroes([3]TraceEntry);
    let trace_s: []TraceEntry = trace_arr[0..];
    let sess = ReactSession.init(0xDEADBEEF12345678, 6, &trace_s);

    expect_eq(sess.startTurn(-500), true);
    expect_eq(sess.recordThought(100, 15, -400), true);
    expect_eq(sess.requestToolCall(0xAA01, -300), true);
    expect_eq(sess.recordToolResult(0xAA01, true, 50, -250), true);
    expect_eq(sess.recordThought(200, 25, 1000), true);
    expect_eq(sess.requestToolCall(0xBB02, 1050), true);
    // Currently suspended in AwaitingTool; ring wrapped and holds: [ToolResult(-250), Thought(1000), ToolCall(1050)]
    expect_eq(sess.phase() == ReactPhase.AwaitingTool, true);
    expect_eq(sess.traceLen(), 3);

    let ckpt_buf: [512]u8 = std.mem.zeroes([512]u8);
    let ckpt_s: []u8 = ckpt_buf[0..];
    const written = saveCheckpoint(sess, &ckpt_s) ?? unreachable;
    expect_eq(written, 44 + 3 * 30);

    // Verify magic bytes LE
    const magic_le: u32 = (ckpt_s[0] as u32) |
        ((ckpt_s[1] as u32) << 8) |
        ((ckpt_s[2] as u32) << 16) |
        ((ckpt_s[3] as u32) << 24);
    expect_eq(magic_le, CHECKPOINT_MAGIC);

    // Restore into a fresh session buffer and continue to Completed
    let restored_arr: [8]TraceEntry = std.mem.zeroes([8]TraceEntry);
    let restored_s: []TraceEntry = restored_arr[0..];
    let resumed = loadCheckpoint(ckpt_s[0..written], &restored_s) ?? unreachable;

    expect_eq(resumed.session_id, 0xDEADBEEF12345678);
    expect_eq(resumed.phase() == ReactPhase.AwaitingTool, true);
    expect_eq(resumed.stepCount(), 2);
    expect_eq(resumed.max_steps, 6);
    expect_eq(resumed.total_tokens, 300);
    expect_eq(resumed.traceLen(), 3);

    // Check chronological order & negative timestamp round-trip
    const r0 = resumed.traceAt(0) ?? unreachable;
    expect_eq(r0.kind == StepKind.ToolResult, true);
    expect_eq(r0.tool_hash, 0xAA01);
    expect_eq(r0.timestamp_ms, -250);

    const r2 = resumed.traceAt(2) ?? unreachable;
    expect_eq(r2.kind == StepKind.ToolCall, true);
    expect_eq(r2.tool_hash, 0xBB02);
    expect_eq(r2.timestamp_ms, 1050);

    // Continue execution from AwaitingTool -> Observing -> Thinking -> Completed
    expect_eq(resumed.recordToolResult(0xBB02, true, 75, 1125), true);
    expect_eq(resumed.recordThought(90, 10, 1135), true);
    expect_eq(resumed.finishWithAnswer(150, 1200), true);
    expect_eq(resumed.phase() == ReactPhase.Completed, true);
    expect_eq(resumed.total_tokens, 540);
    expect_eq(resumed.traceLen(), 6);
});

test("6. Checkpoint rejects any 1-byte tamper, truncation, empty input, and undersized buffers", () => {
    let trace_arr: [4]TraceEntry = std.mem.zeroes([4]TraceEntry);
    let trace_s: []TraceEntry = trace_arr[0..];
    let sess = ReactSession.init(777, 4, &trace_s);
    expect_eq(sess.startTurn(10), true);
    expect_eq(sess.recordThought(50, 5, 15), true);
    expect_eq(sess.requestToolCall(0xCAFE, 20), true);

    // Undersized output buffer rejected
    let small_out: [32]u8 = std.mem.zeroes([32]u8);
    let small_out_s: []u8 = small_out[0..];
    expect_eq(saveCheckpoint(sess, &small_out_s) == null, true);

    let ckpt_buf: [256]u8 = std.mem.zeroes([256]u8);
    let ckpt_s: []u8 = ckpt_buf[0..];
    const written = saveCheckpoint(sess, &ckpt_s) ?? unreachable;

    let restored_arr: [4]TraceEntry = std.mem.zeroes([4]TraceEntry);
    let restored_s: []TraceEntry = restored_arr[0..];

    // Empty & truncated inputs rejected
    const empty_u8: [0]u8 = [];
    expect_eq(loadCheckpoint(empty_u8[0..], &restored_s) == null, true);
    expect_eq(loadCheckpoint(ckpt_s[0..(written - 1)], &restored_s) == null, true);

    // Undersized restore trace_buf rejected (needs 2 entries, given 1)
    let tiny_trace: [1]TraceEntry = std.mem.zeroes([1]TraceEntry);
    let tiny_trace_s: []TraceEntry = tiny_trace[0..];
    expect_eq(loadCheckpoint(ckpt_s[0..written], &tiny_trace_s) == null, true);

    // Tamper 1 byte in header (session_id) -> checksum mismatch
    ckpt_s[10] ^= 0x01;
    expect_eq(loadCheckpoint(ckpt_s[0..written], &restored_s) == null, true);
    ckpt_s[10] ^= 0x01;

    // Tamper 1 byte in trace payload -> checksum mismatch
    ckpt_s[42] ^= 0x80;
    expect_eq(loadCheckpoint(ckpt_s[0..written], &restored_s) == null, true);
    ckpt_s[42] ^= 0x80;

    // Tamper 1 byte in checksum footer -> checksum mismatch
    ckpt_s[written - 1] ^= 0x01;
    expect_eq(loadCheckpoint(ckpt_s[0..written], &restored_s) == null, true);
    ckpt_s[written - 1] ^= 0x01;

    // Restoring untampered buffer succeeds
    expect_eq(loadCheckpoint(ckpt_s[0..written], &restored_s) != null, true);
});
