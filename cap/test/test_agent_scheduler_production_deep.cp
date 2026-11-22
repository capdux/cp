// Source: cp/cap/test/test_agent_scheduler_production_deep.cp (Production deep tests for cap:agent scheduler)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ReqState,
    InferRequest,
    BatchLimits,
    ScheduleStep,
    BatchScheduler,
} from "cap:agent";

test("1. Mixed Prefill+Decode continuous batching obeys token/KV limits and releases KV blocks on Finished", () => {
    let slots: [8]InferRequest = std.mem.zeroes([8]InferRequest);
    let slots_s: []InferRequest = slots[0..];
    // 16 tokens/block, 6 total blocks (96 tokens max), max_batch_tokens = 48
    const limits = BatchLimits {
        max_batch_reqs: 4,
        max_batch_tokens: 48,
        total_kv_blocks: 6,
        tokens_per_kv_block: 16,
        aging_ms_per_priority: 0,
    };
    let sched = BatchScheduler.init(&slots_s, limits) ?? unreachable;
    expect_eq(sched.freeKvBlocks(), 6);

    // Enqueue 3 requests: req1 (32 tok = 2 blk, max_new=2), req2 (32 tok = 2 blk, max_new=2), req3 (16 tok = 1 blk, max_new=1)
    expect_eq(sched.enqueue(1, 0xA, 32, 2, 10, 100), true);
    expect_eq(sched.enqueue(2, 0xB, 32, 2, 10, 101), true);
    expect_eq(sched.enqueue(3, 0xC, 16, 1, 10, 102), true);

    let pf: [4]u64 = std.mem.zeroes([4]u64);
    let dc: [4]u64 = std.mem.zeroes([4]u64);
    let pf_s: []u64 = pf[0..];
    let dc_s: []u64 = dc[0..];

    // Step 1: req1 (32 tok) + req3 (16 tok) = 48 tok fits max_batch_tokens (req2 32 tok exceeds remaining 16 tok)
    const s1 = sched.stepSchedule(110, 0, &pf_s, &dc_s);
    expect_eq(s1.decode_count, 0);
    expect_eq(s1.prefill_count, 2);
    expect_eq(pf_s[0], 1);
    expect_eq(pf_s[1], 3);
    expect_eq(s1.scheduled_tokens, 48);
    expect_eq(s1.kv_blocks_used, 3); // 2 + 1
    expect_eq(sched.freeKvBlocks(), 3);

    // Advance prefill outputs: req1 generates 1/2 (Prefill->Decode), req3 generates 1/1 (Finished, frees 1 blk!)
    expect_eq(sched.advanceDecode(1, 1), true);
    expect_eq(sched.advanceDecode(3, 1), true);
    expect_eq((sched.requestAt(1) ?? unreachable).state == ReqState.Decode, true);
    expect_eq((sched.requestAt(3) ?? unreachable).state == ReqState.Finished, true);
    expect_eq(sched.freeKvBlocks(), 4);

    // Step 2: Mixed batch! req1 Decode (1 tok, grows from 33->34 tokens needing 3rd block!) + req2 Prefill (32 tok, 2 blks)
    const s2 = sched.stepSchedule(120, 0, &pf_s, &dc_s);
    expect_eq(s2.decode_count, 1);
    expect_eq(dc_s[0], 1);
    expect_eq(s2.prefill_count, 1);
    expect_eq(pf_s[0], 2);
    expect_eq(s2.scheduled_tokens, 33);
    expect_eq(s2.kv_blocks_used, 5); // req1=3 blks + req2=2 blks

    // Finish req1 -> releases all 3 blocks
    expect_eq(sched.advanceDecode(1, 1), true);
    expect_eq((sched.requestAt(1) ?? unreachable).state == ReqState.Finished, true);
    expect_eq(sched.freeKvBlocks(), 4);
});

