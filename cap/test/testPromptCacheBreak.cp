// Origin: Pure-CP Stdlib Test Suite (lib/prompt/cacheBreak.cp) | Created: 2026-11-23
// Production Deep Verification Suite for LLM Prompt Prefix Cache Breakpoint Planner & Bust Diagnostics:
//   1. Automatic breakpoint planning: <=4 max_breakpoints, min_cache_tokens, role boundaries & volatile stop
//   2. Ordering anti-pattern detection (volatile before static, static role after history)
//   3. Cross-turn prefix cache hit, incremental cache write & Anthropic positive savings_bps calculation
//   4. Cache bust root-cause diagnostics: content_changed, role_changed, volatile_inserted, prefix_shortened, below_min_tokens
//   5. Anthropic vs OpenAI pricing comparison (cold write surcharge vs warm read discount) & empty/small input guards
//   6. Stateful multi-turn `PromptCacheTracker` global stats, reset & `cap.prompt` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CACHE_BREAK_MAX_BLOCKS,
    CACHE_BREAK_MAX_BREAKPOINTS,
    CACHE_BREAK_BPS_SCALE,
    CacheBlockRole,
    CacheBustReason,
    PromptCacheBlock,
    CacheBreakConfig,
    CacheBreakpointPlan,
    CacheTurnDiff,
    PromptCacheStats,
    hashCacheBlockBytes,
    cacheRoleRank,
    planCacheBreakpoints,
    diffWithPrevTurn,
    PromptCacheTracker,
} from "cap:prompt";

test("CacheBreak: automatic breakpoint planning, max_breakpoints=4 cap & min_cache_tokens") {
    const cfg = CacheBreakConfig.anthropicDefaults();

    // 5 distinct role segments before user_query (exceeds 4 breakpoints -> capped cleanly to 4)
    const blocks: [6]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x10, 1200, false),
        PromptCacheBlock.make(2, CacheBlockRole.tools, 0x20, 800, false),
        PromptCacheBlock.make(3, CacheBlockRole.docs, 0x30, 1500, false),
        PromptCacheBlock.make(4, CacheBlockRole.few_shot, 0x40, 600, false),
        PromptCacheBlock.make(5, CacheBlockRole.history, 0x50, 900, false),
        PromptCacheBlock.make(6, CacheBlockRole.user_query, 0x60, 200, true),
    ];

    let plan = CacheBreakpointPlan{};
    planCacheBreakpoints(blocks[0..], cfg, *plan);

    expect_eq(plan.total_blocks, @as(usize, 6));
    expect_eq(plan.total_tokens, @as(u64, 5200));
    expect_eq(plan.stable_prefix_blocks, @as(usize, 5));
    expect_eq(plan.stable_prefix_tokens, @as(u64, 5000));
    expect_eq(plan.breakpoint_count, CACHE_BREAK_MAX_BREAKPOINTS);
    // Final breakpoint always covers full stable prefix (index 4 -> 5000 tokens)
    expect_eq(plan.breakpoint_indices[3], @as(usize, 4));
    expect_eq(plan.cacheable_tokens, @as(u64, 5000));
    expect_eq(plan.uncached_tokens, @as(u64, 200));
    expect_true(!plan.has_order_anti_pattern);
    expect_true(plan.has_volatile);
    expect_eq(plan.first_volatile_index, @as(usize, 5));

    // Below min_cache_tokens -> 0 breakpoints placed
    const tiny: [2]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x1, 500, false),
        PromptCacheBlock.make(2, CacheBlockRole.user_query, 0x2, 100, true),
    ];
    let tiny_plan = CacheBreakpointPlan{};
    planCacheBreakpoints(tiny[0..], cfg, *tiny_plan);
    expect_eq(tiny_plan.breakpoint_count, @as(usize, 0));
    expect_eq(tiny_plan.cacheable_tokens, @as(u64, 0));
    expect_eq(tiny_plan.uncached_tokens, @as(u64, 600));
}

