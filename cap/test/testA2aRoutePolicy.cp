// Origin: Pure-CP Stdlib Test Suite (lib/a2a/routePolicy.cp) | Created: 2026-11-23
// Production Deep Verification Suite for A2A Capability/Cost/Latency/SLA Policy Router:
//   1. 64-bit capability bitmask hard filtering & `no_matching_capability` rejection
//   2. Four policy goals (`lowest_latency`, `lowest_cost`, `highest_reliability`, `balanced_sla`) & ordered fallback chain
//   3. Cost budget (`budget_exceeded`), latency SLA (`latency_sla_exceeded`) & reliability floor gates
//   4. In-flight concurrency saturation skipping & fallback promotion
//   5. `circuit_open` / `draining` isolation & `degraded` graceful fallback toggle
//   6. Live `updateTargetMetrics` rerouting, `stats()` accounting & `cap.a2a` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    A2A_ROUTE_MAX_FALLBACKS,
    RoutePolicyGoal,
    RouteTargetState,
    RouteRejectKind,
    RouteTaskSpec,
    A2aRoutePolicyEngine,
} from "cap:a2a";

const CAP_CODE: u64 = 1 << 0;
const CAP_SQL: u64 = 1 << 1;
const CAP_VISION: u64 = 1 << 2;

test("A2aRoutePolicy: 64-bit capability bitmask filtering & no_matching_capability rejection") {
    let eng = A2aRoutePolicyEngine.init();
    const idx_code = eng.upsertTarget("coder", CAP_CODE, 500, 200, 9800, 16) ?? 99;
    const idx_full = eng.upsertTarget("multimodal", CAP_CODE | CAP_SQL | CAP_VISION, 1200, 400, 9900, 16) ?? 99;
    * = idx_code;

    // Require CODE + VISION -> only "multimodal" matches
    const p1 = eng.planRoute(RouteTaskSpec{
        required_capabilities: CAP_CODE | CAP_VISION,
        goal: RoutePolicyGoal.lowest_cost,
    });
    expect_eq(p1.status, RouteRejectKind.ok);
    expect_eq(p1.primary_index, idx_full);
    expect_eq(p1.fallback_count, @as(usize, 0));
    expect_eq(p1.filtered_by_cap, @as(usize, 1));

    // Require unsupported capability bit -> no_matching_capability
    const p2 = eng.planRoute(RouteTaskSpec{
        required_capabilities: 1 << 10,
        goal: RoutePolicyGoal.balanced_sla,
    });
    expect_eq(p2.status, RouteRejectKind.no_matching_capability);
    expect_eq(p2.filtered_by_cap, @as(usize, 2));
}

