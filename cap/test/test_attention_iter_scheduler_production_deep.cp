// Origin: Pure-CP Stdlib Test Suite (lib/attention/iter_scheduler.cp) | Created: 2026-11-22
// Production Deep Verification Suite for vLLM/Sarathi-Style Continuous Batching Iteration Scheduler:
//   1. Multi-step Chunked Prefill progression, KV block delta accounting & Prefilling -> Decoding transition
//   2. Mixed Prefill + Decode step scheduling (Decode-first TPOT protection + remaining budget chunked prefill)
//   3. KV-block exhaustion preemption of lowest-priority sequence, deadlock prevention & full recompute recovery
//   4. Equal-priority recency preemption (newest enqueued victim) & KV-tight chunk shrinking
//   5. Early EOS finish, max_new_tokens finish, cancelRequest KV release & Finished slot recycling
//   6. Invalid config, empty buffer, duplicate req_id, full table & uncommitted commitStep defensive guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    IterSchedulerConfig,
    SeqRunState,
    SeqSchedEntry,
    ScheduledStepItem,
    IterStepPlan,
    IterSchedulerStats,
    ContinuousIterScheduler,
} from "cap:attention";

// ============================================================================
// Test 1: Multi-Step Chunked Prefill & Incremental KV Block Accounting
// ============================================================================

fn testChunkedPrefillMultiStep()! {
    let entries: [4]SeqSchedEntry = [SeqSchedEntry{}; 4];
    let entries_s: []SeqSchedEntry = entries[0..];
    const cfg = IterSchedulerConfig{
        max_batch_seqs: 4,
        max_token_budget_per_step: 6,
        max_kv_blocks: 8,
        block_size: 4,
    };
    let sched = cap.attention.ContinuousIterScheduler.init(entries_s, cfg) ?? return error.InitFailed;

    // Prompt = 14 tokens, budget = 6/step => chunks: 6, 6, 2; max_new_tokens = 2
    expect_eq(sched.enqueue(101, 14, 2, 1), true);

    let items: [4]ScheduledStepItem = [ScheduledStepItem{}; 4];
    let items_s: []ScheduledStepItem = items[0..];

    // Step 1: chunk 0..6 (6 tokens => ceil(6/4) = 2 KV blocks)
    const p1 = sched.scheduleStep(items_s);
    expect_eq(p1.step_id, 1);
    expect_eq(p1.num_items, 1);
    expect_eq(p1.num_prefill_seqs, 1);
    expect_eq(p1.num_decode_seqs, 0);
    expect_eq(p1.total_scheduled_tokens, 6);
    expect_eq(p1.kv_blocks_used, 2);
    expect_eq(items[0].req_id, 101);
    expect_eq(items[0].is_prefill, true);
    expect_eq(items[0].start_pos, 0);
    expect_eq(items[0].num_tokens, 6);
    expect_eq(items[0].new_kv_blocks, 2);
    expect_eq(sched.commitStep(101, 0, false), true);

    const e1 = sched.getEntry(101) ?? return error.MissingEntry;
    expect_eq(e1.state == SeqRunState.Prefilling, true);
    expect_eq(e1.prefilled_tokens, 6);
    expect_eq(e1.kv_blocks_held, 2);

    // Step 2: chunk 6..12 (6 tokens => ceil(12/4) = 3 KV blocks, delta = +1)
    const p2 = sched.scheduleStep(items_s);
    expect_eq(p2.num_items, 1);
    expect_eq(items[0].start_pos, 6);
    expect_eq(items[0].num_tokens, 6);
    expect_eq(items[0].new_kv_blocks, 1);
    expect_eq(p2.kv_blocks_used, 3);
    expect_eq(sched.commitStep(101, 0, false), true);

    // Step 3: final chunk 12..14 (2 tokens => ceil(14/4) = 4 KV blocks, delta = +1)
    const p3 = sched.scheduleStep(items_s);
    expect_eq(p3.num_items, 1);
    expect_eq(items[0].start_pos, 12);
    expect_eq(items[0].num_tokens, 2);
    expect_eq(items[0].new_kv_blocks, 1);
    expect_eq(p3.kv_blocks_used, 4);
    expect_eq(sched.commitStep(101, 0, false), true);

    // Must transition to Decoding now
    const e3 = sched.getEntry(101) ?? return error.MissingEntry;
    expect_eq(e3.state == SeqRunState.Decoding, true);
    expect_eq(e3.prefilled_tokens, 14);
    expect_eq(e3.kv_blocks_held, 4);

    // Step 4: Decode token 1 (pos 14 -> total 15 <= 16, delta_blocks = 0)
    const p4 = sched.scheduleStep(items_s);
    expect_eq(p4.num_decode_seqs, 1);
    expect_eq(p4.num_prefill_seqs, 0);
    expect_eq(items[0].is_prefill, false);
    expect_eq(items[0].start_pos, 14);
    expect_eq(items[0].num_tokens, 1);
    expect_eq(items[0].new_kv_blocks, 0);
    expect_eq(sched.commitStep(101, 1, false), true);

    // Step 5: Decode token 2 (pos 15 -> total 16, delta_blocks = 0) -> reaches max_new_tokens = 2 => Finished!
    const p5 = sched.scheduleStep(items_s);
    expect_eq(p5.num_decode_seqs, 1);
    expect_eq(items[0].start_pos, 15);
    expect_eq(sched.commitStep(101, 1, false), true);

    const e5 = sched.getEntry(101) ?? return error.MissingEntry;
    expect_eq(e5.state == SeqRunState.Finished, true);
    expect_eq(e5.decoded_tokens, 2);
    expect_eq(e5.kv_blocks_held, 0);
    expect_eq(sched.kvBlocksUsed(), 0);
    expect_eq(sched.kvBlocksFree(), 8);

    const st = sched.stats();
    expect_eq(st.total_prefill_tokens, 14);
    expect_eq(st.total_decode_tokens, 2);
    expect_eq(st.total_finished, 1);
    expect_eq(st.active_seqs, 0);
}

