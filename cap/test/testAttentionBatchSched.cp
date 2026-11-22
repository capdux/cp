// Origin: Pure-CP Stdlib Test Suite (lib/attention/batchSched.cp) | Created: 2026-11-23
// Deep production regression tests for cap:attention chunked-prefill batch scheduler (`lib/attention/batchSched.cp`):
//   1. Long prompt chunked prefill (300 -> 128 + 128 + 44), TTFT recording, decode & auto KV release
//   2. Running decode protection & KV-pressure priority/arrival preemption reclaiming blocks
//   3. Resuming preempted requests ahead of waiting queue (`is_resume_from_preempt == true`)
//   4. TTFT deadline urgency admission priority within `ttft_urgency_window_ms`
//   5. Sarathi mixed decode + chunked prefill packing strictly within `max_batch_tokens`
//   6. Abort KV reclamation, early EOS completion, `reapTerminal()` slot reuse & 64-slot capacity guards
//   7. Cumulative `BatchSchedStats` telemetry, `reset()`, and `cap.attention` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    BATCH_SCHED_MAX_REQS,
    BatchReqState,
    BatchExecPhase,
    BatchSchedConfig,
    BatchStepPlan,
    ChunkedBatchScheduler,
} from "cap:attention";

test("batchSched: chunked prefill slicing (128+128+44), TTFT tracking, decode & auto KV release") {
    let cfg = BatchSchedConfig.defaults();
    cfg.max_batch_tokens = 256;
    cfg.chunk_size = 128;
    cfg.total_kv_blocks = 32;
    cfg.kv_block_size = 16;

    let sched = ChunkedBatchScheduler.init(cfg) ?? return;
    expect_true(sched.submitRequest(101, 300, 2, 10, 1000, 2000));

    let plan = BatchStepPlan{};

    // Step 1: 128 prefill tokens (8 KV blocks)
    sched.scheduleStep(1010, *plan);
    expect_eq(plan.item_count, @as(usize, 1));
    expect_eq(plan.prefill_count, @as(usize, 1));
    expect_true(plan.items[0].phase == BatchExecPhase.prefill_chunk);
    expect_eq(plan.items[0].tokens_scheduled, @as(u32, 128));
    expect_eq(plan.items[0].kv_blocks_allocated, @as(u32, 8));
    expect_true(sched.commitStepResult(101, 128, false, 1020));

    // Step 2: next 128 prefill tokens (total 256 -> 16 blocks, +8)
    sched.scheduleStep(1030, *plan);
    expect_eq(plan.items[0].tokens_scheduled, @as(u32, 128));
    expect_eq(plan.items[0].kv_blocks_allocated, @as(u32, 8));
    expect_true(sched.commitStepResult(101, 128, false, 1040));

    // Step 3: final 44 prefill tokens (total 300 -> ceil(300/16)=19 blocks, +3) -> TTFT recorded at 1060 - 1000 = 60ms
    sched.scheduleStep(1050, *plan);
    expect_eq(plan.items[0].tokens_scheduled, @as(u32, 44));
    expect_eq(plan.items[0].kv_blocks_allocated, @as(u32, 3));
    expect_true(sched.commitStepResult(101, 44, false, 1060));

    expect_eq(sched.stats().avg_ttft_ms, @as(u64, 60));
    expect_eq(sched.stats().used_kv_blocks, @as(u32, 19));

    // Step 4: Decode token #1 (total 301 -> still 19 blocks since 19*16=304)
    sched.scheduleStep(1070, *plan);
    expect_eq(plan.decode_count, @as(usize, 1));
    expect_true(plan.items[0].phase == BatchExecPhase.decode);
    expect_eq(plan.items[0].tokens_scheduled, @as(u32, 1));
    expect_eq(plan.items[0].kv_blocks_allocated, @as(u32, 0));
    expect_true(sched.commitStepResult(101, 1, false, 1080));

    // Step 5: Decode token #2 reaches max_output_tokens=2 -> finished & all 19 KV blocks auto-released!
    sched.scheduleStep(1090, *plan);
    expect_true(sched.commitStepResult(101, 1, false, 1100));

    const st = sched.stats();
    expect_eq(st.used_kv_blocks, @as(u32, 0));
    expect_eq(st.free_kv_blocks, @as(u32, 32));
    expect_eq(st.finished_count, @as(usize, 1));
    expect_eq(st.total_completed, @as(u64, 1));
    expect_eq(st.total_prefill_tokens, @as(u64, 300));
    expect_eq(st.total_decode_tokens, @as(u64, 2));
}

