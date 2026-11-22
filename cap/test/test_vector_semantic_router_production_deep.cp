// Origin: Pure-CP Stdlib Test Suite (lib/vector/semantic_router.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc SIMD Semantic Intent Router:
//   1. max_exemplar multi-exemplar intent matching & stats accounting
//   2. centroid_mean vs max_exemplar aggregation behavior divergence
//   3. Per-route independent min_threshold_f32 -> below_threshold fallback
//   4. Close Top-1/Top-2 margin (< min_margin_f32) -> ambiguous_margin guard
//   5. Automatic L2 normalization scale invariance for unnormalized exemplars & queries
//   6. out_top_hits descending ranking & deterministic tie-break by route_id ascending
//   7. Zero vector, NaN/Inf, dim mismatch, no_routes, duplicate route_id & full pool guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SEM_ROUTER_MAX_ROUTES,
    SemanticAggMode,
    SemanticRouteStatus,
    SemanticRouterConfig,
    SemanticRouteHit,
    SemanticRouteDecision,
    SemanticRouterStats,
    SemanticIntentRouter,
} from "cap:vector";

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

// ============================================================================
// Test 1: max_exemplar Multi-Exemplar Intent Matching & Telemetry
// ============================================================================

fn testMaxExemplarMultiExemplarMatch()! {
    let pool: [64]f32 = [0.0; 64]; // 16 exemplars x 4D
    let pool_s: []f32 = pool[0..];
    const cfg = SemanticRouterConfig{
        dim: 4,
        agg_mode: SemanticAggMode.max_exemplar,
        min_margin_f32: 0.10,
    };
    let router = cap.vector.SemanticIntentRouter.init(cfg, pool_s) ?? return error.InitFailed;

    expect_eq(router.register_route(10, "billing_refund", 0.80), true);
    expect_eq(router.register_route(20, "code_review", 0.80), true);

    // Route 10 has two distinct phrasing exemplars (axes 0 and 1)
    const ex_b1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const ex_b2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    const ex_c1: [4]f32 = [0.0, 0.0, 1.0, 0.0];
    expect_eq(router.add_exemplar(10, ex_b1[0..]), true);
    expect_eq(router.add_exemplar(10, ex_b2[0..]), true);
    expect_eq(router.add_exemplar(20, ex_c1[0..]), true);

    // Query matches Route 10's second exemplar strongly
    const q: [4]f32 = [0.0, 0.98, 0.10, 0.0];
    let hits: [2]SemanticRouteHit = [SemanticRouteHit{}; 2];
    let hits_s: []SemanticRouteHit = hits[0..];

    const d: SemanticRouteDecision = router.route_query(q[0..], hits_s);
    expect_eq(d.status == SemanticRouteStatus.matched, true);
    expect_eq(d.route_id, 10);
    expect_eq(d.best_score_f32 > 0.99, true);
    expect_eq(d.margin_f32 > 0.85, true);
    expect_eq(hits[0].route_id, 10);
    expect_eq(hits[0].passed_threshold, true);
    expect_eq(hits[1].route_id, 20);

    const st: SemanticRouterStats = router.stats();
    expect_eq(st.route_count, 2);
    expect_eq(st.exemplar_count, 3);
    expect_eq(st.total_queries, 1);
    expect_eq(st.matched_count, 1);
    expect_eq(st.fallback_count, 0);
}

// ============================================================================
// Test 2: centroid_mean vs max_exemplar Aggregation Divergence
// ============================================================================

