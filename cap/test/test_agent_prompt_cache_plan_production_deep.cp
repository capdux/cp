// Origin: Pure-CP Stdlib Test Suite (AI Prompt-Cache Breakpoint Planner & Prefix Stability Optimizer) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/prompt_cache_plan.cp
// Covers:
//   1. Multi-segment stable prefix breakpoint placement & max_breakpoints cap enforcement
//   2. min_cacheable_tokens threshold filtering (below threshold -> 0 breakpoints, worth_caching == false)
//   3. Single-turn no-reuse economics guard (expected_future_turns == 0 -> negative net savings, worth_caching == false)
//   4. volatile_flag & CurrentUser prefix truncation preventing invalid caching past unstable blocks
//   5. Cross-turn computePrefixMatchTokens stopping at first divergence & invalid config guards

import { test, expect_eq } from "cap:test";
import {
    PromptBlockKind,
    PromptBlockDesc,
    CachePlanConfig,
    CachePlanResult,
    planPromptCacheBreakpoints,
    computePrefixMatchTokens,
} from "../../lib/agent/index.cp";

fn testMultiSegmentBreakpointsAndMaxCap()! {
    const blocks: [5]PromptBlockDesc = [
        PromptBlockDesc{ index: 0, kind: PromptBlockKind.SystemPrompt, tokens: 600, content_hash: 0x10, volatile_flag: false },
        PromptBlockDesc{ index: 1, kind: PromptBlockKind.ToolDefs, tokens: 700, content_hash: 0x20, volatile_flag: false },
        PromptBlockDesc{ index: 2, kind: PromptBlockKind.DocContext, tokens: 1200, content_hash: 0x30, volatile_flag: false },
        PromptBlockDesc{ index: 3, kind: PromptBlockKind.HistoryTurn, tokens: 1100, content_hash: 0x40, volatile_flag: false },
        PromptBlockDesc{ index: 4, kind: PromptBlockKind.CurrentUser, tokens: 150, content_hash: 0x50, volatile_flag: false },
    ];

    let bp_buf: [4]u16 = [0; 4];
    let bps: []u16 = bp_buf[0..];

    // 1. max_breakpoints = 4: semantic boundaries after crossing 1024 tokens -> indices [1, 2, 3]
    const cfg4 = CachePlanConfig{
        max_breakpoints: 4,
        min_cacheable_tokens: 1024,
        expected_future_turns: 3,
        write_overhead_pct: 25,
        read_discount_pct: 90,
    };
    const r4 = planPromptCacheBreakpoints(blocks[0..], cfg4, bps) ?? CachePlanResult{};
    expect_eq(r4.worth_caching, true);
    expect_eq(r4.cached_prefix_tokens, 3600);
    expect_eq(r4.breakpoint_count, 3);
    expect_eq(bps[0], 1);
    expect_eq(bps[1], 2);
    expect_eq(bps[2], 3);
    // Net savings: 3600 * 90% * 3 - 3600 * 25% = 9720 - 900 = 8820
    expect_eq(r4.estimated_net_savings_tokens, 8820);

    // 2. max_breakpoints = 2: enforces cap while still covering the full 3600-token stable prefix
    const cfg2 = CachePlanConfig{
        max_breakpoints: 2,
        min_cacheable_tokens: 1024,
        expected_future_turns: 3,
        write_overhead_pct: 25,
        read_discount_pct: 90,
    };
    const r2 = planPromptCacheBreakpoints(blocks[0..], cfg2, bps) ?? CachePlanResult{};
    expect_eq(r2.worth_caching, true);
    expect_eq(r2.cached_prefix_tokens, 3600);
    expect_eq(r2.breakpoint_count, 2);
    expect_eq(bps[0], 1);
    expect_eq(bps[1], 3);
}