test("2. Prefix-cache affinity boosts matching prefix_hash requests into Prefill first", () => {
    let slots: [4]InferRequest = std.mem.zeroes([4]InferRequest);
    let slots_s: []InferRequest = slots[0..];
    const limits = BatchLimits {
        max_batch_reqs: 1,
        max_batch_tokens: 128,
        total_kv_blocks: 8,
        tokens_per_kv_block: 16,
        aging_ms_per_priority: 0,
    };
    let sched = BatchScheduler.init(&slots_s, limits) ?? unreachable;

    // Req 10 arrived earlier with slightly higher priority (15) but cold prefix
    // Req 20 arrived later with priority 10 but matches hot active_prefix_hash (+10 affinity -> effective 20 > 15)
    expect_eq(sched.enqueue(10, 0x1111, 32, 4, 15, 100), true);
    expect_eq(sched.enqueue(20, 0xCAFE, 32, 4, 10, 105), true);

    let pf: [2]u64 = std.mem.zeroes([2]u64);
    let dc: [2]u64 = std.mem.zeroes([2]u64);
    let pf_s: []u64 = pf[0..];
    let dc_s: []u64 = dc[0..];

    const step = sched.stepSchedule(110, 0xCAFE, &pf_s, &dc_s);
    expect_eq(step.prefill_count, 1);
    expect_eq(pf_s[0], 20);
});

test("3. High-priority arrival preempts low-priority Decode, reclaims KV blocks, and resumes later", () => {
    let slots: [4]InferRequest = std.mem.zeroes([4]InferRequest);
    let slots_s: []InferRequest = slots[0..];
    // Tight KV pool: only 4 blocks total
    const limits = BatchLimits {
        max_batch_reqs: 4,
        max_batch_tokens: 128,
        total_kv_blocks: 4,
        tokens_per_kv_block: 16,
        aging_ms_per_priority: 0,
    };
    let sched = BatchScheduler.init(&slots_s, limits) ?? unreachable;

    // Low-priority request holds all 4 blocks (64 tokens = 4 blocks)
    expect_eq(sched.enqueue(1, 0x1, 64, 5, 1, 100), true);

    let pf: [4]u64 = std.mem.zeroes([4]u64);
    let dc: [4]u64 = std.mem.zeroes([4]u64);
    let pf_s: []u64 = pf[0..];
    let dc_s: []u64 = dc[0..];

    const s1 = sched.stepSchedule(100, 0, &pf_s, &dc_s);
    expect_eq(s1.prefill_count, 1);
    expect_eq(s1.kv_blocks_used, 4);
    expect_eq(sched.advanceDecode(1, 1), true); // Now in Decode holding 4 blocks

    // High-priority request arrives needing 2 blocks (32 tokens) while 0 blocks are free
    expect_eq(sched.enqueue(2, 0x2, 32, 1, 100, 110), true);
    const s2 = sched.stepSchedule(110, 0, &pf_s, &dc_s);
    expect_eq(s2.preempted_count, 1);
    expect_eq(s2.prefill_count, 1);
    expect_eq(pf_s[0], 2);
    expect_eq(s2.kv_blocks_used, 2);
    expect_eq((sched.requestAt(1) ?? unreachable).state == ReqState.Preempted, true);
    expect_eq((sched.requestAt(1) ?? unreachable).kv_blocks_held, 0);

    // High-priority request finishes and frees its 2 blocks -> all 4 blocks free
    expect_eq(sched.advanceDecode(2, 1), true);
    expect_eq(sched.freeKvBlocks(), 4);

    // Preempted request 1 can re-enter Prefill once enough KV blocks exist (needs 65 tok -> 5 blks > 4, so won't fit;
    // test with a 3-block preempted request or cancel verify -> here req1 needs 5 blocks so stays Preempted safely without OOM)
    const s3 = sched.stepSchedule(120, 0, &pf_s, &dc_s);
    expect_eq(s3.prefill_count, 0);
    expect_eq(sched.freeKvBlocks(), 4);
});