// ============================================================================
// Test 2: Mixed Prefill + Decode Continuous Batching Step (Sarathi-Serve)
// ============================================================================

fn testMixedPrefillAndDecodeStep()! {
    let entries: [8]SeqSchedEntry = [SeqSchedEntry{}; 8];
    let entries_s: []SeqSchedEntry = entries[0..];
    const cfg = IterSchedulerConfig{
        max_batch_seqs: 4,
        max_token_budget_per_step: 8,
        max_kv_blocks: 16,
        block_size: 4,
    };
    let sched = ContinuousIterScheduler.init(entries_s, cfg) ?? return error.InitFailed;

    // Enqueue req 1 (short prompt 4, prio 10) and req 2 (long prompt 12, prio 1)
    expect_eq(sched.enqueue(1, 4, 3, 10), true);
    expect_eq(sched.enqueue(2, 12, 2, 1), true);

    let items: [4]ScheduledStepItem = [ScheduledStepItem{}; 4];
    let items_s: []ScheduledStepItem = items[0..];

    // Step 1: Budget = 8. Req 1 (prio 10) takes 4 prefill tokens; remaining 4 goes to req 2 chunked prefill.
    const p1 = sched.scheduleStep(items_s);
    expect_eq(p1.num_items, 2);
    expect_eq(p1.num_prefill_seqs, 2);
    expect_eq(p1.num_decode_seqs, 0);
    expect_eq(p1.total_scheduled_tokens, 8);
    expect_eq(items[0].req_id, 1);
    expect_eq(items[0].num_tokens, 4);
    expect_eq(items[1].req_id, 2);
    expect_eq(items[1].num_tokens, 4);

    expect_eq(sched.commitStep(1, 0, false), true);
    expect_eq(sched.commitStep(2, 0, false), true);

    // Enqueue req 3 (prompt 6, prio 5) while req 1 is Decoding and req 2 is Prefilling
    expect_eq(sched.enqueue(3, 6, 1, 5), true);

    // Step 2: Mixed step!
    //   Phase 1 (Decode-first): req 1 takes 1 decode token (rem budget = 7).
    //   Phase 2 (Chunked Prefill): req 3 (prio 5) > req 2 (prio 1) => req 3 takes 6 tokens (rem = 1),
    //                              req 2 takes remaining 1 token!
    const p2 = sched.scheduleStep(items_s);
    expect_eq(p2.num_items, 3);
    expect_eq(p2.num_decode_seqs, 1);
    expect_eq(p2.num_prefill_seqs, 2);
    expect_eq(p2.total_scheduled_tokens, 8);

    expect_eq(items[0].req_id, 1);
    expect_eq(items[0].is_prefill, false);
    expect_eq(items[0].num_tokens, 1);
    expect_eq(items[0].start_pos, 4);

    expect_eq(items[1].req_id, 3);
    expect_eq(items[1].is_prefill, true);
    expect_eq(items[1].num_tokens, 6);
    expect_eq(items[1].start_pos, 0);

    expect_eq(items[2].req_id, 2);
    expect_eq(items[2].is_prefill, true);
    expect_eq(items[2].num_tokens, 1);
    expect_eq(items[2].start_pos, 4);

    expect_eq(sched.commitStep(1, 1, false), true);
    expect_eq(sched.commitStep(3, 0, false), true);
    expect_eq(sched.commitStep(2, 0, false), true);

    const e2 = sched.getEntry(2) ?? return error.MissingEntry;
    expect_eq(e2.prefilled_tokens, 5);
    expect_eq(e2.state == SeqRunState.Prefilling, true);

    const e3 = sched.getEntry(3) ?? return error.MissingEntry;
    expect_eq(e3.state == SeqRunState.Decoding, true);
}

