// Origin: Pure-CP Stdlib Test Suite (lib/prompt/contextWindow.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Tool-Pair-Safe Context Window Compactor & Prefix-Cache Planner:
//   1. Under-budget zero eviction, `effectiveInputBudget()` & `validateToolPairIntegrity` negative/positive guards
//   2. Atomic `tool_call` / `tool_result` indivisibility & strict chronological ascending `kept_indices`
//   3. `preserve_prefix_and_tail` maximizes KV-cache prefix retention (`prefix_cache_ratio_bps`) vs `sliding_recent`
//   4. `drop_tool_outputs_first` evicts older complete tool groups before regular dialogue turns
//   5. Unconditional `system` and `pinned` protection (including pinned atomic tool groups)
//   6. Stateful `ContextWindowTracker` multi-turn lifecycle, `applyCompaction` summary injection & facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CTX_WIN_MAX_TURNS,
    CTX_WIN_BPS_SCALE,
    CtxTurnRole,
    CtxEvictStrategy,
    CtxTurnMeta,
    CtxWindowConfig,
    validateToolPairIntegrity,
    planContextCompaction,
    ContextWindowTracker,
} from "cap:prompt";

test("ContextWindow: under-budget zero eviction & validateToolPairIntegrity guards") {
    const turns: [4]CtxTurnMeta = [
        CtxTurnMeta{ turn_id: 1, role: CtxTurnRole.system, group_id: 0, tokens: 200 },
        CtxTurnMeta{ turn_id: 2, role: CtxTurnRole.user, group_id: 0, tokens: 100 },
        CtxTurnMeta{ turn_id: 3, role: CtxTurnRole.tool_call, group_id: 10, tokens: 80 },
        CtxTurnMeta{ turn_id: 4, role: CtxTurnRole.tool_result, group_id: 10, tokens: 120 },
    ];

    let cfg = CtxWindowConfig.defaults();
    cfg.max_input_tokens = 1000;
    cfg.reserve_output_tokens = 200; // effective budget = 800 > 500
    expect_eq(cfg.effectiveInputBudget(), @as(u32, 800));

    const plan = planContextCompaction(turns[0..], cfg);
    expect_eq(plan.kept_count, @as(usize, 4));
    expect_eq(plan.evicted_count, @as(usize, 0));
    expect_eq(plan.kept_tokens, @as(u32, 500));
    expect_eq(plan.evicted_tokens, @as(u32, 0));
    expect_eq(plan.preserved_prefix_tokens, @as(u32, 500));
    expect_eq(plan.prefix_cache_ratio_bps, @as(u16, @intCast(CTX_WIN_BPS_SCALE)));
    expect_eq(plan.atomic_groups_evicted, @as(usize, 0));
    expect_true(!plan.needs_summary);
    expect_true(validateToolPairIntegrity(turns[0..], plan.kept_indices[0..], plan.kept_count));

    // Negative integrity checks: orphaned tool_call or orphaned tool_result must fail
    const orphan_call: [3]usize = [0, 1, 2];
    const orphan_res: [3]usize = [0, 1, 3];
    expect_true(!validateToolPairIntegrity(turns[0..], orphan_call[0..], 3));
    expect_true(!validateToolPairIntegrity(turns[0..], orphan_res[0..], 3));

    // Dropping the whole tool pair [2, 3] is valid
    const valid_without_tool: [2]usize = [0, 1];
    expect_true(validateToolPairIntegrity(turns[0..], valid_without_tool[0..], 2));
}

