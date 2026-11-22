// Origin: Pure-CP Stdlib Test Suite (lib/prompt/modelRoute.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc Multi-Model LLM Gateway Router & Cascading Fallback:
//   1. Cost estimator precision & all 4 routing strategies (`lowest_cost`, `lowest_latency`, `highest_quality`, `balanced_sla`)
//   2. Hard filtering: capability bitmask, context window, SLA latency/cost/quality bounds & administrative disable
//   3. 429 `rate_limited` immediate exponential-backoff cooldown, fallback promotion & half-open recovery
//   4. `server_error`/`timeout` failure-threshold tripping & EWMA latency adaptation dynamically reordering routes
//   5. 16-endpoint capacity, invalid-spec/duplicate guards & truncation-safe output chain
//   6. Empty-route telemetry & time-aware `stats(now_ms)` health breakdown
//   7. `cap.prompt` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MODEL_ROUTE_MAX_ENDPOINTS,
    MODEL_CAP_TOOLS,
    MODEL_CAP_VISION,
    MODEL_CAP_JSON_MODE,
    MODEL_CAP_REASONING,
    MODEL_CAP_STREAMING,
    ModelRouteStrategy,
    ModelEndpointHealth,
    ModelCallOutcomeKind,
    ModelEndpointSpec,
    ModelEndpointState,
    ModelRouteRequest,
    ModelRouteCandidate,
    ModelRouterConfig,
    estimateRequestCostMicros,
    ModelGatewayRouter,
} from "cap:prompt";

test("ModelRoute: cost estimation & all 4 routing strategies produce exact ordered chains") {
    // 10,000 in @ $3/1M = 30,000 micros; 2,000 out @ $15/1M = 30,000 micros => 60,000 micros
    expect_eq(estimateRequestCostMicros(10_000, 2_000, 3_000_000, 15_000_000), 60_000 as u64);
    expect_eq(estimateRequestCostMicros(0, 0, 3_000_000, 15_000_000), 0 as u64);

    let gw = ModelGatewayRouter.init();
    // Model 1: Flagship (highest quality, slowest, priciest)
    expect_true(gw.registerEndpoint(ModelEndpointSpec{
        model_id: 1,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_JSON_MODE | MODEL_CAP_STREAMING,
        max_context_tokens: 200_000,
        input_price_micros_per_1m: 3_000_000,
        output_price_micros_per_1m: 15_000_000,
        quality_bps: 9600,
        initial_latency_ms: 900,
    }));
    // Model 2: Balanced mid-tier (great quality, moderate cost & latency)
    expect_true(gw.registerEndpoint(ModelEndpointSpec{
        model_id: 2,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_JSON_MODE | MODEL_CAP_STREAMING,
        max_context_tokens: 128_000,
        input_price_micros_per_1m: 800_000,
        output_price_micros_per_1m: 2_400_000,
        quality_bps: 9000,
        initial_latency_ms: 400,
    }));
    // Model 3: Ultra-fast cheap mini (lowest cost & lowest latency, lower quality)
    expect_true(gw.registerEndpoint(ModelEndpointSpec{
        model_id: 3,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_JSON_MODE | MODEL_CAP_STREAMING,
        max_context_tokens: 128_000,
        input_price_micros_per_1m: 150_000,
        output_price_micros_per_1m: 600_000,
        quality_bps: 7400,
        initial_latency_ms: 180,
    }));

    let chain: [4]ModelRouteCandidate = [ModelRouteCandidate{}; 4];
    let chain_s: []ModelRouteCandidate = chain[0..];

    // lowest_cost -> 3, 2, 1
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 4000,
        estimated_output_tokens: 1000,
        strategy: ModelRouteStrategy.lowest_cost,
    }, 1000, chain_s), 3 as usize);
    expect_eq(chain[0].model_id, 3 as u64);
    expect_eq(chain[1].model_id, 2 as u64);
    expect_eq(chain[2].model_id, 1 as u64);

    // lowest_latency -> 3, 2, 1
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 4000,
        estimated_output_tokens: 1000,
        strategy: ModelRouteStrategy.lowest_latency,
    }, 1000, chain_s), 3 as usize);
    expect_eq(chain[0].model_id, 3 as u64);
    expect_eq(chain[1].model_id, 2 as u64);
    expect_eq(chain[2].model_id, 1 as u64);

    // highest_quality -> 1, 2, 3
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 4000,
        estimated_output_tokens: 1000,
        strategy: ModelRouteStrategy.highest_quality,
    }, 1000, chain_s), 3 as usize);
    expect_eq(chain[0].model_id, 1 as u64);
    expect_eq(chain[1].model_id, 2 as u64);
    expect_eq(chain[2].model_id, 3 as u64);

    // balanced_sla -> Model 2 (high quality + strong cost/latency efficiency) beats both extremes
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 4000,
        estimated_output_tokens: 1000,
        strategy: ModelRouteStrategy.balanced_sla,
    }, 1000, chain_s), 3 as usize);
    expect_eq(chain[0].model_id, 2 as u64);
}