test("CacheBreak: ordering anti-pattern detection (volatile before static & inverted roles)") {
    const cfg = CacheBreakConfig.anthropicDefaults();

    // Anti-pattern 1: volatile timestamp block placed before huge static docs
    const bad1: [3]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x1, 200, true), // volatile!
        PromptCacheBlock.make(2, CacheBlockRole.docs, 0x2, 3000, false),
        PromptCacheBlock.make(3, CacheBlockRole.user_query, 0x3, 100, true),
    ];
    let p1 = CacheBreakpointPlan{};
    planCacheBreakpoints(bad1[0..], cfg, *p1);
    expect_true(p1.has_order_anti_pattern);
    expect_eq(p1.anti_pattern_count, @as(u32, 1));
    expect_eq(p1.stable_prefix_blocks, @as(usize, 0));
    expect_eq(p1.cacheable_tokens, @as(u64, 0));

    // Anti-pattern 2: static `tools` placed after `history`
    const bad2: [3]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x1, 1200, false),
        PromptCacheBlock.make(2, CacheBlockRole.history, 0x2, 600, false),
        PromptCacheBlock.make(3, CacheBlockRole.tools, 0x3, 800, false),
    ];
    let p2 = CacheBreakpointPlan{};
    planCacheBreakpoints(bad2[0..], cfg, *p2);
    expect_true(p2.has_order_anti_pattern);
    expect_eq(p2.anti_pattern_count, @as(u32, 1));
    expect_true(cacheRoleRank(CacheBlockRole.system) < cacheRoleRank(CacheBlockRole.user_query));
}

test("CacheBreak: cross-turn cache hit, incremental cache write & Anthropic savings_bps") {
    const cfg = CacheBreakConfig.anthropicDefaults();
    const h_sys = hashCacheBlockBytes("system instructions");
    const h_tools = hashCacheBlockBytes("tool definitions");
    const h_hist1 = hashCacheBlockBytes("turn 1 history");

    const turn1: [3]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, h_sys, 1200, false),
        PromptCacheBlock.make(2, CacheBlockRole.tools, h_tools, 800, false),
        PromptCacheBlock.make(3, CacheBlockRole.user_query, 0xA1, 100, true),
    ];

    // Turn 2 preserves system + tools (2000 tokens), appends history (500) + new user_query (100)
    const turn2: [4]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, h_sys, 1200, false),
        PromptCacheBlock.make(2, CacheBlockRole.tools, h_tools, 800, false),
        PromptCacheBlock.make(3, CacheBlockRole.history, h_hist1, 500, false),
        PromptCacheBlock.make(4, CacheBlockRole.user_query, 0xA2, 100, true),
    ];

    const diff: CacheTurnDiff = diffWithPrevTurn(turn1[0..], turn2[0..], cfg);
    expect_true(!diff.has_bust);
    expect_eq(diff.bust_reason, CacheBustReason.none);
    expect_eq(diff.common_prefix_blocks, @as(usize, 2));
    expect_eq(diff.cached_read_tokens, @as(u64, 2000));
    expect_eq(diff.cache_write_tokens, @as(u64, 500));
    expect_eq(diff.uncached_tokens, @as(u64, 100));
    // Baseline: 2600 * 10000 = 26,000,000
    // Effective: 2000*1000 + 500*12500 + 100*10000 = 9,250,000 -> savings ~= 6442 bps (64.42%)
    expect_true(diff.savings_bps >= 6400 and diff.savings_bps <= 6500);
}

test("CacheBreak: cache bust root-cause localization (content, role, volatile, shortened)") {
    const cfg = CacheBreakConfig.anthropicDefaults();

    const prev: [3]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x11, 1200, false),
        PromptCacheBlock.make(2, CacheBlockRole.tools, 0x22, 800, false),
        PromptCacheBlock.make(3, CacheBlockRole.docs, 0x33, 1000, false),
    ];

    // 1. Content changed at index 1 (tools) -> still hits breakpoint 0 (system=1200), rewrites 1800
    const curr_content: [3]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x11, 1200, false),
        PromptCacheBlock.make(2, CacheBlockRole.tools, 0x99, 800, false),
        PromptCacheBlock.make(3, CacheBlockRole.docs, 0x33, 1000, false),
    ];
    const d1 = diffWithPrevTurn(prev[0..], curr_content[0..], cfg);
    expect_true(d1.has_bust);
    expect_eq(d1.first_bust_index, @as(usize, 1));
    expect_eq(d1.bust_reason, CacheBustReason.content_changed);
    expect_eq(d1.cached_read_tokens, @as(u64, 1200));
    expect_eq(d1.cache_write_tokens, @as(u64, 1800));

    // 2. Role changed at index 1
    const curr_role: [3]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x11, 1200, false),
        PromptCacheBlock.make(2, CacheBlockRole.docs, 0x22, 800, false),
        PromptCacheBlock.make(3, CacheBlockRole.docs, 0x33, 1000, false),
    ];
    const d2 = diffWithPrevTurn(prev[0..], curr_role[0..], cfg);
    expect_true(d2.has_bust);
    expect_eq(d2.first_bust_index, @as(usize, 1));
    expect_eq(d2.bust_reason, CacheBustReason.role_changed);

    // 3. Volatile inserted at index 0 -> total bust (0 cached_read_tokens)
    const curr_vol: [3]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x11, 1200, true),
        PromptCacheBlock.make(2, CacheBlockRole.tools, 0x22, 800, false),
        PromptCacheBlock.make(3, CacheBlockRole.docs, 0x33, 1000, false),
    ];
    const d3 = diffWithPrevTurn(prev[0..], curr_vol[0..], cfg);
    expect_true(d3.has_bust);
    expect_eq(d3.first_bust_index, @as(usize, 0));
    expect_eq(d3.bust_reason, CacheBustReason.volatile_inserted);
    expect_eq(d3.cached_read_tokens, @as(u64, 0));

    // 4. Prefix shortened (only 1 block instead of 3)
    const curr_short: [1]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x11, 1200, false),
    ];
    const d4 = diffWithPrevTurn(prev[0..], curr_short[0..], cfg);
    expect_true(d4.has_bust);
    expect_eq(d4.first_bust_index, @as(usize, 1));
    expect_eq(d4.bust_reason, CacheBustReason.prefix_shortened);
    expect_eq(d4.cached_read_tokens, @as(u64, 1200));
}