fn testCentroidMeanVsMaxExemplar()! {
    let pool_max: [32]f32 = [0.0; 32];
    let pool_mean: [32]f32 = [0.0; 32];
    let pool_max_s: []f32 = pool_max[0..];
    let pool_mean_s: []f32 = pool_mean[0..];

    let r_max = SemanticIntentRouter.init(
        SemanticRouterConfig{ dim: 4, agg_mode: SemanticAggMode.max_exemplar, min_margin_f32: 0.0 },
        pool_max_s,
    ) ?? return error.InitFailed;
    let r_mean = SemanticIntentRouter.init(
        SemanticRouterConfig{ dim: 4, agg_mode: SemanticAggMode.centroid_mean, min_margin_f32: 0.0 },
        pool_mean_s,
    ) ?? return error.InitFailed;

    expect_eq(r_max.register_route(1, "multi_aspect", 0.80), true);
    expect_eq(r_mean.register_route(1, "multi_aspect", 0.80), true);

    // Two orthogonal exemplars for route 1
    const e1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const e2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    expect_eq(r_max.add_exemplar(1, e1[0..]), true);
    expect_eq(r_max.add_exemplar(1, e2[0..]), true);
    expect_eq(r_mean.add_exemplar(1, e1[0..]), true);
    expect_eq(r_mean.add_exemplar(1, e2[0..]), true);

    const q: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    let hits: [1]SemanticRouteHit = [SemanticRouteHit{}; 1];
    let hits_s: []SemanticRouteHit = hits[0..];

    // max_exemplar -> max(1.0, 0.0) = 1.0 >= 0.80 -> matched
    const d_max = r_max.route_query(q[0..], hits_s);
    expect_eq(d_max.status == SemanticRouteStatus.matched, true);
    expect_eq(absF32(d_max.best_score_f32 - 1.0) < 1.0e-5, true);

    // centroid_mean -> mean(1.0, 0.0) = 0.5 < 0.80 -> below_threshold
    const d_mean = r_mean.route_query(q[0..], hits_s);
    expect_eq(d_mean.status == SemanticRouteStatus.below_threshold, true);
    expect_eq(absF32(d_mean.best_score_f32 - 0.5) < 1.0e-5, true);
}

// ============================================================================
// Test 3: Per-Route Independent Threshold -> below_threshold
// ============================================================================

fn testPerRouteThresholdBelowThreshold()! {
    let pool: [32]f32 = [0.0; 32];
    let pool_s: []f32 = pool[0..];
    let router = SemanticIntentRouter.init(
        SemanticRouterConfig{ dim: 4, agg_mode: SemanticAggMode.max_exemplar, min_margin_f32: 0.05 },
        pool_s,
    ) ?? return error.InitFailed;

    // Route 100 requires very high confidence (0.95); Route 200 requires 0.50
    expect_eq(router.register_route(100, "wire_transfer", 0.95), true);
    expect_eq(router.register_route(200, "faq_help", 0.50), true);

    const e100: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const e200: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    expect_eq(router.add_exemplar(100, e100[0..]), true);
    expect_eq(router.add_exemplar(200, e200[0..]), true);

    // Query has cosine ~0.80 with route 100 (< 0.95 threshold) and ~0.60 with route 200
    const q: [4]f32 = [0.80, 0.60, 0.0, 0.0];
    let hits: [2]SemanticRouteHit = [SemanticRouteHit{}; 2];
    let hits_s: []SemanticRouteHit = hits[0..];

    const d = router.route_query(q[0..], hits_s);
    expect_eq(d.status == SemanticRouteStatus.below_threshold, true);
    expect_eq(d.route_id, 100);
    expect_eq(absF32(d.best_score_f32 - 0.80) < 1.0e-4, true);
    expect_eq(hits[0].passed_threshold, false);
    expect_eq(hits[1].passed_threshold, true);
    expect_eq(router.stats().fallback_count, 1);
}

// ============================================================================
// Test 4: Close Top-1 / Top-2 Margin -> ambiguous_margin Guard
// ============================================================================

fn testAmbiguousMarginGuard()! {
    let pool: [32]f32 = [0.0; 32];
    let pool_s: []f32 = pool[0..];
    let router = SemanticIntentRouter.init(
        SemanticRouterConfig{ dim: 4, agg_mode: SemanticAggMode.max_exemplar, min_margin_f32: 0.15 },
        pool_s,
    ) ?? return error.InitFailed;

    expect_eq(router.register_route(1, "intent_a", 0.60), true);
    expect_eq(router.register_route(2, "intent_b", 0.60), true);

    const ea: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const eb: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    expect_eq(router.add_exemplar(1, ea[0..]), true);
    expect_eq(router.add_exemplar(2, eb[0..]), true);

    // Query at ~43 deg: cos(a) ≈ 0.731, cos(b) ≈ 0.682 -> margin ≈ 0.049 < 0.15 -> ambiguous_margin!
    const q: [4]f32 = [0.731, 0.682, 0.0, 0.0];
    let hits: [2]SemanticRouteHit = [SemanticRouteHit{}; 2];
    let hits_s: []SemanticRouteHit = hits[0..];

    const d = router.route_query(q[0..], hits_s);
    expect_eq(d.status == SemanticRouteStatus.ambiguous_margin, true);
    expect_eq(d.route_id, 1);
    expect_eq(d.best_score_f32 > 0.72, true);
    expect_eq(d.second_score_f32 > 0.67, true);
    expect_eq(d.margin_f32 < 0.15 && d.margin_f32 > 0.0, true);
    expect_eq(router.stats().fallback_count, 1);
}