test("ModelRoute: capability bitmask, context window, SLA caps & setEnabled filtering") {
    let gw = ModelGatewayRouter.init();
    * = gw.registerEndpoint(ModelEndpointSpec{
        model_id: 10,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_VISION,
        max_context_tokens: 32_000,
        input_price_micros_per_1m: 1_000_000,
        output_price_micros_per_1m: 2_000_000,
        quality_bps: 8500,
        initial_latency_ms: 300,
    });
    * = gw.registerEndpoint(ModelEndpointSpec{
        model_id: 20,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_VISION | MODEL_CAP_REASONING,
        max_context_tokens: 128_000,
        input_price_micros_per_1m: 5_000_000,
        output_price_micros_per_1m: 10_000_000,
        quality_bps: 9500,
        initial_latency_ms: 1200,
    });

    let chain: [4]ModelRouteCandidate = [ModelRouteCandidate{}; 4];
    let chain_s: []ModelRouteCandidate = chain[0..];

    // Require REASONING -> only model 20 matches
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_VISION | MODEL_CAP_REASONING,
        estimated_input_tokens: 10_000,
        estimated_output_tokens: 2_000,
        strategy: ModelRouteStrategy.highest_quality,
    }, 1000, chain_s), 1 as usize);
    expect_eq(chain[0].model_id, 20 as u64);

    // Context window 50,000 exceeds model 10 (32,000) -> only model 20 matches
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 45_000,
        estimated_output_tokens: 5_000,
        strategy: ModelRouteStrategy.lowest_cost,
    }, 1000, chain_s), 1 as usize);
    expect_eq(chain[0].model_id, 20 as u64);

    // Hard latency cap 500ms excludes model 20 (1200ms) -> only model 10 matches
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 4_000,
        estimated_output_tokens: 1_000,
        strategy: ModelRouteStrategy.highest_quality,
        max_latency_ms: 500,
    }, 1000, chain_s), 1 as usize);
    expect_eq(chain[0].model_id, 10 as u64);

    // Administratively disable model 10 -> empty chain
    expect_true(gw.setEnabled(10, false));
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 4_000,
        estimated_output_tokens: 1_000,
        strategy: ModelRouteStrategy.highest_quality,
        max_latency_ms: 500,
    }, 1000, chain_s), 0 as usize);
    expect_eq(gw.stats(1000).disabled_count, 1 as usize);
    expect_eq(gw.stats(1000).total_routes_empty, 1 as u64);
}

