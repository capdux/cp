// Origin: Pure-CP Stdlib Test Suite (lib/agent/tool_batcher.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Tool Call Batcher, Read Coalescer & Hazard Wave Planner:
//   1. Multiple pure_read tools run in a single parallel wave with exact critical-path speedup_x100
//   2. Duplicate identical pure_read calls auto-coalesce and consume zero concurrency slots
//   3. Same-resource RAW / WAR / WAW hazards strictly serialize across waves (post-write read NEVER coalesces with pre-write read)
//   4. global_barrier enforces exclusive pre- and post-barrier waves
//   5. Multi-resource concurrency + max_parallel_per_wave throttling splits waves cleanly
//   6. Post-write duplicate reads coalesce with each other (same post-write wave) but not pre-write read
//   7. Boundary guards: empty input, >64 calls cap, undersized output buffer & zero-ms calls

import { test, expect_eq } from "cap:test";
import {
    TOOL_BATCH_MAX_CALLS,
    ToolAccessKind,
    ToolBatchItem,
    ToolBatchScheduleEntry,
    ToolBatchPlanSummary,
    planToolCallBatch,
} from "cap:agent";

// ============================================================================
// Test 1: Pure-Read Full Parallel Single Wave & Speedup Calculation
// ============================================================================
test("tool_batcher: multiple pure_read calls execute in single wave with speedup_x100", () => {
    expect_eq(TOOL_BATCH_MAX_CALLS, 64);

    // 4 distinct pure_read calls (two on same resource_hash=1 with different args, two on other resources)
    const items: [4]ToolBatchItem = [
        ToolBatchItem{ call_id: 101, tool_hash: 1, args_hash: 10, resource_hash: 1, access: ToolAccessKind.pure_read, estimated_ms: 100 },
        ToolBatchItem{ call_id: 102, tool_hash: 1, args_hash: 20, resource_hash: 1, access: ToolAccessKind.pure_read, estimated_ms: 80 },
        ToolBatchItem{ call_id: 103, tool_hash: 2, args_hash: 30, resource_hash: 2, access: ToolAccessKind.pure_read, estimated_ms: 60 },
        ToolBatchItem{ call_id: 104, tool_hash: 3, args_hash: 40, resource_hash: 3, access: ToolAccessKind.pure_read, estimated_ms: 60 },
    ];
    let sched_buf: [4]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 4];
    let sched: []ToolBatchScheduleEntry = sched_buf[0..];

    const s: ToolBatchPlanSummary = planToolCallBatch(items[0..], 0, sched);
    expect_eq(s.total_calls, 4);
    expect_eq(s.unique_executions, 4);
    expect_eq(s.coalesced_duplicates, 0);
    expect_eq(s.wave_count, 1);
    expect_eq(s.estimated_serial_ms, 300);
    expect_eq(s.estimated_parallel_ms, 100);
    expect_eq(s.speedup_x100, 300); // 3.00x speedup

    let i: usize = 0;
    while (i < 4) : (i += 1) {
        expect_eq(sched_buf[i].wave_index, 0);
        expect_eq(sched_buf[i].is_coalesced, false);
        expect_eq(sched_buf[i].coalesced_into_idx, 0xFFFF);
    }
});

