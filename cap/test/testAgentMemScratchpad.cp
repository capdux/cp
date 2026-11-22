// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/scratchpad.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Agent ReAct / Plan-Execute Structured Scratchpad:
//   1. Full ReAct TAO trajectory appending, parent links, pinning, truncation & facade parity
//   2. Checkpoint creation, nested branch rollback, repeated retries & rollback telemetry
//   3. Sliding-window repeated action loop detection (`detectRepeatedAction`)
//   4. Token-budget priority rendering (`pinned`/`milestone` first, recency backfill, omission markers)
//   5. Tight token budget, zero-step render & insufficient output buffer boundary guards
//   6. Full capacity boundaries (64 steps & 8 checkpoints) with zero-side-effect refusal
//   7. Telemetry snapshot (`stats()`) and full clean reset (`clear()`)

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SCRATCHPAD_MAX_STEPS,
    SCRATCHPAD_MAX_CHECKPOINTS,
    SCRATCHPAD_TEXT_MAX_LEN,
    SCRATCHPAD_TAG_MAX_LEN,
    SCRATCHPAD_NO_STEP,
    ScratchStepKind,
    formatScratchStepKind,
    AgentScratchpad,
} from "cap:agentmem";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

fn indexOfSubstr(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) return 0;
    if (needle.len > haystack.len) return null;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return i;
    }
    return null;
}

test("AgentScratchpad: full ReAct trajectory, parent links, pinning, truncation & facade parity") {
    let pad = AgentScratchpad.init();
    expect_eq(pad.len(), @as(usize, 0));

    // Verify kind labels
    expect_true(bytesEq(formatScratchStepKind(ScratchStepKind.thought), "Thought"));
    expect_true(bytesEq(formatScratchStepKind(ScratchStepKind.action), "Action"));
    expect_true(bytesEq(formatScratchStepKind(ScratchStepKind.observation), "Observation"));
    expect_true(bytesEq(formatScratchStepKind(ScratchStepKind.reflection), "Reflection"));
    expect_true(bytesEq(formatScratchStepKind(ScratchStepKind.milestone), "Milestone"));
    expect_true(bytesEq(formatScratchStepKind(ScratchStepKind.step_error), "Error"));

    const s1 = pad.appendStep(ScratchStepKind.thought, "", "Plan how to fix query timeout", 18, 1000) ?? 0;
    const s2 = pad.appendStep(ScratchStepKind.action, "sql_explain", "EXPLAIN SELECT * FROM events", 22, 1100) ?? 0;
    const s3 = pad.appendStep(ScratchStepKind.observation, "sql_explain", "Full table scan on 10M rows", 25, 1200) ?? 0;
    const s4 = pad.appendStep(ScratchStepKind.milestone, "index_plan", "Add composite index on (tenant_id, ts)", 30, 1300) ?? 0;

    expect_eq(s1, @as(u32, 1));
    expect_eq(s2, @as(u32, 2));
    expect_eq(s3, @as(u32, 3));
    expect_eq(s4, @as(u32, 4));
    expect_eq(pad.len(), @as(usize, 4));

    // Verify parent linkage & content accessors
    if (st1 := pad.getStep(s1)) {
        expect_eq(st1.parent_step_id, SCRATCHPAD_NO_STEP);
        expect_eq(st1.timestamp_ms, @as(u64, 1000));
        expect_eq(st1.tag().len, @as(usize, 0));
        expect_true(bytesEq(st1.content(), "Plan how to fix query timeout"));
        expect_true(!st1.isPriority());
    } else {
        expect_true(false);
    }

    if (st4 := pad.getStep(s4)) {
        expect_eq(st4.parent_step_id, s3);
        expect_eq(st4.kind, ScratchStepKind.milestone);
        expect_true(st4.isPriority());
        expect_true(bytesEq(st4.tag(), "index_plan"));
    } else {
        expect_true(false);
    }

    // Pinning and unpinning
    expect_true(pad.pinStep(s1, true));
    expect_true(!pad.pinStep(999, true));
    expect_true(!pad.pinStep(SCRATCHPAD_NO_STEP, true));
    if (st1_pinned := pad.getStep(s1)) {
        expect_true(st1_pinned.pinned);
        expect_true(st1_pinned.isPriority());
    } else {
        expect_true(false);
    }
    expect_true(pad.pinStep(s1, false));
    if (st1_unpinned := pad.getStep(s1)) {
        expect_true(!st1_unpinned.pinned);
    } else {
        expect_true(false);
    }

    // Oversized tag (> 40B) and oversized content (> 192B) safely truncate
    let big_tag: [60]u8 = ['T'; 60];
    let big_txt: [240]u8 = ['X'; 240];
    const s5 = pad.appendStep(ScratchStepKind.step_error, big_tag[0..], big_txt[0..], 50, 1400) ?? 0;
    if (st5 := pad.getStep(s5)) {
        expect_eq(st5.tag().len, SCRATCHPAD_TAG_MAX_LEN);
        expect_eq(st5.content().len, SCRATCHPAD_TEXT_MAX_LEN);
        expect_true(st5.tag_truncated);
        expect_true(st5.content_truncated);
    } else {
        expect_true(false);
    }

    // Verify cap.agentmem facade parity
    let facade_pad = cap.agentmem.AgentScratchpad.init();
    expect_eq(facade_pad.len(), @as(usize, 0));
    * = facade_pad.appendStep(cap.agentmem.ScratchStepKind.thought, "init", "hello", 5, 10);
    expect_eq(facade_pad.len(), @as(usize, 1));
}