test("ModelRoute: 429 rate_limited exponential-backoff cooldown, fallback & half-open recovery") {
    let gw = ModelGatewayRouter.initWithConfig(ModelRouterConfig{
        base_cooldown_ms: 5000,
        max_cooldown_ms: 60_000,
        fail_threshold: 3,
        ewma_alpha_bps: 5000,
    });
    * = gw.registerEndpoint(ModelEndpointSpec{
        model_id: 1,
        cap_mask: MODEL_CAP_TOOLS,
        max_context_tokens: 100_000,
        input_price_micros_per_1m: 1_000_000,
        output_price_micros_per_1m: 2_000_000,
        quality_bps: 9500,
        initial_latency_ms: 400,
    });
    * = gw.registerEndpoint(ModelEndpointSpec{
        model_id: 2,
        cap_mask: MODEL_CAP_TOOLS,
        max_context_tokens: 100_000,
        input_price_micros_per_1m: 1_000_000,
        output_price_micros_per_1m: 2_000_000,
        quality_bps: 8500,
        initial_latency_ms: 400,
    });

    // Model 1 hits 429 at t = 10_000 -> immediate 5000ms cooldown (until 15_000)
    expect_true(gw.recordOutcome(1, 100, ModelCallOutcomeKind.rate_limited, 10_000));
    const ep1 = gw.getEndpoint(1) ?? ModelEndpointState{};
    expect_eq(ep1.health, ModelEndpointHealth.cooldown);
    expect_eq(ep1.cooldown_until_ms, 15_000 as u64);

    // At t = 12_000 (inside cooldown), model 1 is excluded and model 2 becomes primary!
    let chain: [4]ModelRouteCandidate = [ModelRouteCandidate{}; 4];
    let chain_s: []ModelRouteCandidate = chain[0..];
    const req = ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 1000,
        estimated_output_tokens: 500,
        strategy: ModelRouteStrategy.highest_quality,
    };
    expect_eq(gw.planRoute(req, 12_000, chain_s), 1 as usize);
    expect_eq(chain[0].model_id, 2 as u64);

    // At t = 15_000 (cooldown expired), model 1 is admitted for half-open probe
    expect_eq(gw.planRoute(req, 15_000, chain_s), 2 as usize);
    expect_eq(chain[0].model_id, 1 as u64);

    // Second consecutive 429 at t = 15_000 doubles cooldown to 10_000ms (until 25_000)
    expect_true(gw.recordOutcome(1, 100, ModelCallOutcomeKind.rate_limited, 15_000));
    expect_eq((gw.getEndpoint(1) ?? ModelEndpointState{}).cooldown_until_ms, 25_000 as u64);

    // Successful probe at t = 25_000 clears cooldown_level & restores healthy
    expect_true(gw.recordOutcome(1, 300, ModelCallOutcomeKind.ok, 25_000));
    const ep1_ok = gw.getEndpoint(1) ?? ModelEndpointState{};
    expect_eq(ep1_ok.health, ModelEndpointHealth.healthy);
    expect_eq(ep1_ok.cooldown_level, 0 as u8);
    expect_eq(ep1_ok.consecutive_failures, 0 as u32);
}

test("ModelRoute: server_error/timeout threshold circuit trip & EWMA latency reordering") {
    let gw = ModelGatewayRouter.initWithConfig(ModelRouterConfig{
        base_cooldown_ms: 4000,
        max_cooldown_ms: 32_000,
        fail_threshold: 3,
        ewma_alpha_bps: 5000, // 50% weight for fast adaptation in test
    });
    * = gw.registerEndpoint(ModelEndpointSpec{
        model_id: 1,
        cap_mask: MODEL_CAP_TOOLS,
        max_context_tokens: 64_000,
        input_price_micros_per_1m: 1_000_000,
        output_price_micros_per_1m: 1_000_000,
        quality_bps: 9000,
        initial_latency_ms: 200,
    });
    * = gw.registerEndpoint(ModelEndpointSpec{
        model_id: 2,
        cap_mask: MODEL_CAP_TOOLS,
        max_context_tokens: 64_000,
        input_price_micros_per_1m: 1_000_000,
        output_price_micros_per_1m: 1_000_000,
        quality_bps: 9000,
        initial_latency_ms: 600,
    });

    // Two timeouts on model 1 -> degraded (not yet tripped), EWMA spikes above model 2
    expect_true(gw.recordOutcome(1, 2000, ModelCallOutcomeKind.timeout, 1000)); // EWMA: (200+2000)/2 = 1100
    expect_eq((gw.getEndpoint(1) ?? ModelEndpointState{}).health, ModelEndpointHealth.degraded);
    expect_eq((gw.getEndpoint(1) ?? ModelEndpointState{}).ewma_latency_ms, 1100 as u32);

    // lowest_latency now prefers model 2 (600ms) over model 1 (1100ms)!
    let chain: [2]ModelRouteCandidate = [ModelRouteCandidate{}; 2];
    let chain_s: []ModelRouteCandidate = chain[0..];
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 1000,
        estimated_output_tokens: 100,
        strategy: ModelRouteStrategy.lowest_latency,
    }, 1100, chain_s), 2 as usize);
    expect_eq(chain[0].model_id, 2 as u64);

    // 2nd and 3rd failures trip cooldown circuit
    expect_true(gw.recordOutcome(1, 0, ModelCallOutcomeKind.server_error, 1200));
    expect_eq((gw.getEndpoint(1) ?? ModelEndpointState{}).health, ModelEndpointHealth.degraded);
    expect_true(gw.recordOutcome(1, 0, ModelCallOutcomeKind.server_error, 1300));
    expect_eq((gw.getEndpoint(1) ?? ModelEndpointState{}).health, ModelEndpointHealth.cooldown);
    expect_eq((gw.getEndpoint(1) ?? ModelEndpointState{}).cooldown_until_ms, 5300 as u64);
}