// ============================================================================
// Test 2: Identical Pure-Read Coalescing Consumes Zero Concurrency Slots
// ============================================================================
test("tool_batcher: duplicate pure_read coalescing consumes zero concurrency slots", () => {
    // max_parallel_per_wave = 2; 2 unique reads + 2 duplicates of call 0 -> ALL fit in wave 0!
    const items: [5]ToolBatchItem = [
        ToolBatchItem{ call_id: 1, tool_hash: 10, args_hash: 100, resource_hash: 7, access: ToolAccessKind.pure_read, estimated_ms: 50 },
        ToolBatchItem{ call_id: 2, tool_hash: 20, args_hash: 200, resource_hash: 8, access: ToolAccessKind.pure_read, estimated_ms: 40 },
        ToolBatchItem{ call_id: 3, tool_hash: 10, args_hash: 100, resource_hash: 7, access: ToolAccessKind.pure_read, estimated_ms: 50 },
        ToolBatchItem{ call_id: 4, tool_hash: 10, args_hash: 100, resource_hash: 7, access: ToolAccessKind.pure_read, estimated_ms: 50 },
        ToolBatchItem{ call_id: 5, tool_hash: 30, args_hash: 300, resource_hash: 9, access: ToolAccessKind.pure_read, estimated_ms: 30 },
    ];
    let sched_buf: [5]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 5];
    let sched: []ToolBatchScheduleEntry = sched_buf[0..];

    const s = planToolCallBatch(items[0..], 2, sched);
    expect_eq(s.total_calls, 5);
    expect_eq(s.unique_executions, 3);
    expect_eq(s.coalesced_duplicates, 2);
    expect_eq(s.wave_count, 2);
    expect_eq(s.estimated_serial_ms, 220);
    expect_eq(s.estimated_parallel_ms, 80); // wave 0 max(50,40)=50 + wave 1 (30) = 80
    expect_eq(s.speedup_x100, 275);         // 220 * 100 / 80 = 275

    // Calls 0 and 1 are primary in wave 0; calls 2 and 3 coalesce into call 0 in wave 0
    expect_eq(sched_buf[0].wave_index, 0);
    expect_eq(sched_buf[0].is_coalesced, false);
    expect_eq(sched_buf[1].wave_index, 0);
    expect_eq(sched_buf[1].is_coalesced, false);
    expect_eq(sched_buf[2].wave_index, 0);
    expect_eq(sched_buf[2].is_coalesced, true);
    expect_eq(sched_buf[2].coalesced_into_idx, 0);
    expect_eq(sched_buf[3].wave_index, 0);
    expect_eq(sched_buf[3].is_coalesced, true);
    expect_eq(sched_buf[3].coalesced_into_idx, 0);
    // Call 4 overflows max_parallel_per_wave=2 into wave 1
    expect_eq(sched_buf[4].wave_index, 1);
    expect_eq(sched_buf[4].is_coalesced, false);
});

// ============================================================================
// Test 3: RAW / WAR / WAW Same-Resource Hazards & No Pre/Post-Write Coalescing
// ============================================================================
test("tool_batcher: RAW, WAR, WAW hazards enforce strict wave order & block stale coalescing", () => {
    // Sequence on resource_hash=42:
    //   0: pure_read(A)   -> wave 0
    //   1: resource_write -> WAR -> wave 1
    //   2: resource_write -> WAW -> wave 2
    //   3: pure_read(A)   -> RAW -> wave 3 (MUST NOT coalesce with call 0!)
    //   4: pure_read(A)   -> same wave 3 -> coalesces with call 3!
    const items: [5]ToolBatchItem = [
        ToolBatchItem{ call_id: 10, tool_hash: 1, args_hash: 9, resource_hash: 42, access: ToolAccessKind.pure_read, estimated_ms: 20 },
        ToolBatchItem{ call_id: 11, tool_hash: 2, args_hash: 1, resource_hash: 42, access: ToolAccessKind.resource_write, estimated_ms: 30 },
        ToolBatchItem{ call_id: 12, tool_hash: 2, args_hash: 2, resource_hash: 42, access: ToolAccessKind.resource_write, estimated_ms: 40 },
        ToolBatchItem{ call_id: 13, tool_hash: 1, args_hash: 9, resource_hash: 42, access: ToolAccessKind.pure_read, estimated_ms: 20 },
        ToolBatchItem{ call_id: 14, tool_hash: 1, args_hash: 9, resource_hash: 42, access: ToolAccessKind.pure_read, estimated_ms: 20 },
    ];
    let sched_buf: [5]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 5];
    let sched: []ToolBatchScheduleEntry = sched_buf[0..];

    const s = planToolCallBatch(items[0..], 0, sched);
    expect_eq(s.wave_count, 4);
    expect_eq(s.unique_executions, 4);
    expect_eq(s.coalesced_duplicates, 1);

    expect_eq(sched_buf[0].wave_index, 0);
    expect_eq(sched_buf[0].is_coalesced, false);
    expect_eq(sched_buf[1].wave_index, 1);
    expect_eq(sched_buf[2].wave_index, 2);
    expect_eq(sched_buf[3].wave_index, 3);
    expect_eq(sched_buf[3].is_coalesced, false);
    expect_eq(sched_buf[4].wave_index, 3);
    expect_eq(sched_buf[4].is_coalesced, true);
    expect_eq(sched_buf[4].coalesced_into_idx, 3);
});