test("ContextWindow: atomic tool_call/tool_result group never splits & ascending order preserved") {
    // Turn layout:
    //  0: system (200)
    //  1: user (100)
    //  2: tool_call   group=10 (100)
    //  3: tool_result group=10 (300)  <-- atomic pair total = 400
    //  4: tool_call   group=20 (100)
    //  5: tool_result group=20 (150)  <-- atomic pair total = 250
    //  6: assistant (150)
    // Total = 1100 tokens. Effective budget = 750 -> must evict group 10 atomically (leaving 700 <= 750).
    const turns: [7]CtxTurnMeta = [
        CtxTurnMeta{ turn_id: 1, role: CtxTurnRole.system, group_id: 0, tokens: 200 },
        CtxTurnMeta{ turn_id: 2, role: CtxTurnRole.user, group_id: 0, tokens: 100 },
        CtxTurnMeta{ turn_id: 3, role: CtxTurnRole.tool_call, group_id: 10, tokens: 100 },
        CtxTurnMeta{ turn_id: 4, role: CtxTurnRole.tool_result, group_id: 10, tokens: 300 },
        CtxTurnMeta{ turn_id: 5, role: CtxTurnRole.tool_call, group_id: 20, tokens: 100 },
        CtxTurnMeta{ turn_id: 6, role: CtxTurnRole.tool_result, group_id: 20, tokens: 150 },
        CtxTurnMeta{ turn_id: 7, role: CtxTurnRole.assistant, group_id: 0, tokens: 150 },
    ];

    let cfg = CtxWindowConfig.defaults();
    cfg.max_input_tokens = 950;
    cfg.reserve_output_tokens = 200; // budget = 750
    cfg.prefix_anchor_turns = 2;     // protects [0, 1] as prefix anchor
    cfg.min_recent_turns = 3;        // protects [4, 5, 6] as recent tail
    cfg.strategy = CtxEvictStrategy.preserve_prefix_and_tail;

    const plan = planContextCompaction(turns[0..], cfg);
    expect_true(plan.kept_tokens <= 750);
    expect_eq(plan.evicted_count, @as(usize, 2)); // both index 2 and 3 evicted together
    expect_eq(plan.atomic_groups_evicted, @as(usize, 1));
    expect_eq(plan.kept_count, @as(usize, 5));
    expect_true(plan.needs_summary);
    expect_true(validateToolPairIntegrity(turns[0..], plan.kept_indices[0..], plan.kept_count));

    // Verify exact kept indices [0, 1, 4, 5, 6] in strict ascending order
    expect_eq(plan.kept_indices[0], @as(usize, 0));
    expect_eq(plan.kept_indices[1], @as(usize, 1));
    expect_eq(plan.kept_indices[2], @as(usize, 4));
    expect_eq(plan.kept_indices[3], @as(usize, 5));
    expect_eq(plan.kept_indices[4], @as(usize, 6));
}

test("ContextWindow: preserve_prefix_and_tail maximizes prefix_cache_ratio_bps vs sliding_recent") {
    // Turn layout:
    //  0: system (200)
    //  1: user (300)    <- prefix anchor (not pinned, so sliding_recent will evict it!)
    //  2: assistant (250) <- middle
    //  3: user (250)      <- middle
    //  4: assistant (200) <- tail
    // Total = 1200 tokens. Effective budget = 950.
    const turns: [5]CtxTurnMeta = [
        CtxTurnMeta{ turn_id: 1, role: CtxTurnRole.system, group_id: 0, tokens: 200 },
        CtxTurnMeta{ turn_id: 2, role: CtxTurnRole.user, group_id: 0, tokens: 300 },
        CtxTurnMeta{ turn_id: 3, role: CtxTurnRole.assistant, group_id: 0, tokens: 250 },
        CtxTurnMeta{ turn_id: 4, role: CtxTurnRole.user, group_id: 0, tokens: 250 },
        CtxTurnMeta{ turn_id: 5, role: CtxTurnRole.assistant, group_id: 0, tokens: 200 },
    ];

    let cfg_prefix = CtxWindowConfig.defaults();
    cfg_prefix.max_input_tokens = 1150;
    cfg_prefix.reserve_output_tokens = 200; // budget = 950
    cfg_prefix.prefix_anchor_turns = 2;
    cfg_prefix.min_recent_turns = 1;
    cfg_prefix.strategy = CtxEvictStrategy.preserve_prefix_and_tail;

    let cfg_sliding = cfg_prefix;
    cfg_sliding.strategy = CtxEvictStrategy.sliding_recent;

    const p_prefix = planContextCompaction(turns[0..], cfg_prefix);
    const p_sliding = planContextCompaction(turns[0..], cfg_sliding);

    // preserve_prefix_and_tail evicts index 2 (middle), keeping unbroken prefix [0, 1] = 500 tokens
    expect_eq(p_prefix.preserved_prefix_tokens, @as(u32, 500));
    // sliding_recent evicts index 1 (oldest non-system), breaking KV prefix after index 0 = 200 tokens
    expect_eq(p_sliding.preserved_prefix_tokens, @as(u32, 200));
    expect_true(p_prefix.prefix_cache_ratio_bps > p_sliding.prefix_cache_ratio_bps);
    expect_eq(p_prefix.prefix_cache_ratio_bps, @as(u16, 4166)); // 500 * 10000 / 1200
    expect_eq(p_sliding.prefix_cache_ratio_bps, @as(u16, 1666)); // 200 * 10000 / 1200
}