test("AgentScratchpad: checkpoint save, nested branch rollback & retry telemetry") {
    let pad = AgentScratchpad.init();
    const s1 = pad.appendStep(ScratchStepKind.milestone, "start", "Baseline passing", 20, 100) ?? 0;
    const cp1 = pad.saveCheckpoint("baseline") ?? 0;
    expect_eq(cp1, @as(u32, 1));

    if (cp_info := pad.getCheckpoint(cp1)) {
        expect_eq(cp_info.step_count, @as(usize, 1));
        expect_eq(cp_info.last_step_id, s1);
        expect_true(bytesEq(cp_info.label(), "baseline"));
    } else {
        expect_true(false);
    }

    // Explore branch A with a nested checkpoint
    const s2 = pad.appendStep(ScratchStepKind.action, "edit_file", "Try aggressive refactor", 25, 200) ?? 0;
    const cp2 = pad.saveCheckpoint("after_refactor") ?? 0;
    expect_eq(cp2, @as(u32, 2));
    const s3 = pad.appendStep(ScratchStepKind.step_error, "build", "Compile error in module B", 30, 300) ?? 0;
    expect_eq(pad.len(), @as(usize, 3));
    * = s2;
    * = s3;

    // Invalid checkpoint rollback returns 0 with no side effects
    expect_eq(pad.rollbackToCheckpoint(99), @as(usize, 0));
    expect_eq(pad.rollbackToCheckpoint(0), @as(usize, 0));
    expect_eq(pad.len(), @as(usize, 3));

    // Roll back to cp1: removes 2 steps (s2, s3) and invalidates nested cp2
    const removed1 = pad.rollbackToCheckpoint(cp1);
    expect_eq(removed1, @as(usize, 2));
    expect_eq(pad.len(), @as(usize, 1));
    expect_true(pad.getStep(s2) == null);
    expect_true(pad.getStep(s3) == null);
    expect_true(pad.getCheckpoint(cp2) == null);
    expect_true(pad.getCheckpoint(cp1) != null);

    // Retry branch B: monotonic step_id continues (4), parent links cleanly back to s1
    const s4 = pad.appendStep(ScratchStepKind.action, "edit_file", "Minimal safe fix", 15, 400) ?? 0;
    expect_eq(s4, @as(u32, 4));
    if (st4 := pad.getStep(s4)) {
        expect_eq(st4.parent_step_id, s1);
    } else {
        expect_true(false);
    }

    // Second rollback to the same cp1 works cleanly and accumulates rollback counters
    const removed2 = pad.rollbackToCheckpoint(cp1);
    expect_eq(removed2, @as(usize, 1));
    expect_eq(pad.len(), @as(usize, 1));

    const st = pad.stats();
    expect_eq(st.total_rollbacks, @as(u64, 2));
    expect_eq(st.total_rolled_back_steps, @as(u64, 3));
    expect_eq(st.total_appended, @as(u64, 4));
    expect_eq(st.checkpoint_count, @as(usize, 1));
}