test("CacheBreak: Anthropic vs OpenAI pricing model & cold-start write surcharge") {
    const empty: [0]PromptCacheBlock = [];
    const turn1: [2]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, 0x1, 2000, false),
        PromptCacheBlock.make(2, CacheBlockRole.user_query, 0x2, 0, true),
    ];

    // Anthropic cold turn 1: 125% cache write surcharge -> -2500 bps
    const anth = diffWithPrevTurn(empty[0..], turn1[0..], CacheBreakConfig.anthropicDefaults());
    expect_eq(anth.cached_read_tokens, @as(u64, 0));
    expect_eq(anth.cache_write_tokens, @as(u64, 2000));
    expect_eq(anth.savings_bps, @as(i32, -2500));

    // OpenAI cold turn 1: 100% write -> 0 bps surcharge; warm turn 2: 50% read -> +5000 bps
    const oai1 = diffWithPrevTurn(empty[0..], turn1[0..], CacheBreakConfig.openAiDefaults());
    expect_eq(oai1.savings_bps, @as(i32, 0));
    const oai2 = diffWithPrevTurn(turn1[0..], turn1[0..], CacheBreakConfig.openAiDefaults());
    expect_eq(oai2.savings_bps, @as(i32, 5000));
}

test("CacheBreak: stateful PromptCacheTracker stats, reset & cap.prompt facade parity") {
    let tracker = cap.prompt.PromptCacheTracker.init(cap.prompt.CacheBreakConfig.anthropicDefaults());
    const h_sys = cap.prompt.hashCacheBlockBytes("stable system prompt");

    const t1: [2]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, h_sys, 2000, false),
        PromptCacheBlock.make(2, CacheBlockRole.user_query, 0x1, 200, true),
    ];
    const t2: [2]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, h_sys, 2000, false),
        PromptCacheBlock.make(2, CacheBlockRole.user_query, 0x2, 200, true),
    ];
    const t3: [2]PromptCacheBlock = [
        PromptCacheBlock.make(1, CacheBlockRole.system, h_sys, 2000, false),
        PromptCacheBlock.make(2, CacheBlockRole.user_query, 0x3, 200, true),
    ];

    * = tracker.recordTurn(t1[0..]); // cold write
    * = tracker.recordTurn(t2[0..]); // cache hit
    * = tracker.recordTurn(t3[0..]); // cache hit

    const st: PromptCacheStats = tracker.stats();
    expect_eq(st.total_turns, @as(u64, 3));
    expect_eq(st.total_tokens, @as(u64, 6600));
    expect_eq(st.total_cached_read_tokens, @as(u64, 4000));
    expect_eq(st.total_cache_write_tokens, @as(u64, 2000));
    expect_eq(st.total_uncached_tokens, @as(u64, 600));
    expect_eq(st.cache_hit_turns, @as(u64, 2));
    expect_eq(st.cache_bust_turns, @as(u64, 0));
    expect_true(st.cumulative_savings_bps > 4500);

    tracker.reset();
    expect_eq(tracker.stats().total_turns, @as(u64, 0));
    expect_eq(CACHE_BREAK_MAX_BLOCKS, @as(usize, 32));
    expect_eq(CACHE_BREAK_BPS_SCALE, @as(u32, 10_000));
}