// ============================================================================
// Test 3: KV Exhaustion Preemption (Lowest Priority) & Full Recovery
// ============================================================================

fn testKvPreemptionAndRecovery()! {
    let entries: [4]SeqSchedEntry = [SeqSchedEntry{}; 4];
    let entries_s: []SeqSchedEntry = entries[0..];
    // Tight KV pool: only 4 blocks * block_size 4 = 16 total token slots
    const cfg = IterSchedulerConfig{
        max_batch_seqs: 4,
        max_token_budget_per_step: 16,
        max_kv_blocks: 4,
        block_size: 4,
    };
    let sched = ContinuousIterScheduler.init(entries_s, cfg) ?? return error.InitFailed;

    // Req 10 (high priority = 10): prompt 8 (2 blocks), max_new = 2
    // Req 20 (low priority  =  1): prompt 8 (2 blocks), max_new = 2
    expect_eq(sched.enqueue(10, 8, 2, 10), true);
    expect_eq(sched.enqueue(20, 8, 2, 1), true);

    let items: [4]ScheduledStepItem = [ScheduledStepItem{}; 4];
    let items_s: []ScheduledStepItem = items[0..];

    // Step 1: Both prefill fully, consuming 2 + 2 = 4 KV blocks (pool is now 100% full!)
    const p1 = sched.scheduleStep(items_s);
    expect_eq(p1.num_items, 2);
    expect_eq(p1.kv_blocks_used, 4);
    expect_eq(p1.kv_blocks_free, 0);
    expect_eq(sched.commitStep(10, 0, false), true);
    expect_eq(sched.commitStep(20, 0, false), true);

    // Step 2: Both are Decoding at pos 8 -> crossing block boundary needs 3rd KV block!
    // Pool has 0 free blocks => Req 10 (prio 10) preempts Req 20 (prio 1), reclaiming 2 blocks!
    // Req 10 allocates 1 block (now holds 3, free = 1). Req 20 stays Preempted in this step.
    const p2 = sched.scheduleStep(items_s);
    expect_eq(p2.preempted_count, 1);
    expect_eq(p2.num_decode_seqs, 1);
    expect_eq(p2.num_prefill_seqs, 0);
    expect_eq(p2.num_items, 1);
    expect_eq(items[0].req_id, 10);
    expect_eq(items[0].is_prefill, false);
    expect_eq(items[0].new_kv_blocks, 1);
    expect_eq(p2.kv_blocks_used, 3);
    expect_eq(p2.kv_blocks_free, 1);

    const e20_preempted = sched.getEntry(20) ?? return error.MissingEntry;
    expect_eq(e20_preempted.state == SeqRunState.Preempted, true);
    expect_eq(e20_preempted.kv_blocks_held, 0);
    expect_eq(e20_preempted.prefilled_tokens, 0);
    expect_eq(e20_preempted.preempt_count, 1);

    expect_eq(sched.commitStep(10, 1, false), true);

    // Step 3: Req 10 decodes 2nd (final) token (pos 9, delta_blocks = 0).
    // Meanwhile 1 KV block is free => Req 20 resumes via Chunked Prefill (shrunk to 4 tokens = 1 block)!
    const p3 = sched.scheduleStep(items_s);
    expect_eq(p3.num_decode_seqs, 1);
    expect_eq(p3.num_prefill_seqs, 1);
    expect_eq(items[0].req_id, 10);
    expect_eq(items[0].is_prefill, false);
    expect_eq(items[1].req_id, 20);
    expect_eq(items[1].is_prefill, true);
    expect_eq(items[1].start_pos, 0);
    expect_eq(items[1].num_tokens, 4); // Shrunk to fit the 1 free KV block!
    expect_eq(items[1].new_kv_blocks, 1);

    // Commit both: Req 10 hits max_new_tokens = 2 => Finished, releases its 3 KV blocks!
    expect_eq(sched.commitStep(10, 1, false), true);
    expect_eq(sched.commitStep(20, 0, false), true);

    const e10_done = sched.getEntry(10) ?? return error.MissingEntry;
    expect_eq(e10_done.state == SeqRunState.Finished, true);
    expect_eq(e10_done.kv_blocks_held, 0);
    expect_eq(sched.kvBlocksUsed(), 1); // Only Req 20's 1 block remains

    // Step 4: Req 20 finishes remaining 4 prefill tokens (4..8)
    const p4 = sched.scheduleStep(items_s);
    expect_eq(p4.num_items, 1);
    expect_eq(items[0].req_id, 20);
    expect_eq(items[0].is_prefill, true);
    expect_eq(items[0].start_pos, 4);
    expect_eq(items[0].num_tokens, 4);
    expect_eq(sched.commitStep(20, 0, false), true);

    // Steps 5 & 6: Req 20 decodes its 2 tokens and finishes cleanly
    const p5 = sched.scheduleStep(items_s);
    expect_eq(p5.num_decode_seqs, 1);
    expect_eq(items[0].req_id, 20);
    expect_eq(sched.commitStep(20, 1, false), true);

    const p6 = sched.scheduleStep(items_s);
    expect_eq(p6.num_decode_seqs, 1);
    expect_eq(sched.commitStep(20, 1, false), true);

    const st = sched.stats();
    expect_eq(st.total_preemptions, 1);
    expect_eq(st.total_finished, 2);
    expect_eq(st.kv_blocks_used, 0);
    expect_eq(st.kv_blocks_free, 4);
    expect_eq(st.active_seqs, 0);
}