// ============================================================================
// Test 5: Automatic L2 Normalization Scale Invariance
// ============================================================================

fn testScaleInvariance()! {
    let pool: [16]f32 = [0.0; 16];
    let pool_s: []f32 = pool[0..];
    let router = SemanticIntentRouter.init(
        SemanticRouterConfig{ dim: 4, agg_mode: SemanticAggMode.max_exemplar, min_margin_f32: 0.05 },
        pool_s,
    ) ?? return error.InitFailed;

    expect_eq(router.register_route(7, "scaled_intent", 0.90), true);
    // Unnormalized exemplar scaled by 50x
    const ex_big: [4]f32 = [150.0, 200.0, 0.0, 0.0];
    expect_eq(router.add_exemplar(7, ex_big[0..]), true);

    // Unnormalized query scaled by 0.001x in exact same direction (3:4 ratio)
    const q_tiny: [4]f32 = [0.003, 0.004, 0.0, 0.0];
    let hits: [1]SemanticRouteHit = [SemanticRouteHit{}; 1];
    let hits_s: []SemanticRouteHit = hits[0..];

    const d = router.route_query(q_tiny[0..], hits_s);
    expect_eq(d.status == SemanticRouteStatus.matched, true);
    expect_eq(d.route_id, 7);
    expect_eq(absF32(d.best_score_f32 - 1.0) < 1.0e-5, true);
}

// ============================================================================
// Test 6: out_top_hits Descending Order & Deterministic Tie-Break (route_id asc)
// ============================================================================

fn testRankingAndDeterministicTieBreak()! {
    let pool: [32]f32 = [0.0; 32];
    let pool_s: []f32 = pool[0..];
    let router = SemanticIntentRouter.init(
        SemanticRouterConfig{ dim: 4, agg_mode: SemanticAggMode.max_exemplar, min_margin_f32: 0.0 },
        pool_s,
    ) ?? return error.InitFailed;

    // Register out of ID order: 30, 10, 20
    expect_eq(router.register_route(30, "route_30", 0.50), true);
    expect_eq(router.register_route(10, "route_10", 0.50), true);
    expect_eq(router.register_route(20, "route_20", 0.50), true);

    // Route 30 and Route 10 have IDENTICAL cosine score with query [1, 1, 0, 0]; Route 20 is orthogonal (0.0)
    const e30: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const e10: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    const e20: [4]f32 = [0.0, 0.0, 1.0, 0.0];
    expect_eq(router.add_exemplar(30, e30[0..]), true);
    expect_eq(router.add_exemplar(10, e10[0..]), true);
    expect_eq(router.add_exemplar(20, e20[0..]), true);

    const q: [4]f32 = [1.0, 1.0, 0.0, 0.0];
    let hits: [3]SemanticRouteHit = [SemanticRouteHit{}; 3];
    let hits_s: []SemanticRouteHit = hits[0..];

    const d = router.route_query(q[0..], hits_s);
    expect_eq(d.status == SemanticRouteStatus.matched, true);
    // Tie between 10 and 30 broken deterministically by route_id ascending -> 10 wins!
    expect_eq(d.route_id, 10);
    expect_eq(hits[0].route_id, 10);
    expect_eq(hits[1].route_id, 30);
    expect_eq(hits[2].route_id, 20);
    expect_eq(hits[0].score_f32 > hits[2].score_f32, true);
}