fn testMinCacheableTokensThresholdFilter()! {
    const short_blocks: [3]PromptBlockDesc = [
        PromptBlockDesc{ index: 0, kind: PromptBlockKind.SystemPrompt, tokens: 400, content_hash: 0x1, volatile_flag: false },
        PromptBlockDesc{ index: 1, kind: PromptBlockKind.ToolDefs, tokens: 500, content_hash: 0x2, volatile_flag: false },
        PromptBlockDesc{ index: 2, kind: PromptBlockKind.CurrentUser, tokens: 300, content_hash: 0x3, volatile_flag: false },
    ];

    let bp_buf: [4]u16 = [0; 4];
    let bps: []u16 = bp_buf[0..];
    const cfg = CachePlanConfig{
        max_breakpoints: 4,
        min_cacheable_tokens: 1024,
        expected_future_turns: 5,
        write_overhead_pct: 25,
        read_discount_pct: 90,
    };

    // Stable prefix is only 900 tokens (< 1024) -> skip caching
    const res = planPromptCacheBreakpoints(short_blocks[0..], cfg, bps) ?? CachePlanResult{};
    expect_eq(res.worth_caching, false);
    expect_eq(res.breakpoint_count, 0);
    expect_eq(res.cached_prefix_tokens, 0);
    expect_eq(res.estimated_net_savings_tokens, 0);
}

fn testSingleTurnNoReuseEconomicsGuard()! {
    const blocks: [2]PromptBlockDesc = [
        PromptBlockDesc{ index: 0, kind: PromptBlockKind.SystemPrompt, tokens: 4000, content_hash: 0xA, volatile_flag: false },
        PromptBlockDesc{ index: 1, kind: PromptBlockKind.CurrentUser, tokens: 100, content_hash: 0xB, volatile_flag: false },
    ];

    let bp_buf: [4]u16 = [0; 4];
    let bps: []u16 = bp_buf[0..];

    // 0 future turns -> write overhead (25% of 4000 = 1000) causes net loss of -1000 tokens
    const one_shot_cfg = CachePlanConfig{
        max_breakpoints: 4,
        min_cacheable_tokens: 1024,
        expected_future_turns: 0,
        write_overhead_pct: 25,
        read_discount_pct: 90,
    };
    const r0 = planPromptCacheBreakpoints(blocks[0..], one_shot_cfg, bps) ?? CachePlanResult{};
    expect_eq(r0.worth_caching, false);
    expect_eq(r0.breakpoint_count, 0);
    expect_eq(r0.cached_prefix_tokens, 0);
    expect_eq(r0.estimated_net_savings_tokens, -1000);

    // 1 future turn with 90% read discount -> 3600 - 1000 = +2600 net savings -> worth caching
    const multi_cfg = CachePlanConfig{
        max_breakpoints: 4,
        min_cacheable_tokens: 1024,
        expected_future_turns: 1,
        write_overhead_pct: 25,
        read_discount_pct: 90,
    };
    const r1 = planPromptCacheBreakpoints(blocks[0..], multi_cfg, bps) ?? CachePlanResult{};
    expect_eq(r1.worth_caching, true);
    expect_eq(r1.breakpoint_count, 1);
    expect_eq(bps[0], 0);
    expect_eq(r1.estimated_net_savings_tokens, 2600);
}

fn testVolatileBlockTruncatesStablePrefix()! {
    // Block 1 has volatile_flag == true (e.g. dynamic timestamp/UUID) -> truncates prefix after block 0
    const blocks: [4]PromptBlockDesc = [
        PromptBlockDesc{ index: 0, kind: PromptBlockKind.SystemPrompt, tokens: 1500, content_hash: 0x11, volatile_flag: false },
        PromptBlockDesc{ index: 1, kind: PromptBlockKind.FewShot, tokens: 800, content_hash: 0x22, volatile_flag: true },
        PromptBlockDesc{ index: 2, kind: PromptBlockKind.DocContext, tokens: 5000, content_hash: 0x33, volatile_flag: false },
        PromptBlockDesc{ index: 3, kind: PromptBlockKind.CurrentUser, tokens: 100, content_hash: 0x44, volatile_flag: false },
    ];

    let bp_buf: [4]u16 = [0; 4];
    let bps: []u16 = bp_buf[0..];
    const cfg = CachePlanConfig{
        max_breakpoints: 4,
        min_cacheable_tokens: 1024,
        expected_future_turns: 2,
        write_overhead_pct: 25,
        read_discount_pct: 90,
    };

    const res = planPromptCacheBreakpoints(blocks[0..], cfg, bps) ?? CachePlanResult{};
    expect_eq(res.worth_caching, true);
    // Only block 0 (1500 tokens) is cached; block 2 (5000 tokens) after volatile block 1 is excluded
    expect_eq(res.cached_prefix_tokens, 1500);
    expect_eq(res.breakpoint_count, 1);
    expect_eq(bps[0], 0);
}