test("AgentScratchpad: sliding-window repeated action loop detection") {
    let pad = AgentScratchpad.init();
    expect_eq(pad.detectRepeatedAction("grep", 5), @as(usize, 0));

    * = pad.appendStep(ScratchStepKind.action, "grep", "search foo", 10, 100);
    * = pad.appendStep(ScratchStepKind.observation, "grep", "0 matches", 10, 110); // not an action!
    * = pad.appendStep(ScratchStepKind.action, "read_file", "open a.cp", 10, 120);
    * = pad.appendStep(ScratchStepKind.action, "grep", "search foo again", 10, 130);
    * = pad.appendStep(ScratchStepKind.thought, "grep", "maybe try different flag", 10, 140); // not an action!
    * = pad.appendStep(ScratchStepKind.action, "grep", "search foo third time", 10, 150);

    // Full history (window >= 6): 3 "grep" actions (observation & thought ignored)
    expect_eq(pad.detectRepeatedAction("grep", 10), @as(usize, 3));
    expect_eq(pad.detectRepeatedAction("read_file", 10), @as(usize, 1));

    // Recent window of 4 steps: indices [2, 3, 4, 5] -> 2 "grep" actions, 1 "read_file"
    expect_eq(pad.detectRepeatedAction("grep", 4), @as(usize, 2));
    expect_eq(pad.detectRepeatedAction("read_file", 4), @as(usize, 1));

    // Recent window of 2 steps: indices [4, 5] -> 1 "grep" action, 0 "read_file"
    expect_eq(pad.detectRepeatedAction("grep", 2), @as(usize, 1));
    expect_eq(pad.detectRepeatedAction("read_file", 2), @as(usize, 0));

    // Edge cases: window_size == 0 or empty tag
    expect_eq(pad.detectRepeatedAction("grep", 0), @as(usize, 0));
    expect_eq(pad.detectRepeatedAction("", 10), @as(usize, 0));
}