// ============================================================================
// Test 4: global_barrier Isolates Pre- and Post-Barrier Waves Exclusively
// ============================================================================
test("tool_batcher: global_barrier isolates exclusive wave & prevents cross-barrier coalescing", () => {
    const items: [5]ToolBatchItem = [
        ToolBatchItem{ call_id: 1, tool_hash: 5, args_hash: 50, resource_hash: 1, access: ToolAccessKind.pure_read, estimated_ms: 10 },
        ToolBatchItem{ call_id: 2, tool_hash: 6, args_hash: 60, resource_hash: 2, access: ToolAccessKind.resource_write, estimated_ms: 15 },
        ToolBatchItem{ call_id: 3, tool_hash: 99, args_hash: 1, resource_hash: 0, access: ToolAccessKind.global_barrier, estimated_ms: 100 },
        ToolBatchItem{ call_id: 4, tool_hash: 5, args_hash: 50, resource_hash: 1, access: ToolAccessKind.pure_read, estimated_ms: 10 },
        ToolBatchItem{ call_id: 5, tool_hash: 7, args_hash: 70, resource_hash: 3, access: ToolAccessKind.pure_read, estimated_ms: 20 },
    ];
    let sched_buf: [5]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 5];
    let sched: []ToolBatchScheduleEntry = sched_buf[0..];

    const s = planToolCallBatch(items[0..], 0, sched);
    // Wave 0: calls 0 & 1 (different resources); Wave 1: call 2 (barrier alone); Wave 2: calls 3 & 4
    expect_eq(s.wave_count, 3);
    expect_eq(s.unique_executions, 5);
    expect_eq(s.coalesced_duplicates, 0); // call 3 must NOT coalesce with call 0 across barrier!
    expect_eq(sched_buf[0].wave_index, 0);
    expect_eq(sched_buf[1].wave_index, 0);
    expect_eq(sched_buf[2].wave_index, 1);
    expect_eq(sched_buf[3].wave_index, 2);
    expect_eq(sched_buf[3].is_coalesced, false);
    expect_eq(sched_buf[4].wave_index, 2);
    expect_eq(s.estimated_parallel_ms, 135); // 15 + 100 + 20
});

// ============================================================================
// Test 5: Multi-Resource Independent Writes Parallelize + Concurrency Cap Splits Waves
// ============================================================================
test("tool_batcher: disjoint resource writes run in parallel & obey max_parallel_per_wave", () => {
    const items: [5]ToolBatchItem = [
        ToolBatchItem{ call_id: 1, tool_hash: 1, args_hash: 1, resource_hash: 10, access: ToolAccessKind.resource_write, estimated_ms: 50 },
        ToolBatchItem{ call_id: 2, tool_hash: 1, args_hash: 2, resource_hash: 20, access: ToolAccessKind.resource_write, estimated_ms: 50 },
        ToolBatchItem{ call_id: 3, tool_hash: 1, args_hash: 3, resource_hash: 30, access: ToolAccessKind.resource_write, estimated_ms: 50 },
        ToolBatchItem{ call_id: 4, tool_hash: 1, args_hash: 4, resource_hash: 40, access: ToolAccessKind.resource_write, estimated_ms: 50 },
        ToolBatchItem{ call_id: 5, tool_hash: 1, args_hash: 5, resource_hash: 50, access: ToolAccessKind.resource_write, estimated_ms: 50 },
    ];
    let sched_buf: [5]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 5];
    let sched: []ToolBatchScheduleEntry = sched_buf[0..];

    // Cap at 2 parallel executions per wave -> 5 calls split across 3 waves (2 + 2 + 1)
    const s = planToolCallBatch(items[0..], 2, sched);
    expect_eq(s.wave_count, 3);
    expect_eq(s.unique_executions, 5);
    expect_eq(sched_buf[0].wave_index, 0);
    expect_eq(sched_buf[1].wave_index, 0);
    expect_eq(sched_buf[2].wave_index, 1);
    expect_eq(sched_buf[3].wave_index, 1);
    expect_eq(sched_buf[4].wave_index, 2);
    expect_eq(s.estimated_serial_ms, 250);
    expect_eq(s.estimated_parallel_ms, 150);
});

