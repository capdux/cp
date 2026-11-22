// Origin: Pure-CP Stdlib Test Suite (lib/prompt/toolRouter.cp) | Created: 2026-11-23
// Deep production regression tests for cap:prompt dynamic tool router (`lib/prompt/toolRouter.cp`):
//   1. Fixed 64-slot catalog CRUD, case-insensitive keyword dedup/cap (8), slot reuse & setPinned
//   2. Pinned core tools locked first + deterministic ordering + budget overflow guard
//   3. Multi-signal scoring (domain @popCount + case-insensitive keyword hit rate + name bonus + priority)
//   4. Greedy budget packing skipping oversized tools + exact consumed/saved tokens & permille ratio
//   5. Hard risk-tier ceiling enforcement (including pinned) + min_score_threshold filtering
//   6. Boundary guards (empty catalog, zero budget, zero max_tools, small output slice, zero-cost tool)
//   7. Cumulative zero-heap telemetry snapshot, clear() vs resetAll(), and `cap.prompt` namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    TOOL_ROUTER_MAX_TOOLS,
    TOOL_ROUTER_MAX_KEYWORDS,
    ToolRiskTier,
    toolRiskRank,
    ToolRouterWeights,
    RoutedToolPick,
    ToolRouter,
} from "cap:prompt";

const DOM_FS: u64 = 1 << 0;
const DOM_DB: u64 = 1 << 1;
const DOM_NET: u64 = 1 << 2;
const DOM_GIT: u64 = 1 << 3;
const DOM_SHELL: u64 = 1 << 4;
const DOM_MATH: u64 = 1 << 5;

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("toolRouter: catalog CRUD, keyword dedup/cap, and 64-slot capacity guard") {
    let router = ToolRouter.init();
    expect_eq(router.count(), @as(usize, 0));
    expect_eq(router.totalCatalogTokens(), @as(u64, 0));

    // Empty name rejected
    expect_true(router.registerTool(1, "", DOM_FS, ToolRiskTier.safe, 100, 10, false) == null);

    // Register tool 101
    let s1: usize = 999;
    if (slot := router.registerTool(101, "read_file", DOM_FS, ToolRiskTier.safe, 120, 50, true)) {
        s1 = slot;
    }
    expect_true(s1 < TOOL_ROUTER_MAX_TOOLS);
    expect_eq(router.count(), @as(usize, 1));

    // Duplicate tool_id rejected
    expect_true(router.registerTool(101, "read_file_dup", DOM_FS, ToolRiskTier.safe, 120, 50, false) == null);

    // Keyword dedup (case-insensitive) & max 8 keywords enforcement
    expect_true(router.addKeyword(101, "file"));
    expect_eq(router.addKeyword(101, "FILE"), false); // duplicate ignored
    expect_eq(router.addKeyword(101, ""), false);     // empty rejected
    expect_true(router.addKeyword(101, "read"));
    expect_true(router.addKeyword(101, "open"));
    expect_true(router.addKeyword(101, "cat"));
    expect_true(router.addKeyword(101, "path"));
    expect_true(router.addKeyword(101, "disk"));
    expect_true(router.addKeyword(101, "source"));
    expect_true(router.addKeyword(101, "inspect"));
    // 9th keyword rejected
    expect_eq(router.addKeyword(101, "extra"), false);

    if (entry := router.getEntry(s1)) {
        expect_true(bytesEq(entry.nameSlice(), "read_file"));
        expect_eq(entry.keywordCount(), TOOL_ROUTER_MAX_KEYWORDS);
        expect_true(bytesEq(entry.keywordSlice(0), "file"));
        expect_true(bytesEq(entry.keywordSlice(7), "inspect"));
        expect_true(bytesEq(entry.keywordSlice(99), ""));
    } else {
        expect_true(false);
    }

    // Fill remaining 63 slots up to TOOL_ROUTER_MAX_TOOLS (64)
    let id: u32 = 200;
    while (id < 263) {
        expect_true(router.registerTool(id, "worker_tool", DOM_MATH, ToolRiskTier.low, 50, 1, false) != null);
        id += 1;
    }
    expect_eq(router.count(), TOOL_ROUTER_MAX_TOOLS);
    // 65th tool rejected
    expect_true(router.registerTool(999, "overflow_tool", DOM_MATH, ToolRiskTier.low, 50, 1, false) == null);

    // Remove and re-register reuses freed slot
    expect_true(router.removeTool(205));
    expect_eq(router.removeTool(205), false);
    expect_eq(router.count(), TOOL_ROUTER_MAX_TOOLS - 1);
    expect_true(router.registerTool(999, "reused_slot", DOM_NET, ToolRiskTier.medium, 80, 5, false) != null);
    expect_eq(router.count(), TOOL_ROUTER_MAX_TOOLS);

    // Toggle pinned
    expect_true(router.setPinned(999, true));
    if (reused_slot := router.findById(999)) {
        if (reused_entry := router.getEntry(reused_slot)) {
            expect_true(reused_entry.pinned);
        } else {
            expect_true(false);
        }
    } else {
        expect_true(false);
    }
}