test("batchSched: decode protection preempts lowest-priority latest-arriving sequence on KV exhaustion") {
    let cfg = BatchSchedConfig.defaults();
    cfg.max_batch_tokens = 128;
    cfg.chunk_size = 64;
    cfg.total_kv_blocks = 4; // Exact capacity for two 32-token prompts (2 blocks each)
    cfg.kv_block_size = 16;

    let sched = ChunkedBatchScheduler.init(cfg) ?? return;
    // req 1: high priority (50), 32 prompt tokens (2 blocks)
    // req 2: low priority (10), 32 prompt tokens (2 blocks)
    expect_true(sched.submitRequest(1, 32, 5, 50, 100, 0));
    expect_true(sched.submitRequest(2, 32, 5, 10, 110, 0));

    let plan = BatchStepPlan{};
    // Step 1: Both admit and prefill 32 tokens each -> 4/4 KV blocks used
    sched.scheduleStep(120, *plan);
    expect_eq(plan.prefill_count, @as(usize, 2));
    expect_eq(plan.free_kv_blocks_after, @as(u32, 0));
    expect_true(sched.commitStepResult(1, 32, false, 130));
    expect_true(sched.commitStepResult(2, 32, false, 130));

    // Step 2: req 1 wants decode token #33 -> needs 3rd KV block, but free=0 -> preempts low-priority req 2!
    sched.scheduleStep(140, *plan);
    expect_eq(plan.preempted_count, @as(usize, 1));
    expect_eq(plan.preempted_ids[0], @as(u64, 2));
    expect_eq(plan.decode_count, @as(usize, 1));
    expect_eq(plan.items[0].req_id, @as(u64, 1));
    expect_eq(plan.items[0].kv_blocks_allocated, @as(u32, 1));
    // req 1 holds 3 blocks, req 2 released 2 blocks => 1 free block left
    expect_eq(plan.free_kv_blocks_after, @as(u32, 1));

    if (r2 := sched.getRequest(2)) {
        expect_true(r2.state == BatchReqState.preempted);
        expect_eq(r2.kv_blocks_held, @as(u32, 0));
        expect_eq(r2.preempt_count, @as(u32, 1));
    } else {
        expect_true(false);
    }
}

test("batchSched: preempted requests resume ahead of waiting requests once KV blocks free") {
    let cfg = BatchSchedConfig.defaults();
    cfg.max_batch_tokens = 128;
    cfg.chunk_size = 64;
    cfg.total_kv_blocks = 4;
    cfg.kv_block_size = 16;
    cfg.max_running_seqs = 2;

    let sched = ChunkedBatchScheduler.init(cfg) ?? return;
    expect_true(sched.submitRequest(1, 32, 1, 50, 100, 0));
    expect_true(sched.submitRequest(2, 32, 2, 10, 110, 0));

    let plan = BatchStepPlan{};
    sched.scheduleStep(120, *plan);
    expect_true(sched.commitStepResult(1, 32, false, 130));
    expect_true(sched.commitStepResult(2, 32, false, 130));

    // Step 2: req 1 decodes & preempts req 2, then finishes (max_output_tokens=1) freeing all 3 blocks!
    sched.scheduleStep(140, *plan);
    expect_true(sched.commitStepResult(1, 1, false, 150));
    expect_eq(sched.stats().free_kv_blocks, @as(u32, 4));

    // Submit a brand new waiting req 3 with higher priority (80), but req 2 is preempted and resumes first!
    expect_true(sched.submitRequest(3, 32, 2, 80, 155, 0));
    sched.scheduleStep(160, *plan);

    expect_eq(plan.item_count, @as(usize, 2));
    expect_eq(plan.items[0].req_id, @as(u64, 2));
    expect_true(plan.items[0].is_resume_from_preempt);
    expect_eq(plan.items[1].req_id, @as(u64, 3));
    expect_eq(plan.items[1].is_resume_from_preempt, false);
}

test("batchSched: TTFT deadline urgency boosts imminent waiting requests ahead of non-urgent ones") {
    let cfg = BatchSchedConfig.defaults();
    cfg.max_batch_tokens = 64;
    cfg.chunk_size = 64;
    cfg.total_kv_blocks = 16;
    cfg.kv_block_size = 16;
    cfg.max_running_seqs = 1; // Only 1 waiting request can be admitted
    cfg.ttft_urgency_window_ms = 200;

    let sched = ChunkedBatchScheduler.init(cfg) ?? return;
    // req 10: high priority (100), far deadline (5000)
    // req 20: lower priority (20), urgent deadline (1150 -> only 150ms left <= 200ms window!)
    expect_true(sched.submitRequest(10, 32, 4, 100, 900, 5000));
    expect_true(sched.submitRequest(20, 32, 4, 20, 950, 1150));

    let plan = BatchStepPlan{};
    sched.scheduleStep(1000, *plan);
    expect_eq(plan.item_count, @as(usize, 1));
    expect_eq(plan.items[0].req_id, @as(u64, 20));
}