// ============================================================================
// Test 6: Two Consecutive Global Barriers Each Get Exclusive Waves
// ============================================================================
test("tool_batcher: consecutive global_barriers never share a wave", () => {
    const items: [3]ToolBatchItem = [
        ToolBatchItem{ call_id: 1, tool_hash: 99, args_hash: 1, resource_hash: 0, access: ToolAccessKind.global_barrier, estimated_ms: 40 },
        ToolBatchItem{ call_id: 2, tool_hash: 99, args_hash: 1, resource_hash: 0, access: ToolAccessKind.global_barrier, estimated_ms: 60 },
        ToolBatchItem{ call_id: 3, tool_hash: 10, args_hash: 1, resource_hash: 5, access: ToolAccessKind.pure_read, estimated_ms: 20 },
    ];
    let sched_buf: [3]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 3];
    let sched: []ToolBatchScheduleEntry = sched_buf[0..];

    const s = planToolCallBatch(items[0..], 0, sched);
    expect_eq(s.wave_count, 3);
    expect_eq(s.unique_executions, 3);
    expect_eq(s.coalesced_duplicates, 0);
    expect_eq(sched_buf[0].wave_index, 0);
    expect_eq(sched_buf[1].wave_index, 1);
    expect_eq(sched_buf[2].wave_index, 2);
    expect_eq(s.speedup_x100, 100);
});

// ============================================================================
// Test 7: Boundary Guards (Empty Input, Undersized Buffer, >64 Calls Cap & Zero-ms)
// ============================================================================
test("tool_batcher: empty input, undersized buffer, >64 cap & zero-ms guards", () => {
    // 1. Empty input
    const dummy: [1]ToolBatchItem = [ToolBatchItem{}];
    let sched1: [1]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 1];
    let sched1_s: []ToolBatchScheduleEntry = sched1[0..];
    const empty_res = planToolCallBatch(dummy[0..0], 4, sched1_s);
    expect_eq(empty_res.total_calls, 0);
    expect_eq(empty_res.wave_count, 0);
    expect_eq(empty_res.speedup_x100, 0);

    // 2. Undersized output buffer (items=4, out_schedule=2 -> safely plans 2)
    const four: [4]ToolBatchItem = [
        ToolBatchItem{ call_id: 1, tool_hash: 1, args_hash: 1, resource_hash: 1, access: ToolAccessKind.pure_read, estimated_ms: 0 },
        ToolBatchItem{ call_id: 2, tool_hash: 2, args_hash: 2, resource_hash: 2, access: ToolAccessKind.pure_read, estimated_ms: 0 },
        ToolBatchItem{ call_id: 3, tool_hash: 3, args_hash: 3, resource_hash: 3, access: ToolAccessKind.pure_read, estimated_ms: 0 },
        ToolBatchItem{ call_id: 4, tool_hash: 4, args_hash: 4, resource_hash: 4, access: ToolAccessKind.pure_read, estimated_ms: 0 },
    ];
    let small_buf: [2]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 2];
    let small_s: []ToolBatchScheduleEntry = small_buf[0..];
    const small_res = planToolCallBatch(four[0..], 0, small_s);
    expect_eq(small_res.total_calls, 2);
    expect_eq(small_res.unique_executions, 2);
    expect_eq(small_res.speedup_x100, 100); // zero-ms safe fallback 1.00x (no div-by-zero)

    // 3. >64 calls input safely capped at TOOL_BATCH_MAX_CALLS (64)
    let many_items: [70]ToolBatchItem = [ToolBatchItem{}; 70];
    let idx: usize = 0;
    while (idx < 70) : (idx += 1) {
        many_items[idx] = ToolBatchItem{
            call_id: (idx + 1) as u64,
            tool_hash: (idx + 1) as u64,
            args_hash: (idx + 1) as u64,
            resource_hash: (idx + 1) as u64,
            access: ToolAccessKind.pure_read,
            estimated_ms: 10,
        };
    }
    let many_sched: [70]ToolBatchScheduleEntry = [ToolBatchScheduleEntry{}; 70];
    let many_s: []ToolBatchScheduleEntry = many_sched[0..];
    const cap_res = planToolCallBatch(many_items[0..], 0, many_s);
    expect_eq(cap_res.total_calls, TOOL_BATCH_MAX_CALLS);
    expect_eq(cap_res.unique_executions, TOOL_BATCH_MAX_CALLS);
    expect_eq(cap_res.wave_count, 1);
    expect_eq(cap_res.estimated_serial_ms, 640);
    expect_eq(cap_res.estimated_parallel_ms, 10);
    expect_eq(cap_res.speedup_x100, 6400);
});