test("toolRouter: pinned tools locked first and budget overflow handled without exceeding cap") {
    let router = ToolRouter.init();

    // Register 3 pinned tools with different priorities and token costs, plus 1 normal tool
    * = router.registerTool(1, "pin_high", DOM_FS, ToolRiskTier.safe, 150, 100, true);
    * = router.registerTool(2, "pin_mid_heavy", DOM_FS, ToolRiskTier.safe, 300, 60, true);
    * = router.registerTool(3, "pin_low_light", DOM_FS, ToolRiskTier.safe, 100, 40, true);
    * = router.registerTool(4, "normal_best", DOM_FS | DOM_DB, ToolRiskTier.safe, 80, 99, false);

    let picks: [4]RoutedToolPick = [RoutedToolPick{}; 4];
    let picks_s: []RoutedToolPick = picks[0..];

    // Budget 350:
    //   Phase 1 (pinned order: #1 prio=100 cost=150 -> fits, rem=200;
    //                         #2 prio=60  cost=300 -> overflow! skipped_by_budget=1;
    //                         #3 prio=40  cost=100 -> fits, rem=100)
    //   Phase 2 (normal: #4 cost=80 -> fits, rem=20)
    const rep = router.routeTools("query", DOM_FS | DOM_DB, ToolRiskTier.safe, 350, 4, *picks_s);

    expect_true(rep.pinned_budget_overflow);
    expect_eq(rep.selected_count, @as(usize, 3));
    expect_eq(rep.pinned_selected_count, @as(usize, 2));
    expect_eq(rep.consumed_tokens, @as(u64, 330));
    expect_true(rep.consumed_tokens <= 350);
    expect_eq(rep.skipped_by_budget, @as(usize, 1));

    expect_eq(picks[0].tool_id, @as(u32, 1));
    expect_true(picks[0].pinned);
    expect_eq(picks[1].tool_id, @as(u32, 3));
    expect_true(picks[1].pinned);
    expect_eq(picks[2].tool_id, @as(u32, 4));
    expect_eq(picks[2].pinned, false);
}

test("toolRouter: multi-signal scoring (domain popCount + case-insensitive keywords + name match + priority)") {
    let router = ToolRouter.init();

    const kw_sql: [3][]const u8 = ["sql", "postgres", "query"];
    const kw_git: [2][]const u8 = ["commit", "diff"];
    const kw_fs: [2][]const u8 = ["file", "read"];
    const kw_hybrid: [2][]const u8 = ["query", "diff"];

    * = router.registerToolWithKeywords(10, "sql_runner", DOM_DB, ToolRiskTier.medium, 100, 10, false, kw_sql[0..]);
    * = router.registerToolWithKeywords(20, "git_inspect", DOM_GIT, ToolRiskTier.safe, 100, 10, false, kw_git[0..]);
    * = router.registerToolWithKeywords(30, "read_file", DOM_FS, ToolRiskTier.safe, 100, 10, false, kw_fs[0..]);
    * = router.registerToolWithKeywords(40, "hybrid_analyzer", DOM_DB | DOM_GIT | DOM_FS, ToolRiskTier.low, 100, 5, false, kw_hybrid[0..]);

    let picks: [4]RoutedToolPick = [RoutedToolPick{}; 4];
    let picks_s: []RoutedToolPick = picks[0..];

    // Query mentions "SQL_RUNNER" (name match) + "POSTGRES" + "QUERY" + "DIFF"
    // Required domains: DOM_DB | DOM_GIT | DOM_FS (3 bits)
    // - hybrid_analyzer: 3 domain bits (1200) + 2/2 kw (300 + 120) + prio 50 = 1670
    // - sql_runner: 1 domain bit (400) + 2/3 kw (200 + 120) + name_bonus (500) + prio 100 = 1320
    // - git_inspect: 1 domain bit (400) + 1/2 kw (150 + 60) + prio 100 = 710
    // - read_file: 1 domain bit (400) + 0 kw + prio 100 = 500
    const rep = router.routeTools(
        "Use SQL_RUNNER on POSTGRES to QUERY and DIFF schema",
        DOM_DB | DOM_GIT | DOM_FS,
        ToolRiskTier.medium,
        1000,
        4,
        *picks_s,
    );

    expect_eq(rep.selected_count, @as(usize, 4));
    expect_eq(picks[0].tool_id, @as(u32, 40));
    expect_eq(picks[0].domain_overlap, @as(u32, 3));
    expect_eq(picks[0].keyword_hits, @as(u32, 2));

    expect_eq(picks[1].tool_id, @as(u32, 10));
    expect_eq(picks[1].domain_overlap, @as(u32, 1));
    expect_eq(picks[1].keyword_hits, @as(u32, 2));

    expect_eq(picks[2].tool_id, @as(u32, 20));
    expect_eq(picks[3].tool_id, @as(u32, 30));
    expect_true(picks[0].score > picks[1].score);
    expect_true(picks[1].score > picks[2].score);
    expect_true(picks[2].score > picks[3].score);
}