test("AgentScratchpad: token-budget priority rendering preserves pinned & milestone + recent steps") {
    let pad = AgentScratchpad.init();

    // 1. Pinned initial thought (20 tok)
    const s1 = pad.appendStep(ScratchStepKind.thought, "", "Root objective: optimize cache", 20, 100) ?? 0;
    * = pad.pinStep(s1, true);
    // 2. Old unpinned action (30 tok) -> should be omitted under budget=85
    * = pad.appendStep(ScratchStepKind.action, "bench", "Run cold benchmark #1", 30, 200);
    // 3. Old unpinned observation (30 tok) -> should be omitted under budget=85
    * = pad.appendStep(ScratchStepKind.observation, "bench", "Latency 120ms", 30, 300);
    // 4. Milestone (25 tok) -> priority retained!
    * = pad.appendStep(ScratchStepKind.milestone, "bottleneck", "Found lock contention in LRU", 25, 400);
    // 5. Middle unpinned thought (25 tok) -> omitted under budget=85 (remaining after s1+s4+s6 is 20 < 25)
    * = pad.appendStep(ScratchStepKind.thought, "", "Consider sharded buckets", 25, 500);
    // 6. Most recent action (20 tok) -> retained by recency backfill!
    * = pad.appendStep(ScratchStepKind.action, "patch", "Implement 16-shard cache", 20, 600);

    // Budget = 85 tokens:
    //   Priority pass selects s1 (20) + s4 (25) = 45 tokens.
    //   Recency pass selects s6 (20) -> 65 tokens; s5 (25) would be 90 > 85 (skipped);
    //   s3 (30) -> 95 > 85 (skipped); s2 (30) -> 95 > 85 (skipped).
    //   Chronological output: s1, omit(2), s4, omit(1), s6.
    let buf: [1024]u8 = [0; 1024];
    let buf_s: []u8 = buf[0..];
    const rep = pad.renderWithinTokenBudget(85, *buf_s);

    expect_eq(rep.rendered_steps, @as(usize, 3));
    expect_eq(rep.omitted_steps, @as(usize, 3));
    expect_eq(rep.pinned_rendered, @as(usize, 1));
    expect_eq(rep.milestone_rendered, @as(usize, 1));
    expect_eq(rep.tokens_used, @as(u64, 65));
    expect_true(rep.truncated_by_budget);
    expect_true(!rep.truncated_by_buffer);

    const rendered: []const u8 = buf[0..rep.bytes_written];
    expect_true(containsSubstr(rendered, "[Thought] Root objective: optimize cache\n"));
    expect_true(containsSubstr(rendered, "... (2 steps omitted)\n"));
    expect_true(containsSubstr(rendered, "[Milestone:bottleneck] Found lock contention in LRU\n"));
    expect_true(containsSubstr(rendered, "... (1 step omitted)\n"));
    expect_true(containsSubstr(rendered, "[Action:patch] Implement 16-shard cache\n"));
    expect_true(!containsSubstr(rendered, "Run cold benchmark #1"));
    expect_true(!containsSubstr(rendered, "Consider sharded buckets"));

    // Verify strict chronological ordering: s1 < s4 < s6
    const p1 = indexOfSubstr(rendered, "[Thought]") ?? 9999;
    const p4 = indexOfSubstr(rendered, "[Milestone:bottleneck]") ?? 9999;
    const p6 = indexOfSubstr(rendered, "[Action:patch]") ?? 9999;
    expect_true(p1 < p4);
    expect_true(p4 < p6);
}

test("AgentScratchpad: empty render, zero token budget & insufficient output buffer guards") {
    let pad = AgentScratchpad.init();
    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];

    // Empty scratchpad render
    const rep_empty = pad.renderWithinTokenBudget(1000, *buf_s);
    expect_eq(rep_empty.bytes_written, @as(usize, 0));
    expect_eq(rep_empty.rendered_steps, @as(usize, 0));
    expect_eq(rep_empty.omitted_steps, @as(usize, 0));
    expect_true(!rep_empty.truncated_by_budget);
    expect_true(!rep_empty.truncated_by_buffer);

    * = pad.appendStep(ScratchStepKind.thought, "", "First step content here", 20, 100);
    * = pad.appendStep(ScratchStepKind.action, "run", "Second step content here", 20, 200);

    // Zero token budget omits both positive-token steps and emits trailing omission marker
    const rep_zero = pad.renderWithinTokenBudget(0, *buf_s);
    expect_eq(rep_zero.rendered_steps, @as(usize, 0));
    expect_eq(rep_zero.omitted_steps, @as(usize, 2));
    expect_true(rep_zero.truncated_by_budget);
    expect_true(!rep_zero.truncated_by_buffer);
    expect_true(containsSubstr(buf[0..rep_zero.bytes_written], "... (2 steps omitted)\n"));

    // Tiny output buffer triggers truncated_by_buffer without partial corrupt line
    let tiny: [36]u8 = [0; 36];
    let tiny_s: []u8 = tiny[0..];
    const rep_tiny = pad.renderWithinTokenBudget(1000, *tiny_s);
    expect_eq(rep_tiny.rendered_steps, @as(usize, 1));
    expect_true(rep_tiny.truncated_by_buffer);
    expect_true(bytesEq(tiny[0..rep_tiny.bytes_written], "[Thought] First step content here\n"));
}