// ============================================================================
// Test 7: Zero Vector, NaN/Inf, Duplicate ID, no_routes & Full Pool Guards
// ============================================================================

fn testDefensiveBoundaryGuards()! {
    const nan_f: f32 = std.math.nan(f32);
    const inf_f: f32 = std.math.inf(f32);

    // Invalid configs & undersized pool
    let tiny_pool: [2]f32 = [0.0; 2];
    let tiny_s: []f32 = tiny_pool[0..];
    expect_eq(SemanticIntentRouter.init(SemanticRouterConfig{ dim: 0 }, tiny_s) == null, true);
    expect_eq(SemanticIntentRouter.init(SemanticRouterConfig{ dim: 4, min_margin_f32: -0.1 }, tiny_s) == null, true);
    expect_eq(SemanticIntentRouter.init(SemanticRouterConfig{ dim: 4 }, tiny_s) == null, true); // pool < dim

    // Pool sized for exactly 2 exemplars of dim 4
    let pool: [8]f32 = [0.0; 8];
    let pool_s: []f32 = pool[0..];
    let router = SemanticIntentRouter.init(SemanticRouterConfig{ dim: 4, min_margin_f32: 0.05 }, pool_s) ?? return error.InitFailed;

    let hits: [2]SemanticRouteHit = [SemanticRouteHit{}; 2];
    let hits_s: []SemanticRouteHit = hits[0..];
    const valid_q: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    // No routes yet -> no_routes
    expect_eq(router.route_query(valid_q[0..], hits_s).status == SemanticRouteStatus.no_routes, true);

    // Invalid route registrations (empty name, out-of-range threshold, duplicate ID)
    expect_eq(router.register_route(1, "", 0.5), false);
    expect_eq(router.register_route(1, "bad_thresh", 1.5), false);
    expect_eq(router.register_route(1, "ok_route", 0.5), true);
    expect_eq(router.register_route(1, "dup_id", 0.5), false);

    // Invalid exemplars (unknown route, zero vec, NaN, dim mismatch)
    const zero_v: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    const nan_v: [4]f32 = [nan_f, 1.0, 0.0, 0.0];
    const inf_v: [4]f32 = [inf_f, 1.0, 0.0, 0.0];
    expect_eq(router.add_exemplar(999, valid_q[0..]), false);
    expect_eq(router.add_exemplar(1, zero_v[0..]), false);
    expect_eq(router.add_exemplar(1, nan_v[0..]), false);
    expect_eq(router.add_exemplar(1, inf_v[0..]), false);
    expect_eq(router.add_exemplar(1, valid_q[0..3]), false);

    // Fill pool capacity (2 exemplars), 3rd must fail cleanly
    expect_eq(router.add_exemplar(1, valid_q[0..]), true);
    const e2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    expect_eq(router.add_exemplar(1, e2[0..]), true);
    expect_eq(router.add_exemplar(1, valid_q[0..]), false); // pool full

    // Invalid queries (zero vector, NaN, dim mismatch) -> invalid_query
    expect_eq(router.route_query(zero_v[0..], hits_s).status == SemanticRouteStatus.invalid_query, true);
    expect_eq(router.route_query(nan_v[0..], hits_s).status == SemanticRouteStatus.invalid_query, true);
    expect_eq(router.route_query(valid_q[0..2], hits_s).status == SemanticRouteStatus.invalid_query, true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("vector semantic_router: max_exemplar multi-exemplar intent match & telemetry", testMaxExemplarMultiExemplarMatch);
test("vector semantic_router: centroid_mean vs max_exemplar aggregation behavior", testCentroidMeanVsMaxExemplar);
test("vector semantic_router: per-route min_threshold_f32 returns below_threshold", testPerRouteThresholdBelowThreshold);
test("vector semantic_router: close Top-1/Top-2 margin triggers ambiguous_margin guard", testAmbiguousMarginGuard);
test("vector semantic_router: automatic L2 normalization scale invariance", testScaleInvariance);
test("vector semantic_router: out_top_hits descending ranking & route_id ascending tie-break", testRankingAndDeterministicTieBreak);
test("vector semantic_router: zero vector, NaN/Inf, duplicate ID, no_routes & full pool guards", testDefensiveBoundaryGuards);