test("toolRouter: greedy budget packing skips oversized high-score tool and reports exact savings") {
    let router = ToolRouter.init();

    const kw_a: [1][]const u8 = ["deploy"];
    const kw_b: [1][]const u8 = ["deploy"];
    const kw_c: [1][]const u8 = ["status"];

    // Total catalog tokens = 100 + 450 + 180 + 270 = 1000
    * = router.registerTool(1, "core_ping", DOM_NET, ToolRiskTier.safe, 100, 10, true);
    * = router.registerToolWithKeywords(2, "heavy_deploy", DOM_NET | DOM_SHELL, ToolRiskTier.medium, 450, 50, false, kw_a[0..]);
    * = router.registerToolWithKeywords(3, "lite_deploy", DOM_NET, ToolRiskTier.medium, 180, 30, false, kw_b[0..]);
    * = router.registerToolWithKeywords(4, "check_status", DOM_NET, ToolRiskTier.safe, 270, 20, false, kw_c[0..]);

    let picks: [4]RoutedToolPick = [RoutedToolPick{}; 4];
    let picks_s: []RoutedToolPick = picks[0..];

    // Budget = 300:
    //   core_ping (pinned, 100) -> rem 200
    //   heavy_deploy (score highest, 450) -> skipped_by_budget
    //   lite_deploy (180) -> fits! rem 20
    //   check_status (270) -> skipped_by_budget
    // Consumed = 280 / 1000 => saved = 720 => savings_permille = 720
    const rep = router.routeTools("deploy service status", DOM_NET | DOM_SHELL, ToolRiskTier.medium, 300, 4, *picks_s);

    expect_eq(rep.catalog_total_tools, @as(usize, 4));
    expect_eq(rep.catalog_total_tokens, @as(u64, 1000));
    expect_eq(rep.selected_count, @as(usize, 2));
    expect_eq(rep.pinned_selected_count, @as(usize, 1));
    expect_eq(rep.consumed_tokens, @as(u64, 280));
    expect_eq(rep.saved_tokens, @as(u64, 720));
    expect_eq(rep.savings_permille, @as(u32, 720));
    expect_eq(rep.skipped_by_budget, @as(usize, 2));
    expect_eq(rep.skipped_by_risk, @as(usize, 0));
    expect_eq(picks[0].tool_id, @as(u32, 1));
    expect_eq(picks[1].tool_id, @as(u32, 3));
}

test("toolRouter: risk ceiling enforcement (including pinned) and min_score_threshold filtering") {
    let weights = ToolRouterWeights.defaults();
    weights.min_score_threshold = 400;
    let router = ToolRouter.initWithWeights(weights);

    expect_eq(toolRiskRank(ToolRiskTier.safe), @as(u8, 0));
    expect_eq(toolRiskRank(ToolRiskTier.critical), @as(u8, 4));

    const kw_db: [1][]const u8 = ["database"];
    const kw_irrel: [1][]const u8 = ["weather"];

    // Critical pinned tool must still be blocked when max_risk_tier = medium
    * = router.registerTool(1, "drop_prod_db", DOM_DB, ToolRiskTier.critical, 50, 100, true);
    // High risk normal tool blocked when max_risk_tier = medium
    * = router.registerTool(2, "exec_shell", DOM_SHELL, ToolRiskTier.high, 100, 80, false);
    // Medium risk relevant tool passes (>= 400 from domain_overlap=1)
    * = router.registerToolWithKeywords(3, "query_db", DOM_DB, ToolRiskTier.medium, 150, 10, false, kw_db[0..]);
    // Safe irrelevant tool (score = 0 + 0 + 0 + 50 = 50 < 400) filtered by threshold
    * = router.registerToolWithKeywords(4, "get_weather", DOM_NET, ToolRiskTier.safe, 100, 5, false, kw_irrel[0..]);

    let picks: [4]RoutedToolPick = [RoutedToolPick{}; 4];
    let picks_s: []RoutedToolPick = picks[0..];

    const rep = router.routeTools("inspect database tables", DOM_DB, ToolRiskTier.medium, 1000, 4, *picks_s);

    expect_eq(rep.selected_count, @as(usize, 1));
    expect_eq(rep.pinned_selected_count, @as(usize, 0));
    expect_eq(rep.skipped_by_risk, @as(usize, 2));
    expect_eq(rep.skipped_by_threshold, @as(usize, 1));
    expect_eq(picks[0].tool_id, @as(u32, 3));
}