fn testCrossTurnPrefixMatchAndInvalidGuards()! {
    const turn1: [4]PromptBlockDesc = [
        PromptBlockDesc{ index: 0, kind: PromptBlockKind.SystemPrompt, tokens: 1000, content_hash: 0x111, volatile_flag: false },
        PromptBlockDesc{ index: 1, kind: PromptBlockKind.ToolDefs, tokens: 800, content_hash: 0x222, volatile_flag: false },
        PromptBlockDesc{ index: 2, kind: PromptBlockKind.HistoryTurn, tokens: 400, content_hash: 0x333, volatile_flag: false },
        PromptBlockDesc{ index: 3, kind: PromptBlockKind.CurrentUser, tokens: 120, content_hash: 0x444, volatile_flag: false },
    ];
    // Turn 2 mutates block 2 hash; block 3 happens to match hash but must NOT be counted past divergence
    const turn2: [4]PromptBlockDesc = [
        PromptBlockDesc{ index: 0, kind: PromptBlockKind.SystemPrompt, tokens: 1000, content_hash: 0x111, volatile_flag: false },
        PromptBlockDesc{ index: 1, kind: PromptBlockKind.ToolDefs, tokens: 800, content_hash: 0x222, volatile_flag: false },
        PromptBlockDesc{ index: 2, kind: PromptBlockKind.HistoryTurn, tokens: 400, content_hash: 0x999, volatile_flag: false },
        PromptBlockDesc{ index: 3, kind: PromptBlockKind.CurrentUser, tokens: 120, content_hash: 0x444, volatile_flag: false },
    ];

    expect_eq(computePrefixMatchTokens(turn1[0..], turn2[0..]), 1800);
    expect_eq(computePrefixMatchTokens(turn1[0..], turn1[0..]), 2320);

    const empty_blocks: [0]PromptBlockDesc = [];
    expect_eq(computePrefixMatchTokens(empty_blocks[0..], turn1[0..]), 0);

    // Invalid config guards
    let bp_buf: [2]u16 = [0; 2];
    let bps: []u16 = bp_buf[0..];
    expect_eq(planPromptCacheBreakpoints(turn1[0..], CachePlanConfig{ max_breakpoints: 0 }, bps) == null, true);
    expect_eq(planPromptCacheBreakpoints(turn1[0..], CachePlanConfig{ read_discount_pct: 101 }, bps) == null, true);

    const empty_res = planPromptCacheBreakpoints(empty_blocks[0..], CachePlanConfig{}, bps) ?? CachePlanResult{};
    expect_eq(empty_res.worth_caching, false);
    expect_eq(empty_res.breakpoint_count, 0);
}

test("agent prompt_cache_plan: multi-segment semantic breakpoints and max_breakpoints cap", testMultiSegmentBreakpointsAndMaxCap);
test("agent prompt_cache_plan: min_cacheable_tokens threshold filtering", testMinCacheableTokensThresholdFilter);
test("agent prompt_cache_plan: single-turn no-reuse economics guard prevents net loss", testSingleTurnNoReuseEconomicsGuard);
test("agent prompt_cache_plan: volatile_flag truncates stable prefix at first unstable block", testVolatileBlockTruncatesStablePrefix);
test("agent prompt_cache_plan: cross-turn prefix match stops at first divergence and invalid guards", testCrossTurnPrefixMatchAndInvalidGuards);
