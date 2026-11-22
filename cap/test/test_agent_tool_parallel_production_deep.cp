// Origin: Pure-CP Stdlib Test Suite (AI Agent Parallel Tool-Call Conflict Analyzer & Wave Scheduler) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/tool_parallel.cp
// Covers:
//   1. Multi-way SharedRead full concurrency in Wave 0 (same & distinct resources)
//   2. Same-file Read -> Write -> Read split into 3 strictly ordered waves + independent resource interleaving
//   3. GlobalBarrier strict pre/post isolation across surrounding parallel waves
//   4. max_parallelism concurrency cap slicing across waves
//   5. Critical path latency, speedup_x100 accounting, empty batch & invalid parameter guards

import { test, expect_eq } from "cap:test";
import {
    ResourceLockMode,
    ToolCallRequest,
    WaveAssignment,
    ParallelPlanStats,
    callsConflict,
    planToolWaves,
} from "../../lib/agent/index.cp";

fn testMultiReadFullConcurrency()! {
    const calls: [4]ToolCallRequest = [
        ToolCallRequest{ call_id: 10, resource_hash: 0x100, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 50 },
        ToolCallRequest{ call_id: 11, resource_hash: 0x100, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 80 },
        ToolCallRequest{ call_id: 12, resource_hash: 0x200, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 100 },
        ToolCallRequest{ call_id: 13, resource_hash: 0x300, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 70 },
    ];

    expect_eq(callsConflict(calls[0], calls[1]), false);
    expect_eq(callsConflict(calls[0], calls[2]), false);

    let out_buf: [4]WaveAssignment = [WaveAssignment{}; 4];
    let out: []WaveAssignment = out_buf[0..];

    const st = planToolWaves(calls[0..], 4, out) ?? ParallelPlanStats{};
    expect_eq(st.total_calls, 4);
    expect_eq(st.total_waves, 1);
    expect_eq(st.max_wave_width, 4);
    expect_eq(st.serial_ms, 300);
    expect_eq(st.critical_path_ms, 100);
    expect_eq(st.speedup_x100, 300);

    expect_eq(out[0].call_id, 10);
    expect_eq(out[0].wave_index, 0);
    expect_eq(out[1].wave_index, 0);
    expect_eq(out[2].wave_index, 0);
    expect_eq(out[3].wave_index, 0);
}

fn testReadWriteReadHazardOrderingAndInterleaving()! {
    // File A: Read -> Write -> Read (must be 3 waves: 0, 1, 2)
    // File B: Read -> Write (independent of A, packs into waves 0 and 1!)
    const calls: [5]ToolCallRequest = [
        ToolCallRequest{ call_id: 1, resource_hash: 0xAAA, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 40 },
        ToolCallRequest{ call_id: 2, resource_hash: 0xAAA, lock_mode: ResourceLockMode.ExclusiveWrite, estimated_ms: 60 },
        ToolCallRequest{ call_id: 3, resource_hash: 0xAAA, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 30 },
        ToolCallRequest{ call_id: 4, resource_hash: 0xBBB, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 50 },
        ToolCallRequest{ call_id: 5, resource_hash: 0xBBB, lock_mode: ResourceLockMode.ExclusiveWrite, estimated_ms: 70 },
    ];

    expect_eq(callsConflict(calls[0], calls[1]), true);  // WAR on 0xAAA
    expect_eq(callsConflict(calls[1], calls[2]), true);  // RAW on 0xAAA
    expect_eq(callsConflict(calls[1], calls[4]), false); // Distinct resources 0xAAA vs 0xBBB

    let out_buf: [5]WaveAssignment = [WaveAssignment{}; 5];
    let out: []WaveAssignment = out_buf[0..];

    const st = planToolWaves(calls[0..], 4, out) ?? ParallelPlanStats{};
    expect_eq(st.total_waves, 3);
    expect_eq(st.max_wave_width, 2);

    // File A strictly serialized: 0 -> 1 -> 2
    expect_eq(out[0].wave_index, 0);
    expect_eq(out[1].wave_index, 1);
    expect_eq(out[2].wave_index, 2);
    // File B interleaved safely into waves 0 and 1
    expect_eq(out[3].wave_index, 0);
    expect_eq(out[4].wave_index, 1);

    // Wave 0 max(40,50)=50, Wave 1 max(60,70)=70, Wave 2 max(30)=30 -> critical_path=150
    expect_eq(st.serial_ms, 250);
    expect_eq(st.critical_path_ms, 150);
    expect_eq(st.speedup_x100, 167);
}