test("A2aRoutePolicy: four policy goals & ordered deduplicated fallback chain") {
    let eng = A2aRoutePolicyEngine.init();
    // 0: fast (lat=80ms, cost=900, rel=9600)
    const i_fast = eng.upsertTarget("fast", CAP_CODE, 900, 80, 9600, 32) ?? 99;
    // 1: cheap (lat=500ms, cost=100, rel=9200)
    const i_cheap = eng.upsertTarget("cheap", CAP_CODE, 100, 500, 9200, 32) ?? 99;
    // 2: reliable (lat=300ms, cost=1200, rel=9990)
    const i_rel = eng.upsertTarget("reliable", CAP_CODE, 1200, 300, 9990, 32) ?? 99;
    // 3: balanced (lat=150ms, cost=250, rel=9950)
    const i_bal = eng.upsertTarget("balanced", CAP_CODE, 250, 150, 9950, 32) ?? 99;

    // 1. lowest_latency -> fast (80) > balanced (150) > reliable (300) > cheap (500)
    const p_lat = eng.planRoute(RouteTaskSpec{ required_capabilities: CAP_CODE, goal: RoutePolicyGoal.lowest_latency });
    expect_eq(p_lat.status, RouteRejectKind.ok);
    expect_eq(p_lat.primary_index, i_fast);
    expect_eq(p_lat.fallback_count, @as(usize, 3));
    expect_eq(p_lat.fallback_indices[0], i_bal);
    expect_eq(p_lat.fallback_indices[1], i_rel);
    expect_eq(p_lat.fallback_indices[2], i_cheap);

    // 2. lowest_cost -> cheap (100) > balanced (250) > fast (900) > reliable (1200)
    const p_cost = eng.planRoute(RouteTaskSpec{ required_capabilities: CAP_CODE, goal: RoutePolicyGoal.lowest_cost });
    expect_eq(p_cost.status, RouteRejectKind.ok);
    expect_eq(p_cost.primary_index, i_cheap);
    expect_eq(p_cost.fallback_indices[0], i_bal);
    expect_eq(p_cost.fallback_indices[1], i_fast);
    expect_eq(p_cost.fallback_indices[2], i_rel);

    // 3. highest_reliability -> reliable (9990) > balanced (9950) > fast (9600) > cheap (9200)
    const p_rel = eng.planRoute(RouteTaskSpec{ required_capabilities: CAP_CODE, goal: RoutePolicyGoal.highest_reliability });
    expect_eq(p_rel.status, RouteRejectKind.ok);
    expect_eq(p_rel.primary_index, i_rel);
    expect_eq(p_rel.fallback_indices[0], i_bal);
    expect_eq(p_rel.fallback_indices[1], i_fast);
    expect_eq(p_rel.fallback_indices[2], i_cheap);

    // 4. balanced_sla -> balanced wins overall
    const p_bal = eng.planRoute(RouteTaskSpec{ required_capabilities: CAP_CODE, goal: RoutePolicyGoal.balanced_sla });
    expect_eq(p_bal.status, RouteRejectKind.ok);
    expect_eq(p_bal.primary_index, i_bal);
    expect_true(p_bal.primary_score > 0.0);
    expect_true(p_bal.fallback_count <= A2A_ROUTE_MAX_FALLBACKS);
}

test("A2aRoutePolicy: cost budget, latency SLA & reliability floor rejection gates") {
    let eng = A2aRoutePolicyEngine.init();
    * = eng.upsertTarget("a1", CAP_SQL, 1000, 400, 9500, 16);
    * = eng.upsertTarget("a2", CAP_SQL, 1500, 350, 9800, 16);

    // Budget cap 500 -> both exceed budget -> budget_exceeded
    const p_budget = eng.planRoute(RouteTaskSpec{
        required_capabilities: CAP_SQL,
        max_cost_micros: 500,
        goal: RoutePolicyGoal.lowest_cost,
    });
    expect_eq(p_budget.status, RouteRejectKind.budget_exceeded);
    expect_eq(p_budget.filtered_by_sla, @as(usize, 2));

    // Latency SLA 200ms -> both exceed latency -> latency_sla_exceeded
    const p_lat = eng.planRoute(RouteTaskSpec{
        required_capabilities: CAP_SQL,
        max_latency_ms: 200,
        goal: RoutePolicyGoal.lowest_latency,
    });
    expect_eq(p_lat.status, RouteRejectKind.latency_sla_exceeded);
    expect_eq(p_lat.filtered_by_sla, @as(usize, 2));

    // Reliability floor 9900 -> both below floor -> all_unavailable
    const p_rel = eng.planRoute(RouteTaskSpec{
        required_capabilities: CAP_SQL,
        min_success_rate_bps: 9900,
        goal: RoutePolicyGoal.highest_reliability,
    });
    expect_eq(p_rel.status, RouteRejectKind.all_unavailable);
    expect_eq(p_rel.filtered_by_sla, @as(usize, 2));
}