test("ContextWindow: drop_tool_outputs_first evicts older tool groups before regular dialogue") {
    //  0: system (100)
    //  1: user (200)        <- older regular dialogue turn
    //  2: assistant (200)   <- older regular dialogue turn
    //  3: tool_call g=7 (100)
    //  4: tool_result g=7 (300) <- newer heavy tool pair (400 tokens)
    //  5: assistant (100)   <- tail
    // Total = 1000. Effective budget = 650.
    // drop_tool_outputs_first must evict tool group 7 [3,4] FIRST, preserving dialogue [1,2]!
    const turns: [6]CtxTurnMeta = [
        CtxTurnMeta{ turn_id: 1, role: CtxTurnRole.system, group_id: 0, tokens: 100 },
        CtxTurnMeta{ turn_id: 2, role: CtxTurnRole.user, group_id: 0, tokens: 200 },
        CtxTurnMeta{ turn_id: 3, role: CtxTurnRole.assistant, group_id: 0, tokens: 200 },
        CtxTurnMeta{ turn_id: 4, role: CtxTurnRole.tool_call, group_id: 7, tokens: 100 },
        CtxTurnMeta{ turn_id: 5, role: CtxTurnRole.tool_result, group_id: 7, tokens: 300 },
        CtxTurnMeta{ turn_id: 6, role: CtxTurnRole.assistant, group_id: 0, tokens: 100 },
    ];

    let cfg = CtxWindowConfig.defaults();
    cfg.max_input_tokens = 850;
    cfg.reserve_output_tokens = 200; // budget = 650
    cfg.prefix_anchor_turns = 1;
    cfg.min_recent_turns = 1;
    cfg.strategy = CtxEvictStrategy.drop_tool_outputs_first;

    const plan = planContextCompaction(turns[0..], cfg);
    expect_eq(plan.kept_count, @as(usize, 4));
    expect_eq(plan.evicted_count, @as(usize, 2));
    expect_eq(plan.kept_tokens, @as(u32, 600));
    expect_true(validateToolPairIntegrity(turns[0..], plan.kept_indices[0..], plan.kept_count));

    // Kept indices: [0, 1, 2, 5] -> regular dialogue [1, 2] survived, tool group [3, 4] evicted!
    expect_eq(plan.kept_indices[0], @as(usize, 0));
    expect_eq(plan.kept_indices[1], @as(usize, 1));
    expect_eq(plan.kept_indices[2], @as(usize, 2));
    expect_eq(plan.kept_indices[3], @as(usize, 5));
}

test("ContextWindow: system and pinned turns (including pinned tool pairs) are strictly protected") {
    const turns: [5]CtxTurnMeta = [
        CtxTurnMeta{ turn_id: 1, role: CtxTurnRole.system, group_id: 0, tokens: 300, pinned: false },
        CtxTurnMeta{ turn_id: 2, role: CtxTurnRole.tool_call, group_id: 99, tokens: 100, pinned: false },
        CtxTurnMeta{ turn_id: 3, role: CtxTurnRole.tool_result, group_id: 99, tokens: 200, pinned: true }, // pins whole group 99!
        CtxTurnMeta{ turn_id: 4, role: CtxTurnRole.user, group_id: 0, tokens: 250, pinned: false },
        CtxTurnMeta{ turn_id: 5, role: CtxTurnRole.assistant, group_id: 0, tokens: 250, pinned: false },
    ];

    let cfg = CtxWindowConfig.defaults();
    cfg.max_input_tokens = 500;
    cfg.reserve_output_tokens = 100; // budget = 400 (smaller than protected 300 + 300 = 600)
    cfg.strategy = CtxEvictStrategy.drop_tool_outputs_first;

    const plan = planContextCompaction(turns[0..], cfg);
    // Non-protected [3, 4] evicted; protected system [0] and pinned tool group [1, 2] survive intact!
    expect_eq(plan.kept_count, @as(usize, 3));
    expect_eq(plan.evicted_count, @as(usize, 2));
    expect_eq(plan.kept_tokens, @as(u32, 600));
    expect_true(validateToolPairIntegrity(turns[0..], plan.kept_indices[0..], plan.kept_count));
    expect_eq(plan.kept_indices[0], @as(usize, 0));
    expect_eq(plan.kept_indices[1], @as(usize, 1));
    expect_eq(plan.kept_indices[2], @as(usize, 2));
}

