// Origin: Pure-CP Stdlib Test Suite (lib/agent/tool_router.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc SIMD Semantic + Tag Hybrid Tool Router:
//   1. 8-lane SIMD @dot semantic Top-K ranking & min_score threshold filtering
//   2. Pinned guaranteed tools (selected first even below min_score) & pinned_count accounting
//   3. 64-bit required_tag_mask filtering & dynamic setEnabled toggling
//   4. Dual budget enforcement: max_tools + max_schema_tokens greedy packing
//   5. Dimension mismatch, zero vector, duplicate ID, full capacity & undersized buffer guards
//   6. Large 64-tool MCP scale routing, deterministic tie-breaking by lowest tool_id & clear() reuse

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RoutableToolEntry,
    ToolRouteHit,
    ToolRouteReport,
    SemanticToolRouter,
} from "cap:agent";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: 8-Lane SIMD Semantic Top-K Ranking & min_score Filtering
// ============================================================================

fn testSimdSemanticTopKAndMinScore()! {
    let entries: [4]RoutableToolEntry = [RoutableToolEntry{}; 4];
    let vecs: [32]f32 = [0.0; 32]; // 4 * 8
    let router = cap.agent.SemanticToolRouter.init(entries[0..], vecs[0..], 8) ?? return error.InitFailed;

    const v1: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]; // exact match with query (~1.0)
    const v2: [8]f32 = [0.8, 0.6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]; // cos = 0.8
    const v3: [8]f32 = [0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]; // cos = 0.0 (below 0.5)
    const v4: [8]f32 = [-1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]; // cos = -1.0 (below 0.5)

    expect_eq(router.registerTool(101, "grep_code", 1, 100, false, v1[0..]), true);
    expect_eq(router.registerTool(102, "read_file", 1, 80, false, v2[0..]), true);
    expect_eq(router.registerTool(103, "send_email", 1, 90, false, v3[0..]), true);
    expect_eq(router.registerTool(104, "delete_db", 1, 120, false, v4[0..]), true);

    const query: [8]f32 = [2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]; // non-unit query (auto normalized)
    let hits: [4]ToolRouteHit = [ToolRouteHit{}; 4];
    const rep: ToolRouteReport = router.routeTools(query[0..], 0, 4, 0, 0.5, hits[0..]) ?? return error.RouteFailed;

    expect_eq(rep.selected_count, 2);
    expect_eq(rep.pinned_count, 0);
    expect_eq(rep.used_schema_tokens, 180);
    expect_eq(rep.filtered_out_count, 2);
    expect_eq(rep.truncated_by_budget, false);

    expect_eq(hits[0].tool_id, 101);
    expect_eq(strEq(hits[0].name, "grep_code"), true);
    expect_eq(hits[0].score > 0.99, true);

    expect_eq(hits[1].tool_id, 102);
    expect_eq(strEq(hits[1].name, "read_file"), true);
    expect_eq(hits[1].score > 0.79 && hits[1].score < 0.81, true);
}

// ============================================================================
// Test 2: Pinned Guaranteed Tools Priority
// ============================================================================

fn testPinnedToolsGuaranteedFirst()! {
    let entries: [4]RoutableToolEntry = [RoutableToolEntry{}; 4];
    let vecs: [32]f32 = [0.0; 32];
    let router = SemanticToolRouter.init(entries[0..], vecs[0..], 8) ?? return error.InitFailed;

    const v_rel: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    const v_ortho: [8]f32 = [0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];

    // Pinned tool is orthogonal (score 0.0 < min_score 0.7) yet MUST be selected first
    expect_eq(router.registerTool(1, "ask_user", 0xFF, 60, true, v_ortho[0..]), true);
    expect_eq(router.registerTool(2, "search_web", 0xFF, 120, false, v_rel[0..]), true);
    expect_eq(router.registerTool(3, "unrelated", 0xFF, 50, false, v_ortho[0..]), true);

    let hits: [4]ToolRouteHit = [ToolRouteHit{}; 4];
    const rep = router.routeTools(v_rel[0..], 0, 4, 0, 0.7, hits[0..]) ?? return error.RouteFailed;

    expect_eq(rep.selected_count, 2);
    expect_eq(rep.pinned_count, 1);
    expect_eq(rep.filtered_out_count, 1);
    expect_eq(hits[0].tool_id, 1);
    expect_eq(hits[0].pinned, true);
    expect_eq(hits[1].tool_id, 2);
    expect_eq(hits[1].pinned, false);
}