test("toolRouter: boundary guards (empty catalog, zero budget, zero max_tools, small output slice)") {
    let router = ToolRouter.init();
    let picks: [4]RoutedToolPick = [RoutedToolPick{}; 4];
    let picks_s: []RoutedToolPick = picks[0..];

    // 1. Empty catalog
    const rep_empty = router.routeTools("anything", DOM_FS, ToolRiskTier.critical, 1000, 4, *picks_s);
    expect_eq(rep_empty.catalog_total_tools, @as(usize, 0));
    expect_eq(rep_empty.selected_count, @as(usize, 0));
    expect_eq(rep_empty.savings_permille, @as(u32, 0));

    // Register 3 tools (including 1 zero-token metadata tool)
    * = router.registerTool(1, "zero_cost", DOM_FS, ToolRiskTier.safe, 0, 10, false);
    * = router.registerTool(2, "tool_a", DOM_FS, ToolRiskTier.safe, 200, 20, false);
    * = router.registerTool(3, "tool_b", DOM_FS, ToolRiskTier.safe, 200, 30, false);

    // 2. Zero max_tools returns 0 picks and counts skipped_by_limit
    const rep_zero_max = router.routeTools("", DOM_FS, ToolRiskTier.safe, 1000, 0, *picks_s);
    expect_eq(rep_zero_max.selected_count, @as(usize, 0));
    expect_eq(rep_zero_max.skipped_by_limit, @as(usize, 3));
    expect_eq(rep_zero_max.savings_permille, @as(u32, 1000));

    // 3. Zero budget allows 0-token tool but blocks >0-token tools
    const rep_zero_budget = router.routeTools("", DOM_FS, ToolRiskTier.safe, 0, 4, *picks_s);
    expect_eq(rep_zero_budget.selected_count, @as(usize, 1));
    expect_eq(picks[0].tool_id, @as(u32, 1));
    expect_eq(rep_zero_budget.skipped_by_budget, @as(usize, 2));

    // 4. Small output slice caps selected_count safely
    let tiny_picks: [1]RoutedToolPick = [RoutedToolPick{}; 1];
    let tiny_s: []RoutedToolPick = tiny_picks[0..];
    const rep_tiny = router.routeTools("", DOM_FS, ToolRiskTier.safe, 1000, 10, *tiny_s);
    expect_eq(rep_tiny.selected_count, @as(usize, 1));
    expect_eq(rep_tiny.skipped_by_limit, @as(usize, 2));
    expect_eq(tiny_picks[0].tool_id, @as(u32, 3)); // highest priority
}

test("toolRouter: cumulative zero-heap telemetry, clear vs resetAll, and cap.prompt facade parity") {
    let router = cap.prompt.ToolRouter.init();
    * = router.registerTool(1, "safe_a", DOM_FS, cap.prompt.ToolRiskTier.safe, 100, 10, false);
    * = router.registerTool(2, "safe_b", DOM_FS, cap.prompt.ToolRiskTier.safe, 300, 5, false);
    * = router.registerTool(3, "risky_c", DOM_SHELL, cap.prompt.ToolRiskTier.high, 200, 90, false);

    let picks: [4]cap.prompt.RoutedToolPick = [cap.prompt.RoutedToolPick{}; 4];
    let picks_s: []cap.prompt.RoutedToolPick = picks[0..];

    // Round 1: budget 150, max_risk = low -> picks #1 (100), #2 budget blocked, #3 risk blocked, saved 500
    * = router.routeTools("fs", DOM_FS, cap.prompt.ToolRiskTier.low, 150, 4, *picks_s);
    // Round 2: budget 1000, max_risk = high -> picks all 3 (600), saved 0
    * = router.routeTools("fs", DOM_FS | DOM_SHELL, cap.prompt.ToolRiskTier.high, 1000, 4, *picks_s);

    const st = router.stats();
    expect_eq(st.total_routes, @as(u64, 2));
    expect_eq(st.total_registered, @as(u64, 3));
    expect_eq(st.total_picks_emitted, @as(u64, 4));
    expect_eq(st.total_tokens_consumed, @as(u64, 700));
    expect_eq(st.total_tokens_saved, @as(u64, 500));
    expect_eq(st.total_risk_blocked, @as(u64, 1));
    expect_eq(st.total_budget_blocked, @as(u64, 1));

    // clear() empties catalog but preserves cumulative telemetry
    router.clear();
    expect_eq(router.count(), @as(usize, 0));
    expect_eq(router.stats().total_routes, @as(u64, 2));

    // resetAll() zeros telemetry as well
    router.resetAll();
    expect_eq(router.stats().total_routes, @as(u64, 0));
    expect_eq(router.stats().total_registered, @as(u64, 0));
}