// ============================================================================
// Test 4: Equal-Priority Recency Preemption (Newest Enqueued Victim)
// ============================================================================

fn testEqualPriorityRecencyPreemption()! {
    let entries: [4]SeqSchedEntry = [SeqSchedEntry{}; 4];
    let entries_s: []SeqSchedEntry = entries[0..];
    // 3 blocks * block_size 4 = 12 tokens
    const cfg = IterSchedulerConfig{
        max_batch_seqs: 4,
        max_token_budget_per_step: 16,
        max_kv_blocks: 3,
        block_size: 4,
    };
    let sched = ContinuousIterScheduler.init(entries_s, cfg) ?? return error.InitFailed;

    // Three requests with identical priority = 5, each taking 1 block (4 tokens)
    expect_eq(sched.enqueue(1, 4, 2, 5), true); // oldest
    expect_eq(sched.enqueue(2, 4, 2, 5), true); // middle
    expect_eq(sched.enqueue(3, 4, 2, 5), true); // newest

    let items: [4]ScheduledStepItem = [ScheduledStepItem{}; 4];
    let items_s: []ScheduledStepItem = items[0..];

    // Step 1: All 3 prefill (3 blocks used, 0 free)
    const p1 = sched.scheduleStep(items_s);
    expect_eq(p1.num_items, 3);
    expect_eq(p1.kv_blocks_free, 0);
    expect_eq(sched.commitStep(1, 0, false), true);
    expect_eq(sched.commitStep(2, 0, false), true);
    expect_eq(sched.commitStep(3, 0, false), true);

    // Step 2: Req 1 (oldest) decodes and needs a 2nd block -> must preempt newest (Req 3)!
    // Then Req 2 also needs a 2nd block -> no newer victim left, so Req 2 is skipped this step.
    const p2 = sched.scheduleStep(items_s);
    expect_eq(p2.preempted_count, 1);
    expect_eq(p2.num_decode_seqs, 1);
    expect_eq(items[0].req_id, 1);

    const e3 = sched.getEntry(3) ?? return error.MissingEntry;
    expect_eq(e3.state == SeqRunState.Preempted, true);
    expect_eq(e3.kv_blocks_held, 0);

    const e2 = sched.getEntry(2) ?? return error.MissingEntry;
    expect_eq(e2.state == SeqRunState.Decoding, true);
    expect_eq(e2.kv_blocks_held, 1);
}