// ============================================================================
// Test 3: 64-Bit Tag Mask Filtering & Dynamic setEnabled Toggling
// ============================================================================

fn testTagFilteringAndDynamicEnable()! {
    let entries: [4]RoutableToolEntry = [RoutableToolEntry{}; 4];
    let vecs: [32]f32 = [0.0; 32];
    let router = SemanticToolRouter.init(entries[0..], vecs[0..], 8) ?? return error.InitFailed;

    const TAG_READ: u64 = 1 << 0;
    const TAG_PROD: u64 = 1 << 40;
    const v_all: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];

    expect_eq(router.registerTool(10, "prod_reader", TAG_READ | TAG_PROD, 100, false, v_all[0..]), true);
    expect_eq(router.registerTool(11, "dev_reader", TAG_READ, 100, false, v_all[0..]), true);
    expect_eq(router.registerTool(12, "prod_pinned", TAG_READ | TAG_PROD, 100, true, v_all[0..]), true);

    let hits: [4]ToolRouteHit = [ToolRouteHit{}; 4];
    const rep1 = router.routeTools(v_all[0..], TAG_READ | TAG_PROD, 4, 0, 0.0, hits[0..]) ?? return error.RouteFailed;
    expect_eq(rep1.selected_count, 2); // 12 (pinned) + 10
    expect_eq(rep1.filtered_out_count, 1); // 11 lacks TAG_PROD

    // Disable tool 10 dynamically
    expect_eq(router.setEnabled(10, false), true);
    expect_eq(router.setEnabled(999, false), false); // unknown ID

    const rep2 = router.routeTools(v_all[0..], TAG_READ | TAG_PROD, 4, 0, 0.0, hits[0..]) ?? return error.RouteFailed;
    expect_eq(rep2.selected_count, 1); // only 12 remains
    expect_eq(rep2.filtered_out_count, 2); // 10 disabled + 11 tag mismatch
    expect_eq(hits[0].tool_id, 12);

    // Re-enable tool 10
    expect_eq(router.setEnabled(10, true), true);
    const rep3 = router.routeTools(v_all[0..], TAG_READ | TAG_PROD, 4, 0, 0.0, hits[0..]) ?? return error.RouteFailed;
    expect_eq(rep3.selected_count, 2);
}

// ============================================================================
// Test 4: Dual Budget Enforcement (max_tools + max_schema_tokens)
// ============================================================================

fn testDualBudgetEnforcement()! {
    let entries: [4]RoutableToolEntry = [RoutableToolEntry{}; 4];
    let vecs: [32]f32 = [0.0; 32];
    let router = SemanticToolRouter.init(entries[0..], vecs[0..], 8) ?? return error.InitFailed;

    const v1: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]; // score 1.0, 200 tokens
    const v2: [8]f32 = [0.9, 0.435, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]; // score ~0.9, 250 tokens (exceeds 300 rem!)
    const v3: [8]f32 = [0.8, 0.6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]; // score 0.8, 90 tokens (fits in 300!)
    const v4: [8]f32 = [0.7, 0.714, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]; // score ~0.7, 50 tokens (exceeds 300)

    * = router.registerTool(1, "top1_heavy", 0, 200, false, v1[0..]);
    * = router.registerTool(2, "top2_too_big", 0, 250, false, v2[0..]);
    * = router.registerTool(3, "top3_compact", 0, 90, false, v3[0..]);
    * = router.registerTool(4, "top4_small", 0, 50, false, v4[0..]);

    let hits: [4]ToolRouteHit = [ToolRouteHit{}; 4];
    // max_schema_tokens = 300 -> picks tool 1 (200), skips tool 2 (200+250>300), picks tool 3 (200+90=290), skips tool 4 (290+50>300)
    const rep = router.routeTools(v1[0..], 0, 4, 300, 0.0, hits[0..]) ?? return error.RouteFailed;
    expect_eq(rep.selected_count, 2);
    expect_eq(rep.used_schema_tokens, 290);
    expect_eq(rep.truncated_by_budget, true);
    expect_eq(hits[0].tool_id, 1);
    expect_eq(hits[1].tool_id, 3);

    // Also test max_tools = 1 cap
    const rep_cap = router.routeTools(v1[0..], 0, 1, 1000, 0.0, hits[0..]) ?? return error.RouteFailed;
    expect_eq(rep_cap.selected_count, 1);
    expect_eq(rep_cap.truncated_by_budget, true);
    expect_eq(hits[0].tool_id, 1);
}

// ============================================================================
// Test 5: Boundary & Malformed Input Guards
// ============================================================================