test("AgentScratchpad: 64-step & 8-checkpoint capacity overflow boundaries") {
    let pad = AgentScratchpad.init();

    // Fill all 64 step slots
    let i: usize = 0;
    while (i < SCRATCHPAD_MAX_STEPS) : (i += 1) {
        const sid = pad.appendStep(ScratchStepKind.thought, "loop", "step payload", 10, @as(u64, i * 10));
        expect_true(sid != null);
    }
    expect_eq(pad.len(), SCRATCHPAD_MAX_STEPS);

    // 65th step must return null with zero side effects
    const overflow_step = pad.appendStep(ScratchStepKind.thought, "loop", "overflow", 10, 9999);
    expect_true(overflow_step == null);
    expect_eq(pad.len(), SCRATCHPAD_MAX_STEPS);
    expect_eq(pad.stats().total_appended, @as(u64, SCRATCHPAD_MAX_STEPS));

    // Fill all 8 checkpoint slots
    let c: usize = 0;
    while (c < SCRATCHPAD_MAX_CHECKPOINTS) : (c += 1) {
        const cid = pad.saveCheckpoint("cp");
        expect_true(cid != null);
    }
    expect_eq(pad.stats().checkpoint_count, SCRATCHPAD_MAX_CHECKPOINTS);

    // 9th checkpoint must return null with zero side effects
    const overflow_cp = pad.saveCheckpoint("overflow_cp");
    expect_true(overflow_cp == null);
    expect_eq(pad.stats().checkpoint_count, SCRATCHPAD_MAX_CHECKPOINTS);
}

test("AgentScratchpad: global stats accounting and clean clear() reset") {
    let pad = AgentScratchpad.init();

    const s1 = pad.appendStep(ScratchStepKind.thought, "", "t1", 10, 100) ?? 0;
    * = pad.pinStep(s1, true);
    * = pad.appendStep(ScratchStepKind.milestone, "m1", "done phase 1", 25, 200);
    * = pad.appendStep(ScratchStepKind.step_error, "tool", "timeout", 15, 300);
    * = pad.saveCheckpoint("cp1");

    const st = pad.stats();
    expect_eq(st.active_steps, @as(usize, 3));
    expect_eq(st.capacity, SCRATCHPAD_MAX_STEPS);
    expect_eq(st.checkpoint_count, @as(usize, 1));
    expect_eq(st.pinned_steps, @as(usize, 1));
    expect_eq(st.milestone_steps, @as(usize, 1));
    expect_eq(st.error_steps, @as(usize, 1));
    expect_eq(st.total_tokens, @as(u64, 50));
    expect_eq(st.total_appended, @as(u64, 3));

    pad.clear();

    const st_after = pad.stats();
    expect_eq(st_after.active_steps, @as(usize, 0));
    expect_eq(st_after.checkpoint_count, @as(usize, 0));
    expect_eq(st_after.pinned_steps, @as(usize, 0));
    expect_eq(st_after.milestone_steps, @as(usize, 0));
    expect_eq(st_after.error_steps, @as(usize, 0));
    expect_eq(st_after.total_tokens, @as(u64, 0));
    expect_eq(st_after.total_appended, @as(u64, 0));
    expect_eq(st_after.total_rollbacks, @as(u64, 0));
    expect_eq(st_after.total_rolled_back_steps, @as(u64, 0));
    expect_true(pad.getStep(s1) == null);

    // Post-clear step IDs restart cleanly at 1
    const fresh_id = pad.appendStep(ScratchStepKind.thought, "", "fresh", 5, 1) ?? 0;
    expect_eq(fresh_id, @as(u32, 1));
}