// ============================================================================
// Test 5: EOS Early Finish, Cancellation & Finished Slot Recycling
// ============================================================================

fn testEosCancelAndSlotRecycling()! {
    // Tight 2-slot entry table to verify recycling of Finished slots
    let entries: [2]SeqSchedEntry = [SeqSchedEntry{}; 2];
    let entries_s: []SeqSchedEntry = entries[0..];
    const cfg = IterSchedulerConfig{
        max_batch_seqs: 2,
        max_token_budget_per_step: 8,
        max_kv_blocks: 8,
        block_size: 4,
    };
    let sched = ContinuousIterScheduler.init(entries_s, cfg) ?? return error.InitFailed;

    expect_eq(sched.enqueue(10, 4, 10, 1), true);
    expect_eq(sched.enqueue(20, 4, 10, 1), true);
    // Table full with 2 active non-finished requests
    expect_eq(sched.enqueue(30, 4, 10, 1), false);

    let items: [2]ScheduledStepItem = [ScheduledStepItem{}; 2];
    let items_s: []ScheduledStepItem = items[0..];

    // Step 1: Prefill both; req 10 emits first token + hits EOS immediately!
    const p1 = sched.scheduleStep(items_s);
    expect_eq(p1.num_items, 2);
    expect_eq(p1.kv_blocks_used, 2);
    expect_eq(sched.commitStep(10, 1, true), true);
    expect_eq(sched.commitStep(20, 0, false), true);

    const e10 = sched.getEntry(10) ?? return error.MissingEntry;
    expect_eq(e10.state == SeqRunState.Finished, true);
    expect_eq(e10.finish_reason_eos, true);
    expect_eq(e10.decoded_tokens, 1);
    expect_eq(e10.kv_blocks_held, 0);
    expect_eq(sched.kvBlocksUsed(), 1);

    // Cancel req 20 mid-flight -> frees its 1 KV block immediately
    expect_eq(sched.cancelRequest(20), true);
    expect_eq(sched.cancelRequest(20), false); // Already finished
    expect_eq(sched.kvBlocksUsed(), 0);
    expect_eq(sched.activeCount(), 0);

    // Both slots are Finished => enqueueing req 30 and req 40 recycles both slots!
    expect_eq(sched.enqueue(30, 4, 0, 2), true); // max_new_tokens = 0 => finishes right after prefill
    expect_eq(sched.enqueue(40, 4, 2, 2), true);
    expect_eq(sched.activeCount(), 2);

    const p2 = sched.scheduleStep(items_s);
    expect_eq(p2.num_items, 2);
    expect_eq(sched.commitStep(30, 0, false), true); // max_new_tokens=0 -> immediate Finish
    expect_eq(sched.commitStep(40, 0, false), true);

    const e30 = sched.getEntry(30) ?? return error.MissingEntry;
    expect_eq(e30.state == SeqRunState.Finished, true);
    expect_eq(e30.kv_blocks_held, 0);
    expect_eq(sched.kvBlocksUsed(), 1);
}