test("A2aRoutePolicy: concurrency saturation skips full targets") {
    let eng = A2aRoutePolicyEngine.init();
    const i1 = eng.upsertTarget("fast-full", CAP_CODE, 300, 50, 9900, 4) ?? 99;
    const i2 = eng.upsertTarget("backup-free", CAP_CODE, 400, 120, 9800, 8) ?? 99;
    * = i1;

    // Saturate fast-full (4/4 inflight)
    expect_true(eng.updateTargetMetrics("fast-full", 50, 9900, 4));

    const p = eng.planRoute(RouteTaskSpec{
        required_capabilities: CAP_CODE,
        goal: RoutePolicyGoal.lowest_latency,
    });
    expect_eq(p.status, RouteRejectKind.ok);
    expect_eq(p.primary_index, i2);
    expect_eq(p.filtered_by_state, @as(usize, 1));
    expect_eq(p.fallback_count, @as(usize, 0));
}

test("A2aRoutePolicy: circuit_open/draining isolation & graceful degraded fallback") {
    let eng = A2aRoutePolicyEngine.init();
    * = eng.upsertTarget("t-open", CAP_CODE, 200, 100, 9900, 16);
    * = eng.upsertTarget("t-drain", CAP_CODE, 200, 100, 9900, 16);
    const i_deg = eng.upsertTarget("t-degraded", CAP_CODE, 300, 200, 9500, 16) ?? 99;

    expect_true(eng.setTargetState("t-open", RouteTargetState.circuit_open));
    expect_true(eng.setTargetState("t-drain", RouteTargetState.draining));
    expect_true(eng.setTargetState("t-degraded", RouteTargetState.degraded));

    // When allow_degraded_fallback == false -> rejected with all_unavailable
    const p_strict = eng.planRoute(RouteTaskSpec{
        required_capabilities: CAP_CODE,
        allow_degraded_fallback: false,
        goal: RoutePolicyGoal.lowest_latency,
    });
    expect_eq(p_strict.status, RouteRejectKind.all_unavailable);
    expect_eq(p_strict.filtered_by_state, @as(usize, 3));

    // When allow_degraded_fallback == true -> gracefully routes to t-degraded
    const p_deg = eng.planRoute(RouteTaskSpec{
        required_capabilities: CAP_CODE,
        allow_degraded_fallback: true,
        goal: RoutePolicyGoal.lowest_latency,
    });
    expect_eq(p_deg.status, RouteRejectKind.ok);
    expect_eq(p_deg.primary_index, i_deg);
    expect_true(p_deg.used_degraded_fallback);

    const st = eng.stats();
    expect_eq(st.total_routes, @as(u64, 2));
    expect_eq(st.routed_ok, @as(u64, 1));
    expect_eq(st.routed_degraded, @as(u64, 1));
    expect_eq(st.rejected_count, @as(u64, 1));
}

test("A2aRoutePolicy: dynamic metric updates reroute live & cap.a2a facade parity") {
    let eng = cap.a2a.A2aRoutePolicyEngine.init();
    const i1 = eng.upsertTarget("worker-1", CAP_CODE, 300, 100, 9900, 16) ?? 99;
    const i2 = eng.upsertTarget("worker-2", CAP_CODE, 300, 300, 9900, 16) ?? 99;

    // Initially worker-1 wins lowest_latency
    const p1 = eng.planRoute(RouteTaskSpec{ required_capabilities: CAP_CODE, goal: RoutePolicyGoal.lowest_latency });
    expect_eq(p1.primary_index, i1);

    // worker-1 suffers latency spike to 900ms -> worker-2 immediately becomes primary!
    expect_true(eng.updateTargetMetrics("worker-1", 900, 9900, 2));
    const p2 = eng.planRoute(RouteTaskSpec{ required_capabilities: CAP_CODE, goal: RoutePolicyGoal.lowest_latency });
    expect_eq(p2.primary_index, i2);
    expect_eq(p2.fallback_indices[0], i1);

    if (t1 := eng.getTarget("worker-1")) {
        expect_eq(t1.p95_latency_ms, @as(u32, 900));
        expect_eq(t1.active_inflight, @as(u16, 2));
    } else {
        expect_true(false);
    }
    expect_eq(eng.stats().target_count, @as(usize, 2));
}