test("batchSched: Sarathi mixed decode + chunked prefill respects exact max_batch_tokens budget") {
    let cfg = BatchSchedConfig.defaults();
    cfg.max_batch_tokens = 65; // 1 decode token + 64 chunked prefill tokens!
    cfg.chunk_size = 128;
    cfg.total_kv_blocks = 64;
    cfg.kv_block_size = 16;

    let sched = ChunkedBatchScheduler.init(cfg) ?? return;
    expect_true(sched.submitRequest(1, 16, 10, 50, 100, 0));

    let plan = BatchStepPlan{};
    // Prefill req 1 completely (16 tokens)
    sched.scheduleStep(110, *plan);
    expect_true(sched.commitStepResult(1, 16, false, 120));

    // Submit long prompt req 2 (200 tokens); next step should pack req 1 decode (1) + req 2 prefill chunk (64) = 65
    expect_true(sched.submitRequest(2, 200, 10, 30, 125, 0));
    sched.scheduleStep(130, *plan);

    expect_eq(plan.decode_count, @as(usize, 1));
    expect_eq(plan.prefill_count, @as(usize, 1));
    expect_eq(plan.total_tokens_scheduled, @as(u32, 65));
    expect_eq(plan.items[0].req_id, @as(u64, 1));
    expect_eq(plan.items[0].tokens_scheduled, @as(u32, 1));
    expect_eq(plan.items[1].req_id, @as(u64, 2));
    expect_eq(plan.items[1].tokens_scheduled, @as(u32, 64));
}

test("batchSched: abort KV reclamation, early EOS, reapTerminal reuse, and 64-slot capacity guards") {
    // Invalid configs rejected
    let bad_cfg = BatchSchedConfig.defaults();
    bad_cfg.max_batch_tokens = 0;
    expect_true(ChunkedBatchScheduler.init(bad_cfg) == null);
    bad_cfg.max_batch_tokens = 64;
    bad_cfg.max_running_seqs = 65;
    expect_true(ChunkedBatchScheduler.init(bad_cfg) == null);

    let sched = ChunkedBatchScheduler.init(BatchSchedConfig.defaults()) ?? return;
    expect_eq(sched.submitRequest(0, 16, 4, 1, 0, 0), false);
    expect_eq(sched.submitRequest(1, 0, 4, 1, 0, 0), false);
    expect_eq(sched.submitRequest(1, 16, 0, 1, 0, 0), false);

    expect_true(sched.submitRequest(1, 32, 10, 10, 0, 0));
    expect_eq(sched.submitRequest(1, 32, 10, 10, 0, 0), false); // duplicate req_id rejected

    let plan = BatchStepPlan{};
    sched.scheduleStep(10, *plan);
    expect_eq(sched.stats().used_kv_blocks, @as(u32, 2));

    // Abort releases held KV blocks immediately
    expect_true(sched.abortRequest(1));
    expect_eq(sched.abortRequest(1), false); // already aborted
    expect_eq(sched.stats().used_kv_blocks, @as(u32, 0));
    expect_eq(sched.stats().aborted_count, @as(usize, 1));

    // Fill remaining 63 slots up to 64
    let id: u64 = 2;
    while (id <= @as(u64, BATCH_SCHED_MAX_REQS)) {
        expect_true(sched.submitRequest(id, 16, 5, 1, 10, 0));
        id += 1;
    }
    expect_eq(sched.submitRequest(999, 16, 5, 1, 10, 0), false); // 65th rejected

    // Reap aborted slot #1 -> allows submitting req 999
    expect_eq(sched.reapTerminal(), @as(usize, 1));
    expect_true(sched.submitRequest(999, 16, 5, 1, 10, 0));
}

test("batchSched: telemetry snapshot, early EOS release, reset, and cap.attention facade parity") {
    let cfg = cap.attention.BatchSchedConfig.defaults();
    cfg.total_kv_blocks = 16;
    let sched = cap.attention.ChunkedBatchScheduler.init(cfg) ?? return;

    expect_true(sched.submitRequest(500, 16, 100, 10, 1000, 0));
    let plan = cap.attention.BatchStepPlan{};
    sched.scheduleStep(1010, *plan);
    // Early EOS during prefill/first-step immediately finishes and releases KV block
    expect_true(sched.commitStepResult(500, 16, true, 1050));

    const st = sched.stats();
    expect_eq(st.total_steps, @as(u64, 1));
    expect_eq(st.total_completed, @as(u64, 1));
    expect_eq(st.used_kv_blocks, @as(u32, 0));
    expect_eq(st.free_kv_blocks, @as(u32, 16));
    expect_eq(st.avg_ttft_ms, @as(u64, 50));

    sched.reset();
    const st2 = sched.stats();
    expect_eq(st2.active_slots, @as(usize, 0));
    expect_eq(st2.total_steps, @as(u64, 0));
    expect_eq(st2.total_completed, @as(u64, 0));
}