// ============================================================================
// Test 6: Defensive Boundary & Invalid Input Guards
// ============================================================================

fn testDefensiveGuards()! {
    let entries: [2]SeqSchedEntry = [SeqSchedEntry{}; 2];
    let entries_s: []SeqSchedEntry = entries[0..];
    let empty_entries: []SeqSchedEntry = entries[0..0];

    // Invalid configs & empty buffer
    expect_eq(ContinuousIterScheduler.init(empty_entries, IterSchedulerConfig.default()) == null, true);
    expect_eq(ContinuousIterScheduler.init(entries_s, IterSchedulerConfig{ max_batch_seqs: 0 }) == null, true);
    expect_eq(ContinuousIterScheduler.init(entries_s, IterSchedulerConfig{ max_token_budget_per_step: 0 }) == null, true);
    expect_eq(ContinuousIterScheduler.init(entries_s, IterSchedulerConfig{ max_kv_blocks: 0 }) == null, true);
    expect_eq(ContinuousIterScheduler.init(entries_s, IterSchedulerConfig{ block_size: 0 }) == null, true);

    const cfg = IterSchedulerConfig{
        max_batch_seqs: 2,
        max_token_budget_per_step: 8,
        max_kv_blocks: 4,
        block_size: 4,
    };
    let sched = ContinuousIterScheduler.init(entries_s, cfg) ?? return error.InitFailed;

    // Zero prompt_tokens rejected
    expect_eq(sched.enqueue(1, 0, 4, 0), false);

    // Valid enqueue & duplicate active req_id rejected
    expect_eq(sched.enqueue(7, 4, 4, 0), true);
    expect_eq(sched.enqueue(7, 4, 4, 0), false);

    // commitStep before scheduleStep (not in-flight) rejected
    expect_eq(sched.commitStep(7, 1, false), false);
    // commitStep on unknown req_id rejected
    expect_eq(sched.commitStep(999, 1, false), false);

    // Empty out_items buffer safely schedules 0 items without panic
    let items: [2]ScheduledStepItem = [ScheduledStepItem{}; 2];
    let empty_items: []ScheduledStepItem = items[0..0];
    const p_empty = sched.scheduleStep(empty_items);
    expect_eq(p_empty.num_items, 0);
    expect_eq(p_empty.total_scheduled_tokens, 0);
    expect_eq(sched.kvBlocksUsed(), 0);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("attention/iter_scheduler: multi-step chunked prefill & incremental KV accounting", testChunkedPrefillMultiStep);
test("attention/iter_scheduler: mixed prefill + decode continuous batching step", testMixedPrefillAndDecodeStep);
test("attention/iter_scheduler: KV exhaustion preemption & full recompute recovery", testKvPreemptionAndRecovery);
test("attention/iter_scheduler: equal-priority recency preemption (newest victim)", testEqualPriorityRecencyPreemption);
test("attention/iter_scheduler: EOS finish, cancellation & finished slot recycling", testEosCancelAndSlotRecycling);
test("attention/iter_scheduler: invalid configs, duplicates & empty buffer guards", testDefensiveGuards);