fn testBoundaryAndMalformedGuards()! {
    let entries: [2]RoutableToolEntry = [RoutableToolEntry{}; 2];
    let too_small_vecs: [8]f32 = [0.0; 8]; // needs 2 * 8 = 16
    expect_eq(SemanticToolRouter.init(entries[0..], too_small_vecs[0..], 8) == null, true);
    expect_eq(SemanticToolRouter.init(entries[0..], too_small_vecs[0..], 0) == null, true);

    let vecs: [16]f32 = [0.0; 16];
    let router = SemanticToolRouter.init(entries[0..], vecs[0..], 8) ?? return error.InitFailed;

    const zero_v: [8]f32 = [0.0; 8];
    const wrong_dim: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const good_v: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];

    // Zero vector, wrong dim, empty name rejected
    expect_eq(router.registerTool(1, "zero", 0, 10, false, zero_v[0..]), false);
    expect_eq(router.registerTool(1, "wrong", 0, 10, false, wrong_dim[0..]), false);
    expect_eq(router.registerTool(1, "", 0, 10, false, good_v[0..]), false);

    // Valid registration + duplicate ID rejection + capacity full rejection
    expect_eq(router.registerTool(1, "t1", 0, 10, false, good_v[0..]), true);
    expect_eq(router.registerTool(1, "t1_dup", 0, 10, false, good_v[0..]), false);
    expect_eq(router.registerTool(2, "t2", 0, 10, false, good_v[0..]), true);
    expect_eq(router.registerTool(3, "t3_full", 0, 10, false, good_v[0..]), false);
    expect_eq(router.toolCount(), 2);

    // Invalid query vectors return null
    let hits: [2]ToolRouteHit = [ToolRouteHit{}; 2];
    expect_eq(router.routeTools(zero_v[0..], 0, 2, 0, 0.0, hits[0..]) == null, true);
    expect_eq(router.routeTools(wrong_dim[0..], 0, 2, 0, 0.0, hits[0..]) == null, true);
}

// ============================================================================
// Test 6: 64-Tool Scale Routing, Deterministic Tie-Break & Clear Reuse
// ============================================================================

fn testLargeScaleRoutingAndClearReuse()! {
    let entries: [64]RoutableToolEntry = [RoutableToolEntry{}; 64];
    let vecs: [512]f32 = [0.0; 512]; // 64 * 8
    let router = SemanticToolRouter.init(entries[0..], vecs[0..], 8) ?? return error.InitFailed;

    let i: usize = 0;
    while (i < 64) : (i += 1) {
        let emb: [8]f32 = [0.0; 8];
        if (i == 42 || i == 17) {
            // Identical top-scoring vector -> deterministic tie-break must pick lower tool_id (17 < 42)
            emb[0] = 1.0;
        } else {
            emb[0] = ((i % 7) as f32) * 0.1;
            emb[1] = 1.0;
        }
        expect_eq(router.registerTool(i as u32, "mcp_tool", 1, 50, false, emb[0..]), true);
    }
    expect_eq(router.toolCount(), 64);

    const q: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    let hits: [5]ToolRouteHit = [ToolRouteHit{}; 5];
    const rep = router.routeTools(q[0..], 1, 5, 500, 0.0, hits[0..]) ?? return error.RouteFailed;
    expect_eq(rep.selected_count, 5);
    expect_eq(hits[0].tool_id, 17);
    expect_eq(hits[1].tool_id, 42);

    // Clear and verify clean reuse
    router.clear();
    expect_eq(router.toolCount(), 0);
    expect_eq(router.entryAt(0) == null, true);
    expect_eq(router.registerTool(77, "fresh_tool", 1, 40, false, q[0..]), true);
    expect_eq(router.toolCount(), 1);
}

test("agent.tool_router: 8-lane SIMD semantic Top-K & min_score filtering", testSimdSemanticTopKAndMinScore);
test("agent.tool_router: pinned tools guaranteed inclusion first", testPinnedToolsGuaranteedFirst);
test("agent.tool_router: 64-bit tag mask filtering & dynamic setEnabled toggling", testTagFilteringAndDynamicEnable);
test("agent.tool_router: dual budget enforcement (max_tools + max_schema_tokens)", testDualBudgetEnforcement);
test("agent.tool_router: dimension, zero-vector, duplicate & capacity guards", testBoundaryAndMalformedGuards);
test("agent.tool_router: 64-tool MCP scale routing, tie-break & clear reuse", testLargeScaleRoutingAndClearReuse);