test("ContextWindowTracker: stateful multi-turn compaction, summary injection, capacity & facade parity") {
    let cfg = CtxWindowConfig.defaults();
    cfg.max_input_tokens = 1100;
    cfg.reserve_output_tokens = 200; // effective budget = 900
    cfg.prefix_anchor_turns = 2;
    cfg.min_recent_turns = 2;
    cfg.strategy = CtxEvictStrategy.preserve_prefix_and_tail;

    let tracker = ContextWindowTracker.init(cfg);
    expect_eq(tracker.len(), @as(usize, 0));
    expect_eq(tracker.utilizationBps(), @as(u16, 0));
    expect_true(!tracker.shouldCompact(8000));

    * = tracker.pushTurn(CtxTurnRole.system, 0, 200, false, 10, 0x1001);
    * = tracker.pushTurn(CtxTurnRole.user, 0, 150, false, 9, 0x1002);
    * = tracker.pushTurn(CtxTurnRole.tool_call, 5, 100, false, 5, 0x1003);
    * = tracker.pushTurn(CtxTurnRole.tool_result, 5, 300, false, 5, 0x1004);
    * = tracker.pushTurn(CtxTurnRole.assistant, 0, 250, false, 7, 0x1005);
    * = tracker.pushTurn(CtxTurnRole.user, 0, 150, false, 7, 0x1006);

    // Total = 1150 > 900 budget -> utilization clamped to 10000 bps and shouldCompact triggers
    expect_eq(tracker.totalTokens(), @as(u32, 1150));
    expect_eq(tracker.utilizationBps(), @as(u16, 10000));
    expect_true(tracker.shouldCompact(8000));

    // Preview plan matches applied compaction
    const preview = tracker.planCompaction();
    expect_eq(preview.evicted_count, @as(usize, 2)); // tool group 5 [2, 3]

    const applied = tracker.applyCompaction(80, 0x50AA);
    expect_eq(applied.evicted_count, @as(usize, 2));
    expect_eq(tracker.compaction_count, @as(u64, 1));
    expect_eq(tracker.total_evicted_turns, @as(u64, 2));
    // 6 - 2 evicted + 1 summary = 5 turns: [system, user, summary, assistant, user]
    expect_eq(tracker.len(), @as(usize, 5));
    expect_eq(tracker.totalTokens(), @as(u32, 830)); // 1150 - 400 + 80

    // Summary turn injected at exact eviction boundary index 2 (preserving prefix [0, 1]!)
    if (t2 := tracker.getTurn(2)) {
        expect_eq(t2.role, CtxTurnRole.summary);
        expect_eq(t2.tokens, @as(u32, 80));
        expect_eq(t2.content_hash, @as(u64, 0x50AA));
    } else {
        expect_true(false);
    }

    // Continue pushing more turns and trigger a second compaction
    * = tracker.pushTurn(CtxTurnRole.assistant, 0, 200, false, 6, 0x1007);
    expect_eq(tracker.totalTokens(), @as(u32, 1030));
    const applied2 = tracker.applyCompaction(60, 0x50BB);
    expect_true(applied2.evicted_count >= 1);
    expect_eq(tracker.compaction_count, @as(u64, 2));

    // Capacity boundary (64 turns) & cap.prompt facade parity
    let full_tr = cap.prompt.ContextWindowTracker.init(cap.prompt.CtxWindowConfig.defaults());
    let i: usize = 0;
    while (i < CTX_WIN_MAX_TURNS) : (i += 1) {
        const tid = full_tr.pushTurn(cap.prompt.CtxTurnRole.user, 0, 10, false, 1, @as(u64, i));
        expect_true(tid != null);
    }
    expect_true(full_tr.pushTurn(cap.prompt.CtxTurnRole.user, 0, 10, false, 1, 999) == null);
    expect_eq(full_tr.len(), CTX_WIN_MAX_TURNS);
}