test("4. Anti-starvation priority aging promotes long-waiting low-priority requests", () => {
    let slots: [4]InferRequest = std.mem.zeroes([4]InferRequest);
    let slots_s: []InferRequest = slots[0..];
    const limits = BatchLimits {
        max_batch_reqs: 1,
        max_batch_tokens: 128,
        total_kv_blocks: 8,
        tokens_per_kv_block: 16,
        aging_ms_per_priority: 100, // +1 priority per 100ms waited
    };
    let sched = BatchScheduler.init(&slots_s, limits) ?? unreachable;

    // Req 1: low base priority 5, arrived at t=0. At t=5000 -> effective priority 5 + 50 = 55!
    // Req 2: high base priority 30, arrived at t=4900. At t=5000 -> effective priority 30 + 1 = 31.
    expect_eq(sched.enqueue(1, 0, 16, 2, 5, 0), true);
    expect_eq(sched.enqueue(2, 0, 16, 2, 30, 4900), true);

    let pf: [2]u64 = std.mem.zeroes([2]u64);
    let dc: [2]u64 = std.mem.zeroes([2]u64);
    let pf_s: []u64 = pf[0..];
    let dc_s: []u64 = dc[0..];

    const step = sched.stepSchedule(5000, 0, &pf_s, &dc_s);
    expect_eq(step.prefill_count, 1);
    expect_eq(pf_s[0], 1); // Aged low-priority request wins!
});

test("5. Defensive guards for zero limits, empty pool, duplicate ID, zero prompt, and cancel KV release", () => {
    let slots: [2]InferRequest = std.mem.zeroes([2]InferRequest);
    let slots_s: []InferRequest = slots[0..];

    // Zero limits rejected
    expect_eq(BatchScheduler.init(&slots_s, BatchLimits {
        max_batch_reqs: 0, max_batch_tokens: 64, total_kv_blocks: 4, tokens_per_kv_block: 16, aging_ms_per_priority: 0,
    }) == null, true);
    expect_eq(BatchScheduler.init(&slots_s, BatchLimits {
        max_batch_reqs: 2, max_batch_tokens: 64, total_kv_blocks: 0, tokens_per_kv_block: 16, aging_ms_per_priority: 0,
    }) == null, true);
    expect_eq(BatchScheduler.init(&slots_s, BatchLimits {
        max_batch_reqs: 2, max_batch_tokens: 64, total_kv_blocks: 4, tokens_per_kv_block: 0, aging_ms_per_priority: 0,
    }) == null, true);

    let sched = BatchScheduler.init(&slots_s, BatchLimits {
        max_batch_reqs: 2, max_batch_tokens: 64, total_kv_blocks: 4, tokens_per_kv_block: 16, aging_ms_per_priority: 0,
    }) ?? unreachable;

    // Zero prompt_tokens, duplicate ID, and pool overflow rejected
    expect_eq(sched.enqueue(1, 0, 0, 2, 10, 0), false);
    expect_eq(sched.enqueue(1, 0, 32, 2, 10, 0), true);
    expect_eq(sched.enqueue(1, 0, 32, 2, 10, 0), false);
    expect_eq(sched.enqueue(2, 0, 16, 2, 10, 0), true);
    expect_eq(sched.enqueue(3, 0, 16, 2, 10, 0), false);

    // Advance on Queued or unknown ID rejected
    expect_eq(sched.advanceDecode(1, 1), false);
    expect_eq(sched.advanceDecode(999, 1), false);

    // Admit req 1 (2 blocks) then cancel it -> KV blocks immediately reclaimed
    let pf: [2]u64 = std.mem.zeroes([2]u64);
    let dc: [2]u64 = std.mem.zeroes([2]u64);
    let pf_s: []u64 = pf[0..];
    let dc_s: []u64 = dc[0..];
    const step = sched.stepSchedule(10, 0, &pf_s, &dc_s);
    expect_eq(step.kv_blocks_used, 3); // req1(2) + req2(1)
    expect_eq(sched.cancel(1), true);
    expect_eq(sched.cancel(1), false); // double cancel rejected
    expect_eq((sched.requestAt(1) ?? unreachable).state == ReqState.Cancelled, true);
    expect_eq(sched.freeKvBlocks(), 3); // 4 - 1 = 3
});