fn testGlobalBarrierStrictIsolation()! {
    const calls: [5]ToolCallRequest = [
        ToolCallRequest{ call_id: 1, resource_hash: 0x1, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 50 },
        ToolCallRequest{ call_id: 2, resource_hash: 0x2, lock_mode: ResourceLockMode.ExclusiveWrite, estimated_ms: 80 },
        ToolCallRequest{ call_id: 3, resource_hash: 0x0, lock_mode: ResourceLockMode.GlobalBarrier, estimated_ms: 200 },
        ToolCallRequest{ call_id: 4, resource_hash: 0x1, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 40 },
        ToolCallRequest{ call_id: 5, resource_hash: 0x2, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 60 },
    ];

    expect_eq(callsConflict(calls[0], calls[2]), true);
    expect_eq(callsConflict(calls[2], calls[4]), true);

    let out_buf: [5]WaveAssignment = [WaveAssignment{}; 5];
    let out: []WaveAssignment = out_buf[0..];

    const st = planToolWaves(calls[0..], 8, out) ?? ParallelPlanStats{};
    expect_eq(st.total_waves, 3);
    expect_eq(st.max_wave_width, 2);

    // Pre-barrier calls in Wave 0
    expect_eq(out[0].wave_index, 0);
    expect_eq(out[1].wave_index, 0);
    // Barrier isolated alone in Wave 1
    expect_eq(out[2].wave_index, 1);
    // Post-barrier calls in Wave 2
    expect_eq(out[3].wave_index, 2);
    expect_eq(out[4].wave_index, 2);

    // Critical path: 80 + 200 + 60 = 340, serial = 430
    expect_eq(st.serial_ms, 430);
    expect_eq(st.critical_path_ms, 340);
}

fn testMaxParallelismCapSlicing()! {
    const calls: [5]ToolCallRequest = [
        ToolCallRequest{ call_id: 1, resource_hash: 0x1, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 100 },
        ToolCallRequest{ call_id: 2, resource_hash: 0x2, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 100 },
        ToolCallRequest{ call_id: 3, resource_hash: 0x3, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 100 },
        ToolCallRequest{ call_id: 4, resource_hash: 0x4, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 100 },
        ToolCallRequest{ call_id: 5, resource_hash: 0x5, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 100 },
    ];

    let out_buf: [5]WaveAssignment = [WaveAssignment{}; 5];
    let out: []WaveAssignment = out_buf[0..];

    // Capped at max_parallelism == 2 -> waves [0, 0, 1, 1, 2]
    const st = planToolWaves(calls[0..], 2, out) ?? ParallelPlanStats{};
    expect_eq(st.total_waves, 3);
    expect_eq(st.max_wave_width, 2);
    expect_eq(out[0].wave_index, 0);
    expect_eq(out[1].wave_index, 0);
    expect_eq(out[2].wave_index, 1);
    expect_eq(out[3].wave_index, 1);
    expect_eq(out[4].wave_index, 2);
    expect_eq(st.serial_ms, 500);
    expect_eq(st.critical_path_ms, 300);
    expect_eq(st.speedup_x100, 167);
}

fn testEmptyBatchZeroEstimatesAndInvalidGuards()! {
    let out_buf: [2]WaveAssignment = [WaveAssignment{}; 2];
    let out: []WaveAssignment = out_buf[0..];

    // 1. Empty call list returns zeroed stats with 1.00x baseline speedup
    const empty_calls: [0]ToolCallRequest = [];
    const empty_st = planToolWaves(empty_calls[0..], 4, out) ?? ParallelPlanStats{};
    expect_eq(empty_st.total_calls, 0);
    expect_eq(empty_st.total_waves, 0);
    expect_eq(empty_st.max_wave_width, 0);
    expect_eq(empty_st.speedup_x100, 100);

    // 2. Zero estimated_ms calls avoid division by zero and return 100 (1.00x)
    const zero_ms_calls: [2]ToolCallRequest = [
        ToolCallRequest{ call_id: 1, resource_hash: 0x1, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 0 },
        ToolCallRequest{ call_id: 2, resource_hash: 0x2, lock_mode: ResourceLockMode.SharedRead, estimated_ms: 0 },
    ];
    const zero_st = planToolWaves(zero_ms_calls[0..], 2, out) ?? ParallelPlanStats{};
    expect_eq(zero_st.total_waves, 1);
    expect_eq(zero_st.critical_path_ms, 0);
    expect_eq(zero_st.speedup_x100, 100);

    // 3. max_parallelism == 0 rejected
    expect_eq(planToolWaves(zero_ms_calls[0..], 0, out) == null, true);

    // 4. Undersized output buffer rejected
    let small_buf: [1]WaveAssignment = [WaveAssignment{}; 1];
    let small_out: []WaveAssignment = small_buf[0..];
    expect_eq(planToolWaves(zero_ms_calls[0..], 2, small_out) == null, true);
}

test("agent tool_parallel: multi-way SharedRead full concurrency in single wave", testMultiReadFullConcurrency);
test("agent tool_parallel: same-file Read-Write-Read 3-wave ordering with independent interleaving", testReadWriteReadHazardOrderingAndInterleaving);
test("agent tool_parallel: GlobalBarrier strict pre/post wave isolation", testGlobalBarrierStrictIsolation);
test("agent tool_parallel: max_parallelism concurrency cap wave slicing", testMaxParallelismCapSlicing);
test("agent tool_parallel: empty batch, zero-ms speedup accounting and invalid guards", testEmptyBatchZeroEstimatesAndInvalidGuards);