test("ModelRoute: 16-endpoint capacity, invalid specs & truncation-safe output chain") {
    let gw = ModelGatewayRouter.init();

    // Invalid specs rejected
    expect_true(!gw.registerEndpoint(ModelEndpointSpec{ model_id: 0, quality_bps: 8000 }));
    expect_true(!gw.registerEndpoint(ModelEndpointSpec{ model_id: 1, quality_bps: 10001 }));

    // Fill all 16 slots
    let id: u64 = 1;
    while (id <= MODEL_ROUTE_MAX_ENDPOINTS as u64) : (id += 1) {
        expect_true(gw.registerEndpoint(ModelEndpointSpec{
            model_id: id,
            cap_mask: MODEL_CAP_TOOLS,
            max_context_tokens: 64_000,
            input_price_micros_per_1m: id * 100_000,
            output_price_micros_per_1m: id * 200_000,
            quality_bps: 8000,
            initial_latency_ms: 300,
        }));
    }
    // Duplicate & 17th endpoint rejected
    expect_true(!gw.registerEndpoint(ModelEndpointSpec{ model_id: 1, quality_bps: 8000 }));
    expect_true(!gw.registerEndpoint(ModelEndpointSpec{ model_id: 99, quality_bps: 8000 }));

    // Truncation-safe 3-element chain picks top 3 cheapest (models 1, 2, 3)
    let top3: [3]ModelRouteCandidate = [ModelRouteCandidate{}; 3];
    let top3_s: []ModelRouteCandidate = top3[0..];
    expect_eq(gw.planRoute(ModelRouteRequest{
        required_caps: MODEL_CAP_TOOLS,
        estimated_input_tokens: 1000,
        estimated_output_tokens: 500,
        strategy: ModelRouteStrategy.lowest_cost,
    }, 5000, top3_s), 3 as usize);
    expect_eq(top3[0].model_id, 1 as u64);
    expect_eq(top3[1].model_id, 2 as u64);
    expect_eq(top3[2].model_id, 3 as u64);
}

test("ModelRoute: global telemetry stats & cap.prompt namespace facade parity") {
    let gw = cap.prompt.ModelGatewayRouter.init();
    * = gw.registerEndpoint(cap.prompt.ModelEndpointSpec{
        model_id: 100,
        cap_mask: cap.prompt.MODEL_CAP_TOOLS | cap.prompt.MODEL_CAP_STREAMING,
        max_context_tokens: 128_000,
        input_price_micros_per_1m: 500_000,
        output_price_micros_per_1m: 1_500_000,
        quality_bps: 9100,
        initial_latency_ms: 350,
    });
    * = gw.registerEndpoint(cap.prompt.ModelEndpointSpec{
        model_id: 200,
        cap_mask: cap.prompt.MODEL_CAP_TOOLS | cap.prompt.MODEL_CAP_STREAMING,
        max_context_tokens: 128_000,
        input_price_micros_per_1m: 2_000_000,
        output_price_micros_per_1m: 6_000_000,
        quality_bps: 9700,
        initial_latency_ms: 700,
    });

    * = gw.recordOutcome(100, 300, cap.prompt.ModelCallOutcomeKind.ok, 1000);
    * = gw.recordOutcome(200, 0, cap.prompt.ModelCallOutcomeKind.rate_limited, 1000);

    const st = gw.stats(2000);
    expect_eq(st.endpoint_count, 2 as usize);
    expect_eq(st.capacity, MODEL_ROUTE_MAX_ENDPOINTS);
    expect_eq(st.healthy_count, 1 as usize);
    expect_eq(st.cooldown_count, 1 as usize);
    expect_eq(st.total_outcomes_ok, 1 as u64);
    expect_eq(st.total_outcomes_failed, 1 as u64);
    expect_eq(st.total_circuit_trips, 1 as u64);

    // After cooldown expires at t = 10_000, stats dynamically reflects 0 in cooldown
    expect_eq(gw.stats(10_000).cooldown_count, 0 as usize);
}
